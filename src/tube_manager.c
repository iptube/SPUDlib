/*
 * Copyright (c) 2015 SPUDlib authors.  See LICENSE file.
 */

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <string.h>

#include "tube.h"
#include "ls_eventing.h"
#include "ls_htable.h"
#include "ls_log.h"
#include "ls_sockaddr.h"

#include "tube_manager_int.h"

#define GHEAP_MALLOC ls_data_malloc
#define GHEAP_FREE ls_data_free
#include "../vendor/gheap/gpriority_queue.h"

#define DEFAULT_HASH_SIZE 65521
#define MAXBUFLEN 1500

#ifndef MAX
#define MAX(a,b) ( ( (a) > (b) ) ? (a) : (b) )
#endif

static tube_sendmsg_func _sendmsg_func = sendmsg;
static tube_recvmsg_func _recvmsg_func = recvmsg;

typedef struct _sig_context {
  int                  sig;
  tube_manager*        mgr;
  sig_t                cb;
  struct _sig_context* next;
} sig_context;

static sig_context* sig_contexts = NULL;

static int
_timer_less(const void* const context,
            const void* const a,
            const void* const b)
{
  UNUSED_PARAM(context);

  return ls_timer_greater(*(ls_timer**)a, *(ls_timer**)b);
}

static void
_timer_move(void* const       dst,
            const void* const src)
{
  ls_timer** dt = (void*)dst;
  ls_timer** st = (void*)src;
  *dt = *st;
}

static void
_timer_del(void* item)
{
  ls_timer_destroy(*(ls_timer**)item);
}

static unsigned int
hash_id(const void* id)
{
  /* treat the 8 bytes of tube ID like a long long. */
  uint64_t key = *(uint64_t*)id;

  /* from */
  /* https://gist.github.com/badboy/6267743#64-bit-to-32-bit-hash-functions */
  key = (~key) + (key << 18);
  key = key ^ (key >> 31);
  key = key * 21;
  key = key ^ (key >> 11);
  key = key + (key << 6);
  key = key ^ (key >> 22);
  return (unsigned int) key;
}

static int
compare_id(const void* key1,
           const void* key2)
{
  int      ret = 0;
  uint64_t k1  = *(uint64_t*)key1;
  uint64_t k2  = *(uint64_t*)key2;
  if (k1 < k2)
  {
    ret = -1;
  }
  else
  {
    ret = (k1 == k2) ? 0 : 1;
  }
  return ret;
}

