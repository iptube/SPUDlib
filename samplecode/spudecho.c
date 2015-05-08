/*
 * Copyright (c) 2015 SPUDlib authors.  See LICENSE file.
 */

#include <errno.h>
#include <stdio.h>
#include <strings.h>
#include <string.h>
#include <stdlib.h>

#include "spud.h"
#include "tube_manager.h"
#include "ls_htable.h"
#include "ls_log.h"
#include "ls_sockaddr.h"

#define MYPORT 1402    // the port users will be connecting to
#define MAXBUFLEN 2048
#define MAX_LISTEN_SOCKETS 10

tube_manager *mgr = NULL;

typedef struct _delay_write_t {
    tube *t;
    uint8_t *buf;
    size_t length;
} delay_write_t;

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
    ls_err err;
    ls_log(LS_LOG_INFO, "Quitting...");
    if (!tube_manager_stop(mgr, &err)) {
        LS_LOG_ERR(err, "tube_manager_stop");
    }
}

void print_stats()
{
    ls_log(LS_LOG_INFO, "Tube count: %zu", tube_manager_size(mgr));
}

static void read_delay(const struct timeval *now, const void *ctx)
{
    delay_write_t *dw = (delay_write_t*)ctx;
    ls_err err;
    context_t *c = tube_get_data(dw->t);

    UNUSED_PARAM(now);

    if (!tube_data(dw->t, dw->buf, dw->length, &err)) {
        LS_LOG_ERR(err, "tube_data");
    }
    ls_data_free(dw->buf);
    ls_data_free(dw);
    c->count++;
}

static void read_cb(ls_event_data evt,
                    void *arg)
{
    tube_event_data *td = evt->data;
    delay_write_t *dw;
    ls_err err;
    UNUSED_PARAM(arg);

    dw = ls_data_calloc(1, sizeof(*dw));
    if (!dw) {
        ls_log(LS_LOG_ERROR, "%s:%d ls_data_malloc", __FILE__, __LINE__);
        return;
    }
    dw->t = td->t;
    if (td->cbor) {
        const cn_cbor *cp = cn_cbor_mapget_int(td->cbor, 0);
        if (cp) {
            dw->length = cp->length;
            dw->buf = ls_data_malloc(cp->length);
            if (!dw->buf)
            {
                ls_log(LS_LOG_ERROR, "ls_data_malloc");
                ls_data_free(dw);
                return;
            }
            memcpy(dw->buf, cp->v.str, dw->length);
            ls_log(LS_LOG_VERBOSE, "Received %*s", (int)dw->length, dw->buf);
        }
    }
    if (!tube_manager_schedule_ms(mgr, 10, read_delay, dw, &err)) {
        LS_LOG_ERR(err, "tube_manager_schedule_ms");
    }
}

static void close_cb(ls_event_data evt,
                     void *arg)
{
    UNUSED_PARAM(arg);
    tube_event_data *td = evt->data;
    context_t *c = tube_get_data(td->t);
    char idStr[SPUD_ID_STRING_SIZE+1];

    ls_log(LS_LOG_VERBOSE,
           "Spud ID: %s CLOSED: %zd data packets",
           tube_id_to_string(td->t, idStr, sizeof(idStr)),
           c->count);
}

static void add_cb(ls_event_data evt,
                   void *arg)
{
    tube *t = evt->data;
    UNUSED_PARAM(arg);
    tube_set_data(t, new_context());
}

static void remove_cb(ls_event_data evt,
                      void *arg)
{
    tube *t = evt->data;
    context_t *c = tube_get_data(t);
    UNUSED_PARAM(arg);
    ls_data_free(c);
}

int main(int argc, char** argv)
{
    ls_err err;

    if (argc > 1 && !strcmp(argv[1], "-v"))
    {
        ls_log_set_level(LS_LOG_VERBOSE);
    }

    if (!tube_manager_create(0, &mgr, &err)) {
        LS_LOG_ERR(err, "tube_manager_create");
        return 1;
    }

    if (!tube_manager_signal(mgr, SIGUSR1, print_stats, &err)) {
        LS_LOG_ERR(err, "tube_manager_signal");
        return 1;
    }

    if (!tube_manager_socket(mgr, MYPORT, &err)) {
      LS_LOG_ERR(err, "tube_manager_socket");
      return 1;
    }

    if (!tube_manager_bind_event(mgr, EV_DATA_NAME, read_cb, &err) ||
        !tube_manager_bind_event(mgr, EV_CLOSE_NAME, close_cb, &err) ||
        !tube_manager_bind_event(mgr, EV_ADD_NAME, add_cb, &err) ||
        !tube_manager_bind_event(mgr, EV_REMOVE_NAME, remove_cb, &err)) {
        LS_LOG_ERR(err, "tube_manager_bind_event");
        return 1;
    }

    ls_log(LS_LOG_INFO, "Listening on port %d", MYPORT);
    if (!tube_manager_loop(mgr, &err)) {
        LS_LOG_ERR(err, "tube_manager_loop");
        return 1;
    }
    return 0;
}
