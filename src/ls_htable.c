/**
 * Copyrights
 *
 * Portions created or assigned to Cisco Systems, Inc. are
 * Copyright (c) 2010 Cisco Systems, Inc.  All Rights Reserved.
 */

#include "ls_htable.h"
#include "ls_mem.h"

#include <assert.h>
#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


/*****************************************************************************
 * Internal type definitions
 */

#define HASH_NUM_BUCKETS 509 /* should be a prime number; see Knuth */

struct _ls_hnode
{
  struct _ls_hnode*   next; /* next node in list */
  const void*         key;  /* key pointer */
  void*               value; /* value pointer */
  int                 bucket;
  unsigned int        khash;
  ls_htable_cleanfunc cleaner;
};

struct _ls_htable
{
  ls_htable_hashfunc hash;      /* hash function */
  ls_htable_cmpfunc  cmp;       /* comparison function */
  unsigned int       count;     /* table entry count */
  unsigned int       bcount;    /* bucket count */
  unsigned int       resize_count; /* number of time resized */
  ls_hnode**         buckets;    /* the hash buckets */
};

#define _hash_key(tb, key)             ( ( *( (tb)->hash ) )(key) )
#define _bucket_from_khash(tb, khash)  ( (khash) % ( (tb)->bcount ) )

/****************************************************************************
 * Internal functions
 */

/**
 * walks a hash bucket to find a node whose key matches the named key value.
 * Returns the node pointer, or NULL if it's not found.
 */
static ls_hnode*
_find_node(ls_htable*   tab,
           const void*  key,
           int          bucket,
           unsigned int khash)
{
  register ls_hnode* p;    /* search pointer/return from this function */

  if (bucket < 0)
  {
    khash  = _hash_key(tab, key);
    bucket = _bucket_from_khash(tab, khash);
  }

  for (p = tab->buckets[bucket]; p; p = p->next)
  {
    if ( (khash == p->khash) && ( ( *(tab->cmp) )(key, p->key) == 0 ) )
    {
      return p;
    }
  }

  return NULL;
}

static bool
_resize_hashtable(ls_htable*   tab,
                  unsigned int buckets,
                  ls_err*      err)
{
  ls_hnode**   old_buckets = tab->buckets;
  unsigned int old_bcount  = tab->bcount;
  ls_hnode**   new_buckets;
  ls_hnode*    node, * next_node;
  unsigned int c;

  new_buckets = ls_data_calloc( buckets, sizeof(ls_hnode*) );
  if (!new_buckets)
  {
    LS_ERROR(err, LS_ERR_NO_MEMORY);
    return false;
  }

  tab->buckets = new_buckets;
  tab->bcount  = buckets;
  ++tab->resize_count;

  for (c = 0; c < old_bcount; ++c)
  {
    node = old_buckets[c];
    while (node != 0)
    {
      unsigned int bucket = node->khash % tab->bcount;

      next_node            = node->next;
      node->bucket         = bucket;
      node->next           = tab->buckets[bucket];
      tab->buckets[bucket] = node;

      node = next_node;
    }
  }

  ls_data_free(old_buckets);
  return true;
}

LS_API const void*
ls_hnode_get_key(ls_hnode* node)
{
  assert(node);

  return node->key;
}

LS_API void*
ls_hnode_get_value(ls_hnode* node)
{
  assert(node);

  return node->value;
}
LS_API void
ls_hnode_put_value(ls_hnode*           node,
                   void*               data,
                   ls_htable_cleanfunc cleaner)
{
  void*               pvalue;
  ls_htable_cleanfunc pcleaner;

  assert(node);
  pvalue        = node->value;
  pcleaner      = node->cleaner;
  node->value   = data;
  node->cleaner = cleaner;

  if (pvalue && pcleaner)
  {
    pcleaner(true, false, (void*)node->key, pvalue);
  }
}

LS_API void
ls_htable_remove_node(ls_htable* tbl,
                      ls_hnode*  node)
{
  register ls_hnode* p;

  assert(tbl);
  assert(node);

  /* look to unchain it from the bucket it's in */
  if (node == tbl->buckets[node->bucket])
  {
    /* unchain at head */
    tbl->buckets[node->bucket] = node->next;
  }
  else
  {
    /* unchain in middle of list */
    for (p = tbl->buckets[node->bucket]; p->next != node; p = p->next)
    {
    }
    p->next = node->next;
  }

  if (node->cleaner)
  {
    node->cleaner(false, true, (void*)node->key, node->value);
  }
  ls_data_free(node);
  tbl->count--;
}