/* "friend" functions */
bool
_tube_manager_init(tube_manager* m,
                   int           buckets,
                   ls_err*       err)
{
  m->initialized = false;
  m->sock4       = -1;
  m->sock6       = -1;
  m->max_fd      = -1;
  m->keep_going  = true;
  m->pipe[0]     = -1;
  m->pipe[1]     = -1;

  if (buckets <= 0)
  {
    buckets = DEFAULT_HASH_SIZE;
  }

  if (pthread_mutex_init(&m->lock, NULL) != 0)
  {
    LS_ERROR(err, -errno);
    goto cleanup;
  }

  if ( !ls_htable_create(buckets, hash_id, compare_id, &m->tubes, err) )
  {
    goto cleanup;
  }

  if ( !ls_event_dispatcher_create(m, &m->dispatcher, err) )
  {
    goto cleanup;
  }

  if ( !ls_event_dispatcher_create_event(m->dispatcher,
                                         EV_LOOPSTART_NAME,
                                         &m->e_loopstart,
                                         err) ||
       !ls_event_dispatcher_create_event(m->dispatcher,
                                         EV_RUNNING_NAME,
                                         &m->e_running,
                                         err) ||
       !ls_event_dispatcher_create_event(m->dispatcher,
                                         EV_DATA_NAME,
                                         &m->e_data,
                                         err) ||
       !ls_event_dispatcher_create_event(m->dispatcher,
                                         EV_CLOSE_NAME,
                                         &m->e_close,
                                         err) ||
       !ls_event_dispatcher_create_event(m->dispatcher,
                                         EV_ADD_NAME,
                                         &m->e_add,
                                         err) ||
       !ls_event_dispatcher_create_event(m->dispatcher,
                                         EV_REMOVE_NAME,
                                         &m->e_remove,
                                         err) )
  {
    goto cleanup;
  }

  {
    static const struct gheap_ctx paged_binary_heap_ctx = {
      .fanout            = 2,
      .page_chunks       = 512,
      .item_size         = sizeof(ls_timer*),
      .less_comparer     = &_timer_less,
      .less_comparer_ctx = NULL,
      .item_mover        = &_timer_move,
    };
    m->timer_q = gpriority_queue_create(&paged_binary_heap_ctx, _timer_del);
    if (!m->timer_q)
    {
      LS_ERROR(err, LS_ERR_NO_MEMORY);
      goto cleanup;
    }
  }

  /* Prime the pump to make sure we always have the current time */
  if (gettimeofday(&m->last, NULL) != 0)
  {
    LS_ERROR(err, -errno);
    goto cleanup;
  }

  {
    /* Set pipe to non-blocking, both directions */
    int i, flags;
    if (pipe(m->pipe) == -1)
    {
      LS_ERROR(err, -errno);
      goto cleanup;
    }

    for (i = 0; i < 2; i++)
    {
      m->max_fd = MAX(m->max_fd, m->pipe[i]);
      flags     = fcntl(m->pipe[i], F_GETFL);
      if (flags == -1)
      {
        LS_ERROR(err, -errno);
        goto cleanup;
      }
      if (fcntl(m->pipe[i], F_SETFL, flags | O_NONBLOCK) == -1)
      {
        LS_ERROR(err, -errno);
        goto cleanup;
      }
    }
  }
  return true;
cleanup:
  _tube_manager_finalize(m);
  return false;
}

void
_tube_manager_finalize(tube_manager* mgr)
{
  assert(mgr);
  mgr->keep_going = false;
  if (mgr->initialized)
  {
    tube_manager_interrupt(mgr, -1, NULL);
  }
  mgr->initialized = false;

  if (pthread_mutex_destroy(&mgr->lock) != 0)
  {
    LS_LOG_PERROR("pthread_mutex_destroy");
    /* keep destroying */
  }
  if (mgr->tubes)
  {
    /* will clean, but not send out events. */
    /* TODO: consider sending events, but not from clean_tube */
    ls_htable_destroy(mgr->tubes);
    mgr->tubes = NULL;
  }
  if (mgr->dispatcher)
  {
    ls_event_dispatcher_destroy(mgr->dispatcher);
    mgr->dispatcher = NULL;
  }
  if (mgr->timer_q)
  {
    gpriority_queue_delete(mgr->timer_q);
    mgr->timer_q = NULL;
  }
}

ls_event_dispatcher*
_tube_manager_get_dispatcher(tube_manager* mgr)
{
  assert(mgr);
  return mgr->dispatcher;
}

LS_API bool
tube_manager_create(int            buckets,
                    tube_manager** m,
                    ls_err*        err)
{
  tube_manager* ret = NULL;
  assert(m != NULL);
  ret = ls_data_calloc( 1, sizeof(tube_manager) );
  if (ret == NULL)
  {
    LS_ERROR(err, LS_ERR_NO_MEMORY);
    return false;
  }

  if ( !_tube_manager_init(ret, buckets, err) )
  {
    ls_data_free(ret);
    *m = ret = NULL;
    return false;
  }
  ret->initialized = true;
  *m               = ret;
  return true;
}

LS_API void
tube_manager_destroy(tube_manager* mgr)
{
  if (NULL == mgr)
  {
    /* treat it as a no-op */
    return;
  }
  _tube_manager_finalize(mgr);
  ls_data_free(mgr);
}

LS_API void
tube_manager_set_data(tube_manager* m,
                      void*         data)
{
  assert(m);
  m->data = data;
}

LS_API void*
tube_manager_get_data(tube_manager* m)
{
  assert(m);
  return m->data;
}

