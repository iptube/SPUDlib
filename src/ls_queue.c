/**
 * Copyrights
 *
 * Portions created or assigned to Cisco Systems, Inc. are
 * Copyright (c) 2010 Cisco Systems, Inc.  All Rights Reserved.
 */

#include "ls_queue.h"
#include "ls_mem.h"

#include <assert.h>
#include <unistd.h>

typedef struct _ls_qnode ls_qnode;

struct _ls_qnode
{
  ls_qnode* next;
  void*     value;
};

struct _ls_queue
{
  ls_qnode*          head;
  ls_qnode*          tail;
  ls_queue_cleanfunc cleaner;
};

LS_API bool
ls_queue_create(ls_queue_cleanfunc cleaner,
                ls_queue**         q,
                ls_err*            err)
{
  ls_queue* ret;

  assert(q);

  ret = ls_data_calloc( 1, sizeof(struct _ls_queue) );
  if (!ret)
  {
    LS_ERROR(err, LS_ERR_NO_MEMORY);
    return false;
  }

  ret->head    = ret->tail = NULL;
  ret->cleaner = cleaner;
  *q           = ret;
  return true;
}

LS_API bool
ls_queue_enq(ls_queue* q,
             void*     value,
             ls_err*   err)
{
  ls_qnode* n = NULL;
  assert(q);
  assert(value);
  n = ls_data_calloc( 1, sizeof(ls_qnode) );
  if (!n)
  {
    LS_ERROR(err, LS_ERR_NO_MEMORY);
    return false;
  }
  n->value = value;
  if (q->tail)
  {
    q->tail->next = n;
    q->tail       = n;
  }
  else
  {
    q->head = q->tail = n;
  }
  return true;
}

static ls_qnode*
deq(ls_queue* q)
{
  ls_qnode* ret = q->head;
  if (ret)
  {
    q->head = ret->next;
    if (!q->head)
    {
      q->tail = NULL;
    }
  }
  return ret;
}

LS_API void*
ls_queue_deq(ls_queue* q)
{
  ls_qnode* n;
  void*     ret = NULL;
  assert(q);
  n = deq(q);
  if (n)
  {
    ret = n->value;
    ls_data_free(n);
  }
  return ret;
}

LS_API void
ls_queue_destroy(ls_queue* q)
{
  ls_qnode* n;
  assert(q);

  while ( ( n = deq(q) ) )
  {
    if (q->cleaner)
    {
      q->cleaner(q, n->value);
    }
    ls_data_free(n);
  }
  ls_data_free(q);
}
