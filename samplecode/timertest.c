/*
 * Copyright (c) 2015 SPUDlib authors.  See LICENSE file.
 */

#include <stdio.h>

#include "tube_manager.h"
#include "ls_log.h"

#define MYPORT 1403
tube_manager* mgr = NULL;

void
after(ls_timer* tim)
{
  UNUSED_PARAM(tim);
  ls_log(LS_LOG_ERROR, "after");
  tube_manager_stop(mgr, NULL);
}

void
sg(int sig)
{
  ls_log(LS_LOG_ERROR, "sig: %d", sig);
}

void
startup(ls_event_data* evt,
        void*          arg)
{
  ls_err err;
  UNUSED_PARAM(evt);
  UNUSED_PARAM(arg);
  if ( !tube_manager_schedule_ms(mgr, 100, after, NULL, NULL, &err) )
  {
    LS_LOG_ERR(err, "tube_manager_schedule_ms");
  }
}

int
main()
{
  ls_err err;
  if ( !tube_manager_create(0, &mgr, &err) )
  {
    LS_LOG_ERR(err, "tube_manager_create");
    return 1;
  }

  if ( !tube_manager_signal(mgr, SIGUSR1, sg, &err) )
  {
    LS_LOG_ERR(err, "tube_manager_signal");
    return 1;
  }

  if ( !tube_manager_bind_event(mgr, EV_LOOPSTART_NAME, startup, &err) )
  {
    LS_LOG_ERR(err, "tube_manager_bind_event");
    return 1;
  }

  if ( !tube_manager_loop(mgr, &err) )
  {
    LS_LOG_ERR(err, "tube_manager_loop");
    return 1;
  }

  tube_manager_destroy(mgr);

  return 0;
}