LS_API bool
tube_manager_socket(tube_manager* m,
                    int           port,
                    ls_err*       err)
{
  assert(m);
  assert(port >= 0);
  assert(port <= 0xffff);

  tube_manager_set_policy_responder(m, port != 0);

  m->sock6 = socket(PF_INET6, SOCK_DGRAM, 0);
  if (m->sock6 < 0)
  {
    LS_ERROR(err, -errno);
    return false;
  }

#ifdef IPV6_V6ONLY
  {
    /* we're going to have a separate v4 socket */
    const int on = 1;
    if (setsockopt( m->sock6, IPPROTO_IPV6, IPV6_V6ONLY, &on, sizeof(on) ) != 0)
    {
      LS_ERROR(err, -errno);
      return false;
    }
  }
#endif

  {
    struct sockaddr_in6 addr;
    ls_sockaddr_v6_any(&addr, port);
    if (bind( m->sock6,
              (struct sockaddr*)&addr,
              ls_sockaddr_get_length( (struct sockaddr*)&addr ) ) != 0)
    {
      LS_ERROR(err, -errno);
      return false;
    }
  }

#ifdef IPV6_RECVPKTINFO
  {
    const int on = 1;
    if (setsockopt( m->sock6, IPPROTO_IPV6, IPV6_RECVPKTINFO, &on,
                    sizeof(on) ) != 0)
    {
      LS_ERROR(err, -errno);
      return false;
    }
  }
#else
    #pragma message "No IPV6_RECVPKTINFO.  Destination addresses won't work."
#endif

#ifdef SO_TIMESTAMP
  {
    const int on = 1;
    if (setsockopt( m->sock6, SOL_SOCKET, SO_TIMESTAMP, &on, sizeof(on) ) != 0)
    {
      LS_ERROR(err, -errno);
      return false;
    }
  }
#else
    #pragma message "No SO_TIMESTAMP.  Receive timers won't work."
#endif

  {
    int flags = fcntl(m->sock6, F_GETFL, O_NONBLOCK);
    if (flags == -1)
    {
      LS_ERROR(err, -errno);
      return false;
    }

    if (fcntl(m->sock6, F_SETFL, flags | O_NONBLOCK) == -1)
    {
      LS_ERROR(err, -errno);
      return false;
    }
  }
  m->max_fd = MAX(m->max_fd, m->sock6);

  m->sock4 = socket(PF_INET, SOCK_DGRAM, 0);
  if (m->sock4 < 0)
  {
    LS_ERROR(err, -errno);
    return false;
  }

  {
    struct sockaddr_in addr;
    ls_sockaddr_v4_any(&addr, port);
    if (bind( m->sock4,
              (struct sockaddr*)&addr,
              ls_sockaddr_get_length( (struct sockaddr*)&addr ) ) != 0)
    {
      LS_ERROR(err, -errno);
      return false;
    }
  }

#ifdef IP_PKTINFO
  {
    const int on = 1;
    if (setsockopt( m->sock4, IPPROTO_IP, IP_PKTINFO, &on, sizeof(on) ) != 0)
    {
      LS_ERROR(err, -errno);
      return false;
    }
  }
#else
    #pragma message "No IP_PKTINFO.  Destination addresses won't work."
#endif

#ifdef SO_TIMESTAMP
  {
    const int on = 1;
    if (setsockopt( m->sock4, SOL_SOCKET, SO_TIMESTAMP, &on, sizeof(on) ) != 0)
    {
      LS_ERROR(err, -errno);
      return false;
    }
  }
#else
    #pragma message "No SO_TIMESTAMP.  Receive timers won't work."
#endif

  {
    int flags = fcntl(m->sock4, F_GETFL, O_NONBLOCK);
    if (flags == -1)
    {
      LS_ERROR(err, -errno);
      return false;
    }

    if (fcntl(m->sock4, F_SETFL, flags | O_NONBLOCK) == -1)
    {
      LS_ERROR(err, -errno);
      return false;
    }
  }

  m->max_fd = MAX(m->max_fd, m->sock4);
  return true;
}

