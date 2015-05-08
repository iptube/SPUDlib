/*
 * Copyright (c) 2015 SPUDlib authors.  See LICENSE file.
 */

#include <stdio.h>

#include "tube_manager.h"
#include "ls_log.h"

#define MYPORT 1403
tube_manager *mgr = NULL;

void after(const struct timeval *actual, const void *context)
{
    UNUSED_PARAM(actual);
    UNUSED_PARAM(context);
    ls_log(LS_LOG_ERROR, "after");
}

void sg(int sig)
{
    ls_log(LS_LOG_ERROR, "sig: %d", sig);
}

int main()
{
    ls_err err;
    if (!tube_manager_create(0, &mgr, &err)) {
        LS_LOG_ERR(err, "tube_manager_create");
        return 1;
    }
    // if (!tube_manager_socket(mgr, MYPORT, &err)) {
    //   LS_LOG_ERR(err, "tube_manager_socket");
    //   return 1;
    // }

    if (!tube_manager_signal(mgr, SIGUSR1, sg, &err)) {
        LS_LOG_ERR(err, "tube_manager_signal");
        return 1;
    }

    if (!tube_manager_schedule_ms(mgr, 5000, after, NULL, &err)) {
        LS_LOG_ERR(err, "tube_manager_schedule_ms");
        return 1;
    }

    if (!tube_manager_loop(mgr, &err)) {
        LS_LOG_ERR(err, "tube_manager_loop");
        return 1;
    }
    return 0;
}
