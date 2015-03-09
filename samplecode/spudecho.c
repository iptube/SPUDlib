#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <strings.h>
#include <sys/socket.h>
#include <signal.h>
#include <string.h>

#include "spud.h"
#include "tube.h"
#include "iphelper.h"
#include "sockethelper.h"
#include "ls_eventing.h"
#include "ls_htable.h"
#include "ls_log.h"
#include "ls_sockaddr.h"

#define MYPORT 1402    // the port users will be connecting to
#define MAXBUFLEN 2048
#define MAX_LISTEN_SOCKETS 10

int sockfd = -1;
ls_htable clients = NULL;
bool keepGoing = true;

typedef struct _context_t {
    size_t count;
} context_t;

context_t *new_context() {
    context_t *c = ls_data_malloc(sizeof(context_t));
    c->count = 0;
    return c;
}

void teardown()
{
    ls_log(LS_LOG_INFO, "Quitting...");
    keepGoing = false;
    // if (sockfd >= 0) {
    //     close(sockfd);
    // }
    // exit(0);
}

void print_stats()
{
    ls_log(LS_LOG_INFO, "Tube count: %d", ls_htable_get_count(clients));
}

static void read_cb(ls_event_data evt,
                    void *arg)
{
    UNUSED_PARAM(arg);
    tubeData *td = evt->data;
    ls_err err;

    if (td->cbor) {
        const cn_cbor *cp = cn_cbor_mapget_int(td->cbor, 0);
        if (cp) {
            // echo
            if (!tube_data(td->t, (uint8_t*)cp->v.str, cp->length, &err)) {
                LS_LOG_ERR(err, "tube_data");
            }
        } else {
            if (!tube_data(td->t, NULL, 0, &err)) {
                LS_LOG_ERR(err, "tube_data");
            }
        }
    }
    ((context_t*)td->t->data)->count++;
}

static void close_cb(ls_event_data evt,
                     void *arg)
{
    UNUSED_PARAM(arg);
    tubeData *td = evt->data;
    context_t *c = td->t->data;
    char idStr[SPUD_ID_STRING_SIZE+1];
    tube old;

    ls_log(LS_LOG_VERBOSE,
           "Spud ID: %s CLOSED: %zd data packets",
           spud_idToString(idStr,
                           sizeof(idStr),
                           &td->t->id, NULL),
           c->count);
    old = ls_htable_remove(clients, &td->t->id);
    if (old != td->t) {
        ls_log(LS_LOG_ERROR, "Invalid state closing tube\n");
    }
    ls_data_free(c);
    tube_destroy(td->t);
    td->t = NULL;
}

static int clean_tube(void *user_data, const void *key, void *data) {
    UNUSED_PARAM(user_data);
    UNUSED_PARAM(key);
    tube t = data;
    context_t *c = t->data;
    ls_data_free(c);
    tube_destroy(t);
    return 1;
}

static int socketListen() {
    struct sockaddr_storage their_addr;
    uint8_t buf[MAXBUFLEN];
    char idStr[SPUD_ID_STRING_SIZE+1];
    socklen_t addr_len = sizeof(their_addr);
    int numbytes;
    tube t;
    spud_message_t sMsg = {NULL, NULL};
    ls_err err;
    spud_tube_id_t uid;
    int ret = 0;
    ls_event_dispatcher dispatcher;

    if (!ls_event_dispatcher_create(&sMsg, &dispatcher, &err)) {
        goto error;
    }
    if (!tube_bind_events(dispatcher,
                          NULL, read_cb, close_cb,
                          &sMsg, &err)) {
        goto error;
    }

    while (keepGoing) {
        addr_len = sizeof(their_addr);
        if ((numbytes = recvfrom(sockfd, buf,
                                 MAXBUFLEN , 0,
                                 (struct sockaddr *)&their_addr,
                                 &addr_len)) == -1) {
            if (errno == EINTR) {
                break;
            }
            LS_LOG_PERROR("recvfrom (data)");
            ret = 1;
            goto error;
        }

        if (!spud_parse(buf, numbytes, &sMsg, &err)) {
            // it's an attack.  Move along.
            LS_LOG_ERR(err, "spud_parse");
            goto cleanup;
        }

        if (!spud_copyId(&sMsg.header->tube_id, &uid, &err)) {
            LS_LOG_ERR(err, "spud_copyId");
            goto cleanup;
        }

        t = (tube)ls_htable_get(clients, &uid);
        if (!t) {
            // get started
            if (!tube_create(sockfd, dispatcher, &t, &err)) {
                LS_LOG_ERR(err, "tube_create");
                ret = 1; // TODO: replace with an unused queue
                goto error;
            }
            t->data = new_context();
            if (!ls_htable_put(clients, &uid, t, NULL, &err)) {
                LS_LOG_ERR(err, "ls_htable_put");
                ret = 1;
                goto error;
            }

            ls_log(LS_LOG_VERBOSE, "Spud ID: %s created",
                   spud_idToString(idStr, sizeof(idStr), &uid, NULL));

        }
        if (!tube_recv(t, &sMsg, (struct sockaddr *)&their_addr, &err)) {
            LS_LOG_ERR(err, "tube_recv");
            // ignore error?
        }
cleanup:
        spud_unparse(&sMsg);
    }
error:
    ls_htable_clear(clients, clean_tube, NULL);
    ls_htable_destroy(clients);
    return ret;
}

unsigned int hash_id(const void *id) {
    // treat the 8 bytes of tube ID like a long long.
    uint64_t key = *(uint64_t *)id;

    // from
    // https://gist.github.com/badboy/6267743#64-bit-to-32-bit-hash-functions
    key = (~key) + (key << 18);
    key = key ^ (key >> 31);
    key = key * 21;
    key = key ^ (key >> 11);
    key = key + (key << 6);
    key = key ^ (key >> 22);
    return (unsigned int) key;
}

int compare_id(const void *key1, const void *key2) {
    int ret = 0;
    uint64_t k1 = *(uint64_t *)key1;
    uint64_t k2 = *(uint64_t *)key2;
    if (k1<k2) {
        ret = -1;
    } else {
        ret = (k1==k2) ? 0 : 1;
    }
    return ret;
}

int main(void)
{
    struct sockaddr_in6 servaddr;
    ls_err err;

    signal(SIGUSR1, print_stats);

    // 65521 is max prime under 65535, which seems like an interesting
    // starting point for scale.
    if (!ls_htable_create(65521, hash_id, compare_id, &clients, &err)) {
        LS_LOG_ERR(err, ls_htable_create);
        return 1;
    }

    sockfd = socket(PF_INET6, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        LS_LOG_PERROR("socket");
        return 1;
    }
    sockaddr_initAsIPv6Any(&servaddr, MYPORT);
    if (bind(sockfd,
             (struct sockaddr*)&servaddr,
             ls_sockaddr_get_length((struct sockaddr*)&servaddr)) != 0) {
        LS_LOG_PERROR("bind");
        return 1;
    }

    ls_log(LS_LOG_INFO, "Listening on port %d", MYPORT);
    return socketListen();
}