LS_API bool
tube_manager_bind_event(tube_manager*            mgr,
                        const char*              name,
                        ls_event_notify_callback cb,
                        ls_err*                  err)
{
  ls_event*            ev;
  ls_event_dispatcher* dispatcher = _tube_manager_get_dispatcher(mgr);
  assert(name);
  assert(cb);

  ev = ls_event_dispatcher_get_event(dispatcher, name);
  if (!ev)
  {
    LS_ERROR(err, LS_ERR_BAD_FORMAT);
    return false;
  }

  return ls_event_bind(ev, cb, mgr, err);
}

static void
clean_tube(bool  replace,
           bool  destroy_key,
           void* key,
           void* data)
{
  tube*  t = data;
  ls_err err;
  UNUSED_PARAM(replace);
  UNUSED_PARAM(destroy_key);
  UNUSED_PARAM(key);

  if (tube_get_state(t) == TS_RUNNING)
  {
    if ( !tube_close(t, &err) )
    {
      LS_LOG_ERR(err, "tube_close");
      /* keep going! */
    }
  }
  tube_destroy(t);
}

LS_API bool
tube_manager_add(tube_manager* mgr,
                 tube*         t,
                 ls_err*       err)
{
  spud_tube_id* id;
  assert(mgr);
  assert(t);

  tube_get_id(t, &id);
  if ( !ls_htable_put(mgr->tubes, id, t, clean_tube, err) )
  {
    return false;
  }
  return ls_event_trigger(mgr->e_add, t, NULL, NULL, err);
}

LS_API void
tube_manager_remove(tube_manager* mgr,
                    tube*         t)
{
  ls_err        err;
  spud_tube_id* id;
  assert(mgr);
  assert(t);
  if ( !ls_event_trigger(mgr->e_remove, t, NULL, NULL, &err) )
  {
    LS_LOG_ERR(err, "ls_event_trigger");
    /* keep going! */
  }

  tube_get_id(t, &id);
  ls_htable_remove(mgr->tubes, id);
}

LS_API bool
tube_manager_interrupt(tube_manager* mgr,
                       char          b,
                       ls_err*       err)
{
  int pipe_w = mgr->pipe[1];
  int e;

  do
  {
    switch ( write( pipe_w, &b, sizeof(b) ) )
    {
    case -1:
      e = errno;
      switch (e)
      {
      case EAGAIN:                /* EGAIN == EWOULDBLOCK */
        /* TODO: set timer? */
        continue;
      default:
        LS_ERROR(err, -e);
        return false;
      }
      break;
    case 0:
      /* This shouldn't be able to happen? */
      assert(false);
      continue;
    default:
      return true;
    }
  } while (true);
}

LS_API bool
tube_manager_cancel_timer(tube_manager* mgr,
                          ls_timer*     tim,
                          ls_err*       err)
{
  assert(mgr);
  ls_timer_cancel(tim);

  if (pthread_mutex_lock(&mgr->lock) != 0)
  {
    LS_ERROR(err, -errno);
    return false;
  }

  /* TODO: no need to do a full heapwalk here.  Should be able to just */
  /* sift this one up to the top. */
  gheap_make_heap(mgr->timer_q->ctx, mgr->timer_q->base, mgr->timer_q->size);

  /* always unlock, if lock worked */
  if (pthread_mutex_unlock(&mgr->lock) != 0)
  {
    LS_ERROR(err, -errno);
    return false;
  }
  return true;
}

LS_API bool
tube_manager_schedule_timer(tube_manager* mgr,
                            ls_timer*     tim,
                            ls_err*       err)
{
  bool ret = true;
  assert(mgr);
  assert(tim);

  if (pthread_mutex_lock(&mgr->lock) != 0)
  {
    LS_ERROR(err, -errno);
    return false;
  }
  if ( !gpriority_queue_push(mgr->timer_q, &tim) )
  {
    LS_ERROR(err, LS_ERR_NO_MEMORY);
    ret = false;
  }
  /* always unlock, if lock worked */
  if (pthread_mutex_unlock(&mgr->lock) != 0)
  {
    LS_ERROR(err, -errno);
    ret = false;
  }
  return ret;
}

