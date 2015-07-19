/**
 * \file
 * \brief
 * Tube manager typedefs and common functions.
 * private, not for use outside library and unit tests.
 * \see tube_manager.h
 *
 * Copyright (c) 2015 SPUDlib authors.  See LICENSE file.
 */
 
#include "tube_manager.h"
#include "ls_eventing.h"
#include "ls_htable.h"

struct _tube_manager
{
    bool initialized;
    int sock4;
    int sock6;
    int pipe[2];
    int max_fd;
    ls_htable *tubes;
    ls_event_dispatcher *dispatcher;
    struct gpriority_queue *timer_q;
    struct timeval last;
    pthread_mutex_t lock;
    ls_event *e_loopstart;
    ls_event *e_running;
    ls_event *e_data;
    ls_event *e_close;
    ls_event *e_add;
    ls_event *e_remove;
    tube_policies policy;
    bool keep_going;
    void *data;
};

/**
 * Initializes a pre-allocated tube manager. Useful for subclasses.
 *
 * \invariant m != NULL
 * \param[in] m The tube manager to initialize
 * \param[in] buckets The tubes hashtable bucket size
 * \param[out] err If non-NULL on input, describes error if false is returned
 * \return true: m in initialized.  false: see err.
 */
bool _tube_manager_init(tube_manager *m,
                        int buckets,
                        ls_err *err);

/**
 * Finalizes a tube manager.  Useful for subclasses.
 *
 * \invariant m != NULL
 * \param[in] mgr The tube manager to finalize
 * \param[in] buckets The tubes hashtable bucket size
 * \param[out] err If non-NULL on input, describes error if false is returned
 * \return true: m in initialized.  false: see err.
 */
void _tube_manager_finalize(tube_manager *mgr);

/**
 * Get the tube manager's event dispatcher. Useful for subclasses.
 */
ls_event_dispatcher *_tube_manager_get_dispatcher(tube_manager *mgr);