LS_API bool
ls_htable_create(int                buckets,
                 ls_htable_hashfunc hash,
                 ls_htable_cmpfunc  cmp,
                 ls_htable**        tbl,
                 ls_err*            err)
{
  ls_htable* ret_table;

  assert(tbl);
  assert(hash);
  assert(cmp);

  if (buckets <= 0)
  {
    buckets = HASH_NUM_BUCKETS;
  }

  ret_table = ls_data_calloc( 1, sizeof(struct _ls_htable) );
  if (!ret_table)
  {
    LS_ERROR(err, LS_ERR_NO_MEMORY);
    return false;
  }

  ret_table->buckets = ls_data_calloc( buckets, sizeof(ls_hnode*) );
  if (!ret_table->buckets)
  {
    ls_data_free(ret_table);
    ret_table = NULL;

    LS_ERROR(err, LS_ERR_NO_MEMORY);
    return false;
  }

  /* fill the fields of the hash table */
  ret_table->hash   = hash;
  ret_table->cmp    = cmp;
  ret_table->bcount = buckets;
  *tbl              = ret_table;

  return true;
}

LS_API void
ls_htable_destroy(ls_htable* tbl)
{
  ls_hnode*    cur, * next;
  unsigned int i;

  assert(tbl);

  for (i = 0; i < tbl->bcount; i++)
  {
    cur = tbl->buckets[i];
    while (cur)
    {
      next = cur->next;
      if (cur->cleaner)
      {
        cur->cleaner(false, true, (void*)cur->key, cur->value);
      }
      ls_data_free(cur);
      cur = next;
    }
  }
  ls_data_free(tbl->buckets);
  ls_data_free(tbl);
}

LS_API unsigned int
ls_htable_get_count(ls_htable* tbl)
{
  assert(tbl);
  return tbl->count;
}

LS_API ls_hnode*
ls_htable_get_node(ls_htable*  tbl,
                   const void* key)
{
  ls_hnode* node;

  assert(tbl);

  node = _find_node(tbl, key, -1, 0);
  return node;
}

LS_API void*
ls_htable_get(ls_htable*  tbl,
              const void* key)
{
  ls_hnode* node;

  assert(tbl);

  node = _find_node(tbl, key, -1, 0);
  return node ? node->value : NULL;
}

LS_API bool
ls_htable_put(ls_htable*          tbl,
              const void*         key,
              void*               value,
              ls_htable_cleanfunc cleaner,
              ls_err*             err)
{
  unsigned int khash;
  unsigned int bucket;
  ls_hnode*    node;

  assert(tbl);

  /* compute the hash bucket and try to find an existing node */
  khash  = _hash_key(tbl, key);
  bucket = _bucket_from_khash(tbl, khash);
  node   = _find_node(tbl, key, bucket, khash);
  if (node)
  {
    /* already in table - just reassign value */
    if (node->cleaner)
    {
      node->cleaner(true, (node->key != key),
                    (void*)node->key, node->value);
    }

    node->value   = value;
    node->key     = key;
    node->cleaner = cleaner;     /* new value may have different cleaner */

    return true;
  }

  /* increase the size of the table if necessary */
  if ( (tbl->count + 1) > ( (tbl->bcount * 3) >> 2 ) )
  {
    /* double size and add one to make it an odd number */
    if ( !_resize_hashtable(tbl, (tbl->bcount << 1) + 1, err) )
    {
      return false;
    }

    /* recalculate bucket */
    bucket = _bucket_from_khash(tbl, khash);
  }

  /* create new node */
  node = ls_data_malloc( sizeof(struct _ls_hnode) );
  if (!node)
  {
    LS_ERROR(err, LS_ERR_NO_MEMORY);
    return false;
  }

  node->next           = NULL;
  node->key            = key;
  node->value          = value;
  node->bucket         = bucket;
  node->khash          = khash;
  node->cleaner        = cleaner;
  node->next           = tbl->buckets[bucket];
  tbl->buckets[bucket] = node;
  tbl->count++;
  node->bucket = bucket;

  return true;
}

LS_API void
ls_htable_remove(ls_htable*  tbl,
                 const void* key)
{
  ls_hnode* node = _find_node(tbl, key, -1, 0);
  assert(tbl);
  if (node)
  {
    ls_htable_remove_node(tbl, node);
  }
}