LS_API bool
tube_manager_schedule_ms(tube_manager* mgr,
                         unsigned long ms,
                         ls_timer_func cb,
                         void*         context,
                         ls_timer**    tim,
                         ls_err*       err)
{
  ls_timer* ret;
  assert(mgr);

  if ( !ls_timer_create_ms(&mgr->last, ms, cb, context, &ret, err) )
  {
    return false;
  }
  if ( !tube_manager_schedule_timer(mgr, ret, err) )
  {
    return false;
  }
  if (tim)
  {
    *tim = ret;
  }
  return true;
}

LS_API bool
tube_manager_schedule(tube_manager*   mgr,
                      struct timeval* tv,
                      ls_timer_func   cb,
                      void*           context,
                      ls_timer**      tim,
                      ls_err*         err)
{
  ls_timer* ret;
  assert(mgr);

  if ( !ls_timer_create(tv, cb, context, &ret, err) )
  {
    return false;
  }
  if ( !tube_manager_schedule_timer(mgr, ret, err) )
  {
    return false;
  }
  if (tim)
  {
    *tim = ret;
  }
  return true;
}

static void
_tm_signal(int sig)
{
  ls_err       err;
  sig_context* c = sig_contexts;
  while (c)
  {
    if (c->sig == sig)
    {
      if ( !tube_manager_interrupt(c->mgr, sig, &err) )
      {
        LS_LOG_ERR(err, "tube_manager_interrupt");
      }
    }
    c = c->next;
  }
}

LS_API bool
tube_manager_signal(tube_manager* mgr,
                    int           sig,
                    sig_t         cb,
                    ls_err*       err)
{
  sig_context* ctx = ls_data_malloc( sizeof(*ctx) );
  if (!ctx)
  {
    LS_ERROR(err, LS_ERR_NO_MEMORY);
    return false;
  }
  if (signal(sig, _tm_signal) == SIG_ERR)
  {
    LS_ERROR(err, -errno);
    ls_data_free(ctx);
    return false;
  }
  ctx->mgr     = mgr;
  ctx->sig     = sig;
  ctx->cb      = cb;
  ctx->next    = sig_contexts;
  sig_contexts = ctx;
  return true;
}

static int
pending_timers(tube_manager*    mgr,
               struct timeval** tv,
               ls_err*          err)
{
  /* returns: */
  /*   -1 on error */
  /*   0 with no pending timers */
  /*   1 with tv filled out */
  int        ret = -2;
  ls_timer** tim;

  /* while there are still timeouts to process */
  while ( (ret == -2) && mgr->keep_going )
  {
    if (pthread_mutex_lock(&mgr->lock) != 0)
    {
      LS_ERROR(err, -errno);
      ret = -1;
      break;
    }

    /* make sure to copy everything we need out of the tcb while we're locked */
    tim = (ls_timer**)gpriority_queue_top(mgr->timer_q);
    if (!tim)
    {
      ret = 0;
    }
    else
    {
      if ( ls_timer_greater_tv(*tim, &mgr->last) )
      {
        *tv = ls_timer_get_time(*tim);
        ret = 1;
        tim = NULL;
      }
      else
      {
        gpriority_queue_pop(mgr->timer_q);
        /* keep going */
      }
    }

    if (pthread_mutex_unlock(&mgr->lock) != 0)
    {
      LS_ERROR(err, -errno);
      ret = -1;
      break;
    }

    if (tim)
    {
      ls_timer_exec(*tim);
    }
  }
  return ret;
}

static int
tube_manager_wait(tube_manager* mgr,
                  ls_err*       err)
{
  /* TODO: make this use a better mechanism than select on your OS. */
  int             pipe_r = mgr->pipe[0];
  char            b;
  int             e;
  int             pending;
  struct timeval  timeout;
  struct timeval* term;
  fd_set          reads;
  FD_ZERO(&reads);

  /* While there are no sockets ready to read */
  while (mgr->keep_going)
  {
    pending = pending_timers(mgr, &term, err);
    if (pending < 0)
    {
      return pending;
    }

    if (mgr->sock6 >= 0)
    {
      FD_SET(mgr->sock6, &reads);
    }
    if (mgr->sock4 >= 0)
    {
      FD_SET(mgr->sock4, &reads);
    }
    FD_SET(pipe_r, &reads);
    if (pending)
    {
      timersub(term, &mgr->last, &timeout);
    }
    switch ( select(mgr->max_fd + 1,
                    &reads, NULL, NULL,
                    pending ? &timeout : NULL) )
    {
    case -1:
      e = errno;
      switch (e)
      {
      case EINTR:
        continue;
      default:
        LS_ERROR(err, -e);
        return -1;
      }
      break;
    case 0:
      /* ls_log(LS_LOG_INFO, "count: %zu (%zd.%06d)", */
      /*        gpriority_queue_size(mgr->timer_q), */
      /*        timeout.tv_sec, */
      /*        timeout.tv_usec); */

      /* timeout */
      if (gettimeofday(&mgr->last, NULL) != 0)
      {
        LS_ERROR(err, -errno);
        return -1;
      }
      /* assert(term); */
      /* mgr->last = *term; */
      /* ls_log_format_timeval(&mgr->last, "last"); */
      continue;
    default:
      if ( FD_ISSET(pipe_r, &reads) )
      {
        /* this is non-blocking. */
        if ( (read(pipe_r, &b, 1) == 1) )             /* && (b > 0)) { */
        {
          sig_context* c = sig_contexts;
          while (c)
          {
            if (c->sig == b)
            {
              c->cb(b);
            }
            c = c->next;
          }
        }
      }
      /* TODO: make sure v4 doesn't starve */
      /* select is level-triggered */
      if ( (mgr->sock6 >= 0) && FD_ISSET(mgr->sock6, &reads) )
      {
        return mgr->sock6;
      }
      if ( (mgr->sock4 >= 0) && FD_ISSET(mgr->sock4, &reads) )
      {
        return mgr->sock4;
      }
      break;
    }
  }
  return -2;
}