LS_API void
ls_htable_clear(ls_htable* tbl)
{
  unsigned int i;
  ls_hnode*    cur, * next;

  assert(tbl);

  for (i = 0; i < tbl->bcount; i++)
  {
    /* free each bucket in turn */
    cur = tbl->buckets[i];
    while (cur)
    {
      /* clean up each of the nodes in this bucket */
      next = cur->next;
      if (cur->cleaner)
      {
        cur->cleaner(false, true, (void*)cur->key, cur->value);
      }
      ls_data_free(cur);
      cur = next;
    }
    tbl->buckets[i] = NULL;
  }
  tbl->count = 0;    /* no elements */
}

LS_API ls_hnode*
ls_htable_get_first_node(ls_htable* tbl)
{
  unsigned int i = 0;

  for (i = 0; i < tbl->bcount; i++)
  {
    if (tbl->buckets[i])
    {
      return tbl->buckets[i];
    }
  }
  return NULL;
}

LS_API ls_hnode*
ls_htable_get_next_node(ls_htable* tbl,
                        ls_hnode*  cur)
{
  unsigned int i;

  assert(tbl);
  assert(cur);

  if (cur->next)
  {
    return cur->next;
  }

  for (i = cur->bucket + 1; i < tbl->bcount; i++)
  {
    if (tbl->buckets[i])
    {
      return tbl->buckets[i];
    }
  }
  return NULL;
}

LS_API unsigned int
ls_htable_walk(ls_htable*         tbl,
               ls_htable_walkfunc func,
               void*              user_data)
{
  unsigned int i, count = 0;
  int          running = 1;
  ls_hnode*    cur, * next;

  assert(tbl);
  assert(func);

  for (i = 0; running && (i < tbl->bcount); i++)
  {
    /* visit the contents of each bucket */
    cur = tbl->buckets[i];
    while (running && cur)
    {
      /* visit each node in turn */
      next = cur->next;
      count++;
      running = (*func)(user_data, cur->key, cur->value);
      cur     = next;
    }
  }
  return count;
}

/*
 *  hashcode/compare functions
 */
LS_API unsigned int
ls_str_hashcode(const void* key)
{
  const char*   s = (const char*)key;
  unsigned long h = 0;
  const char*   p;

  assert(s);
  for (p = (const char*)s; *p != '\0'; p += 1)
  {
    h = (h << 5) - h + *p;
  }
  return (unsigned int) h;
}

LS_API ls_htable_cmpfunc ls_str_compare =
  (ls_htable_cmpfunc)strcmp;

LS_API unsigned int
ls_strcase_hashcode(const void* key)
{
  const char*   s = (const char*)key;
  unsigned long h = 0;
  const char*   p;

  assert(s);
  for (p = (const char*)s; *p != '\0'; p += 1)
  {
    h = (h << 5) - h + (unsigned long)tolower(*p);
  }
  return (unsigned int) h;
}

LS_API ls_htable_cmpfunc ls_strcase_compare =
  (ls_htable_cmpfunc)strcasecmp;

LS_API unsigned int
ls_int_hashcode(const void* key)
{
  /* Taken from Thomas Wangs article on integer hash functions.
   * http://www.concentric.net/~Ttwang/tech/inthash.htm
   */
  /* NOTE: assumed to be int; casting to long to minimize warnings */
  unsigned long a = ( (unsigned long) key ) & 0xffffffff;
  a = (a + 0x7ed55d16) + (a << 12);
  a = (a ^ 0xc761c23c) ^ (a >> 19);
  a = (a + 0x165667b1) + (a << 5);
  a = (a + 0xd3a2646c) ^ (a << 9);
  a = (a + 0xfd7046c5) + (a << 3);
  a = (a ^ 0xb55a4f09) ^ (a >> 16);
  return (unsigned int)a;
}

LS_API int
ls_int_compare(const void* key1,
               const void* key2)
{
  /* NOTE: assumed to be int; casting to long to minimize warnings */
  long i1 = (long)key1;
  long i2 = (long)key2;

  if (i1 < i2)
  {
    return -1;
  }
  return (i1 == i2 ? 0 : 1);
}

LS_API void
ls_htable_free_data_cleaner(bool  replace,
                            bool  destroy_key,
                            void* key,
                            void* data)
{
  UNUSED_PARAM(replace);
  UNUSED_PARAM(key);
  UNUSED_PARAM(destroy_key);
  ls_data_free(data);
}