LS_API bool
tube_manager_loop(tube_manager* mgr,
                  ls_err*       err)
{
  struct msghdr           hdr;
  struct sockaddr_storage their_addr;
  struct iovec            iov[1];
  ssize_t                 numbytes;
  uint8_t                 buf[MAXBUFLEN];
  char                    id_str[SPUD_ID_STRING_SIZE + 1];
  spud_message            msg = {NULL, NULL};
  spud_tube_id            uid;
  spud_command            cmd;
  tube_event_data         d;
  bool                    got_time;
  struct cmsghdr*         cmsg;
  uint8_t                 mctl[sizeof(struct cmsghdr) +
                               sizeof(struct in6_pktinfo) +
                               sizeof(struct in_pktinfo) +
                               16]; /* TODO: measure this on some other OS's to
                                     * see if it's enough */
  ls_pktinfo*   info;
  int           sock;
  tube_states_t state;

  assert(mgr);

  if ( !ls_pktinfo_create(&info, err) )
  {
    return false;
  }

  hdr.msg_name    = &their_addr;
  hdr.msg_iov     = iov;
  hdr.msg_iovlen  = 1;
  hdr.msg_control = mctl;

  iov[0].iov_base = buf;
  iov[0].iov_len  = sizeof(buf);
  d.peer          = (const struct sockaddr*)&their_addr;

  if ( !ls_event_trigger(mgr->e_loopstart, mgr, NULL, NULL, err) )
  {
    goto error;
  }

  while (mgr->keep_going)
  {
    hdr.msg_namelen    = sizeof(their_addr);
    hdr.msg_controllen = sizeof(mctl);
    hdr.msg_flags      = 0;
    ls_pktinfo_clear(info);
    got_time = false;

    sock = tube_manager_wait(mgr, err);
    switch (sock)
    {
    case -1:
      goto error;
    case -2:
      goto cleanup;
    default:
      break;
    }
    if ( ( numbytes = _recvmsg_func(sock, &hdr, 0) ) == -1 )
    {
      if (errno == EINTR)
      {
        continue;
      }
      /* unrecoverable */
      LS_ERROR(err, -errno);
      goto error;
    }

    /* recvmsg should only return 0 on TCP EOF */
    assert(numbytes != 0);

    for ( cmsg = CMSG_FIRSTHDR(&hdr); cmsg; cmsg = CMSG_NXTHDR(&hdr, cmsg) )
    {
      if ( (cmsg->cmsg_level == IPPROTO_IPV6) &&
           (cmsg->cmsg_type == IPV6_PKTINFO) )
      {
        ls_pktinfo_set6( info, (struct in6_pktinfo*)CMSG_DATA(cmsg) );
      }
      if ( (cmsg->cmsg_level == IPPROTO_IP) && (cmsg->cmsg_type == IP_PKTINFO) )
      {
        ls_pktinfo_set4( info, (struct in_pktinfo*)CMSG_DATA(cmsg) );
      }
      else if ( (cmsg->cmsg_level == SOL_SOCKET) &&
                (cmsg->cmsg_type == SCM_TIMESTAMP) &&
                ( cmsg->cmsg_len == CMSG_LEN( sizeof(struct timeval) ) ) )
      {
        memcpy( &mgr->last, CMSG_DATA(cmsg), sizeof(struct timeval) );
        got_time = true;
      }
    }
    if (!got_time)
    {
      if (gettimeofday(&mgr->last, NULL) == -1)
      {
        LS_ERROR(err, -errno);
        goto error;
      }
    }

    if ( !spud_parse(buf, numbytes, &msg, err) )
    {
      /* it's an attack.  Move along. */
      LS_LOG_ERR(*err, "spud_parse");
      goto cleanup;
    }

    spud_copy_id(&msg.header->tube_id, &uid);

    cmd    = msg.header->flags & SPUD_COMMAND;
    d.t    = ls_htable_get(mgr->tubes, &uid);
    d.tmgr = mgr;
    d.cbor = msg.cbor;
    if (!d.t)
    {
      if ( !tube_manager_is_responder(mgr) || (cmd != SPUD_OPEN) )
      {
        /* Not for one of our tubes, and we're not a responder, so punt. */
        /* Even if we're a responder, if we get anything but an open */
        /* for an unknown tube, ignore it. */
        ls_log( LS_LOG_WARN, "Invalid tube ID: %s",
                spud_id_to_string(id_str, sizeof(id_str), &uid) );
        goto cleanup;
      }

      /* get started */
      if ( !tube_create(&d.t, err) )
      {
        /* probably out of memory */
        /* TODO: replace with an unused queue */
        goto error;
      }

      tube_set_info(d.t, sock, (struct sockaddr*)&their_addr, &uid);

      if ( !tube_set_local(d.t, info, err) )
      {
        goto error;
      }

      if ( !tube_manager_add(mgr, d.t, err) )
      {
        goto error;
      }

      tube_set_state(d.t, TS_RUNNING);

      if ( !tube_send(d.t, SPUD_ACK, false, false, NULL, 0, 0, err) )
      {
        goto cleanup;
      }
    }

    state = tube_get_state(d.t);
    switch (cmd)
    {
    case SPUD_DATA:
      if (state == TS_RUNNING)
      {
        if ( !ls_event_trigger(mgr->e_data, &d, NULL, NULL, err) )
        {
          goto error;
        }
      }
      break;
    case SPUD_CLOSE:
      if (state != TS_UNKNOWN)
      {
        /* double-close is a no-op */
        tube_set_state(d.t, TS_UNKNOWN);
        if ( !ls_event_trigger(mgr->e_close, &d, NULL, NULL, err) )
        {
          goto error;
        }
        tube_manager_remove(mgr, d.t);
      }
      break;
    case SPUD_OPEN:
      /* Double open.  no-op. */
      break;
    case SPUD_ACK:
      if (state == TS_OPENING)
      {
        tube_set_state(d.t, TS_RUNNING);
        if ( !ls_event_trigger(mgr->e_running, &d, NULL, NULL, err) )
        {
          goto error;
        }
      }
      break;
    }
cleanup:
    spud_unparse(&msg);
  }
  ls_pktinfo_destroy(info);
  return true;
error:
  ls_pktinfo_destroy(info);
  return false;
}

LS_API bool
tube_manager_running(tube_manager* mgr)
{
  assert(mgr);
  return mgr->keep_going;
}

LS_API bool
tube_manager_stop(tube_manager* mgr,
                  ls_err*       err)
{
  assert(mgr);
  mgr->keep_going = false;
  return tube_manager_interrupt(mgr, -2, err);
}

LS_API size_t
tube_manager_size(tube_manager* mgr)
{
  assert(mgr);
  return ls_htable_get_count(mgr->tubes);
}

LS_API void
tube_manager_set_policy_responder(tube_manager* mgr,
                                  bool          will_respond)
{
  assert(mgr);
  if (will_respond)
  {
    mgr->policy |= TP_WILL_RESPOND;
  }
  else
  {
    mgr->policy &= ~TP_WILL_RESPOND;
  }
}

LS_API bool
tube_manager_is_responder(tube_manager* mgr)
{
  return (mgr->policy & TP_WILL_RESPOND) == TP_WILL_RESPOND;
}

LS_API bool
tube_manager_open_tube(tube_manager*          mgr,
                       const struct sockaddr* dest,
                       tube**                 t,
                       ls_err*                err)
{
  tube*        ret = NULL;
  spud_tube_id id;

  if ( !tube_create(&ret, err) )
  {
    return false;
  }

  if ( !spud_create_id(&id, err) )
  {
    tube_destroy(ret);
    return false;
  }

  switch (dest->sa_family)
  {
  case AF_INET:
    tube_set_info(ret, mgr->sock4, dest, &id);
    break;
  case AF_INET6:
    tube_set_info(ret, mgr->sock6, dest, &id);
    break;
  default:
    LS_ERROR(err, LS_ERR_INVALID_ARG);
    tube_destroy(ret);
    return false;
  }

  tube_set_state(ret, TS_OPENING);

  if ( !tube_manager_add(mgr, ret, err) )
  {
    tube_destroy(ret);
    return false;
  }

  if ( !tube_send(ret, SPUD_OPEN, false, false, NULL, 0, 0, err) )
  {
    tube_manager_remove(mgr,ret);
    tube_destroy(ret);
    return false;
  }
  *t = ret;
  return true;
}

LS_API void
tube_manager_set_socket_functions(tube_sendmsg_func send,
                                  tube_recvmsg_func recv)
{
  _sendmsg_func = (send == NULL) ? sendmsg : send;
  _recvmsg_func = (recv == NULL) ? recvmsg : recv;
}

LS_API bool
tube_manager_sendmsg(int              sock,
                     ls_pktinfo*      source,
                     struct sockaddr* dest,
                     struct iovec*    iov,
                     size_t           count,
                     ls_err*          err)
{
  char          msg_control[1024];
  struct msghdr msg;

  assert(dest);
  assert(iov);
  assert(count > 0);

  memset( &msg, 0, sizeof(msg) );
  msg.msg_name    = dest;
  msg.msg_namelen = ls_sockaddr_get_length(dest);
  msg.msg_iov     = iov;
  msg.msg_iovlen  = count;

  if (source)
  {
    memset( msg_control, 0, sizeof(msg_control) );
    msg.msg_control    = msg_control;
    msg.msg_controllen = sizeof(msg_control);
    msg.msg_controllen = ls_pktinfo_cmsg( source, CMSG_FIRSTHDR(&msg) );
  }

  if (_sendmsg_func(sock, &msg, 0) <= 0)
  {
    LS_ERROR(err, -errno)
    return false;
  }
  return true;
}

static int
log_walk(void*       user_data,
         const void* key,
         void*       data)
{
  const spud_tube_id* kid = key;
  char                buf[SPUD_ID_STRING_SIZE + 1];
  UNUSED_PARAM(user_data);
  UNUSED_PARAM(data);

  ls_log( LS_LOG_INFO, "%s", spud_id_to_string(buf, sizeof(buf), kid) );
  return 1;
}

LS_API void
tube_manager_print_tubes(tube_manager* mgr)
{
  assert(mgr);
  ls_htable_walk(mgr->tubes, log_walk, mgr);
}

LS_API void
tube_manager_foreach(tube_manager*    mgr,
                     tube_walker_func walker,
                     void*            data)
{
  assert(mgr);
  assert(walker);

  ls_htable_walk(mgr->tubes, (ls_htable_walkfunc) walker, data);
}
