/**
 * Copyrights
 *
 * Portions created or assigned to Cisco Systems, Inc. are
 * Copyright (c) 2010 Cisco Systems, Inc.  All Rights Reserved.
 */

#include "htable.h"
#include "mem.h"

#include <assert.h>
#include <ctype.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/*****************************************************************************
 * Internal type definitions
 */

#define HASH_NUM_BUCKETS   509          /* should be a prime number; see Knuth */

typedef struct _jw_hnode_int
{
    struct _jw_hnode_int *next; /* next node in list */
    const void *key;                    /* key pointer */
    void *value;                        /* value pointer */
    int bucket;
    unsigned int khash;
} jw_hnode_int;

typedef struct _jw_htable_int
{
    jw_htable_hashfunc hash;    /* hash function */
    jw_htable_cmpfunc cmp;      /* comparison function */
    unsigned int count;                 /* table entry count */
    unsigned int bcount;                /* bucket count */
    unsigned int resize_count;          /* number of time resized */
    jw_hnode_int **buckets;     /* the hash buckets */
} jw_htable_int;

#define _hash_key(tb, key)             ((*((tb)->hash))(key))
#define _bucket_from_khash(tb, khash)  ((khash) % ((tb)->bcount))

/****************************************************************************
 * Internal functions
 */

/**
 * walks a hash bucket to find a node whose key matches the named key value.
 * Returns the node pointer, or NULL if it's not found.
 */
static jw_hnode _find_node(jw_htable tab,
                           const void *key,
                           int bucket,
                           unsigned int khash)
{
    register jw_hnode p;  /* search pointer/return from this function */

    if (bucket < 0)
    {
        khash = _hash_key(tab, key);
        bucket = _bucket_from_khash(tab, khash);
    }

    for (p = tab->buckets[bucket]; p; p = p->next)
    {
        if ((khash == p->khash) && ((*(tab->cmp))(key, p->key) == 0))
        {
            return p;
        }
    }

    return NULL;
}

/**
 * Resizes the hashtable.
 */
static bool _resize_hashtable(jw_htable tab,
                              unsigned int buckets,
                              jw_err *err)
{
    jw_hnode_int **old_buckets = tab->buckets;
    unsigned int old_bcount = tab->bcount;
    jw_hnode_int **new_buckets;
    jw_hnode_int *node, *next_node;
    unsigned int c;

    new_buckets =
        (jw_hnode_int **)jw_data_malloc(buckets * sizeof(jw_hnode_int*));
    if (!new_buckets)
    {
        JABBERWERX_ERROR(err, JW_ERR_NO_MEMORY);
        return false;
    }
    memset(new_buckets, 0, buckets * sizeof(jw_hnode_int*));

    tab->buckets = new_buckets;
    tab->bcount = buckets;
    ++tab->resize_count;

    for (c = 0; c < old_bcount; ++c)
    {
        node = old_buckets[c];
        while (node != 0)
        {
            unsigned int bucket = node->khash % tab->bcount;

            next_node = node->next;
            node->bucket = bucket;
            node->next = tab->buckets[bucket];
            tab->buckets[bucket] = node;

            node = next_node;
        }
    }

    jw_data_free(old_buckets);
    return true;
}

JABBERWERX_API const void *jw_hnode_get_key(jw_hnode node)
{
    assert(node);

    return node->key;
}
JABBERWERX_API void *jw_hnode_get_value(jw_hnode node)
{
    assert(node);

    return node->value;
}
JABBERWERX_API void *jw_hnode_put_value(jw_hnode node,
                                        void *data)
{
    void *pvalue;

    assert(node);
    pvalue = node->value;
    node->value = data;

    return pvalue;
}

JABBERWERX_API bool jw_htable_create(int buckets,
                                     jw_htable_hashfunc hash,
                                     jw_htable_cmpfunc cmp,
                                     jw_htable *tbl,
                                     jw_err *err)
{
    jw_htable tmp_table;  /* new table structure, used temporarily for
                           * creation; memory will be given to the out
                           * parameter 'tbl' upon success
                           */

    assert(tbl);
    assert(hash);
    assert(cmp);

    if (buckets <= 0)
    {
        buckets = HASH_NUM_BUCKETS;
    }

    tmp_table = (jw_htable)jw_data_malloc(sizeof(jw_htable_int));
    if (!tmp_table)
    {
        JABBERWERX_ERROR(err, JW_ERR_NO_MEMORY);
        return false;
    }
    memset(tmp_table, 0, sizeof(jw_htable_int));

    tmp_table->buckets =
        (jw_hnode_int **)jw_data_malloc(buckets * sizeof(jw_hnode_int*));
    if (!tmp_table->buckets)
    {
        jw_data_free(tmp_table);
        tmp_table = NULL;

        JABBERWERX_ERROR(err, JW_ERR_NO_MEMORY);
        return false;
    }
    memset(tmp_table->buckets, 0, buckets * sizeof(jw_hnode_int*));

    /* fill the fields of the hash table */
    tmp_table->hash = hash;
    tmp_table->cmp = cmp;
    tmp_table->bcount = buckets;
    *tbl = tmp_table;

    return true;
}

JABBERWERX_API void jw_htable_destroy(jw_htable tbl)
{
    jw_hnode_int *cur, *next;
    unsigned int i;

    assert(tbl);

    for (i = 0; i < tbl->bcount; i++)
    {
        cur = tbl->buckets[i];
        while (cur)
        {
            next = cur->next;
            jw_data_free(cur);
            cur = next;
        }
    }
    jw_data_free(tbl->buckets);
    jw_data_free(tbl);
}

JABBERWERX_API unsigned int jw_htable_get_count(jw_htable tbl)
{
    assert(tbl);
    return tbl->count;
}
JABBERWERX_API jw_hnode jw_htable_get_node(jw_htable tbl,
                                           const void* key)
{
    jw_hnode node;

    assert(tbl);

    node = _find_node(tbl, key, -1, 0);
    return node;
}

JABBERWERX_API void *jw_htable_get(jw_htable tbl,
                                   const void *key)
{
    jw_hnode_int *node;

    assert(tbl);

    node = _find_node(tbl, key, -1, 0);
    return node ? node->value : NULL;
}
JABBERWERX_API bool jw_htable_put(jw_htable tbl,
                                  const void *key,
                                  void *value,
                                  void **pvalue,
                                  jw_err *err)
{
    unsigned int khash;
    unsigned int bucket;
    jw_hnode_int *node;


    assert(tbl);

    if ((tbl->count + 1) > ((tbl->bcount * 3) >> 2))
    {
        /* double size and add one to make it an odd number */
        if (!_resize_hashtable(tbl, (tbl->bcount << 1) + 1, err))
        {
            return false;
        }
    }

    /* Compute the hash bucket and try to find an existing node. */
    khash = _hash_key(tbl, key);
    bucket = _bucket_from_khash(tbl, khash);
    node = _find_node(tbl, key, bucket, khash);
    if (!node)
    {
        node = (jw_hnode_int *)jw_data_malloc(sizeof(jw_hnode_int));
        if (!node)
        {
            JABBERWERX_ERROR(err, JW_ERR_NO_MEMORY);
            return false;
        }

        node->next = NULL;
        node->key = key;
        node->value = value;
        node->bucket = bucket;
        node->khash = khash;
        node->next = tbl->buckets[bucket];
        tbl->buckets[bucket] = node;
        tbl->count++;
        node->bucket = bucket;
        if (pvalue)
        {
            *pvalue = NULL;
        }
    }
    else
    {/* already in table - just reassign value */
        if (pvalue)
        {
            *pvalue = node->value;
        }
        node->value = value;
    }

    return true;
}
JABBERWERX_API void *jw_htable_remove(jw_htable tbl,
                                      const void *key)
{
    unsigned int khash;
    int bucket;
    jw_hnode_int *node;
    register jw_hnode_int *p;
    void *pvalue = NULL;

    assert(tbl);

    /* Compute the hash bucket and try to find an existing node. */
    khash = _hash_key(tbl, key);
    bucket = _bucket_from_khash(tbl, khash);
    node = _find_node(tbl, key, bucket, khash);
    if (node)
    { /* look to unchain it from the bucket it's in */
        pvalue = node->value;
        if (node == tbl->buckets[bucket])
        { /* unchain at head */
            tbl->buckets[bucket] = node->next;
        }
        else
        { /* unchain in middle of list */
            for (p = tbl->buckets[bucket]; p->next != node; p = p->next) ;
            p->next = node->next;
        }

        jw_data_free(node);
        tbl->count--;
    }

    return pvalue;
}
JABBERWERX_API void jw_htable_clear(jw_htable tbl,
                                    jw_htable_walkfunc cleanup,
                                    void *user_data)
{
    unsigned int i;
    register jw_hnode_int *cur, *next;

    assert(tbl);

    for (i = 0; i < tbl->bcount; i++)
    { /* free each bucket in turn */
        cur = tbl->buckets[i];
        while (cur)
        { /* clean up each of the nodes in this bucket */
            /* start with the first one */
            next = cur->next;
            if (cleanup)
                (*cleanup)(user_data, cur->key, cur->value);
            jw_data_free(cur);
            cur = next;
        }
        tbl->buckets[i] = NULL;
    }
    tbl->count = 0;  /* no elements */
}

JABBERWERX_API jw_hnode_int *jw_htable_get_first_node(
        jw_htable tbl)
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
JABBERWERX_API jw_hnode_int *jw_htable_get_next_node(jw_htable tbl,
                                                     jw_hnode_int *cur)
{
    unsigned int i;

    assert(tbl);
    assert(cur);

    if (cur->next)
    {
        return cur->next;
    }

    for (i = cur->bucket + 1; i<tbl->bcount; i++)
    {
        if (tbl->buckets[i])
        {
            return tbl->buckets[i];
        }
    }
    return NULL;
}

JABBERWERX_API unsigned int jw_htable_walk(jw_htable tbl,
                                           jw_htable_walkfunc func,
                                           void *user_data)
{
    unsigned int i, count = 0;
    int running = 1;
    register jw_hnode_int *cur, *next;

    assert(tbl);
    assert(func);

    for (i = 0; running && (i < tbl->bcount); i++)
    { /* visit the contents of each bucket */
        cur = tbl->buckets[i];
        while (running && cur)
        { /* visit each node in turn */
            next = cur->next;
            count++;
            running = (*func)(user_data, cur->key, cur->value);
            cur = next;
        }
    }
    return count;
}

/* hashcode/compare functions */
JABBERWERX_API unsigned int jw_str_hashcode(const void *key)
{
    const char *s = (const char *)key;
    unsigned long h = 0;
    const char*   p;

    assert(s);
    for(p = (const char*)s; *p != '\0'; p += 1)
    {
        h = (h << 5) - h + *p;
    }
    return (unsigned int) h;
}
JABBERWERX_API jw_htable_cmpfunc jw_str_compare =
               (jw_htable_cmpfunc)strcmp;

JABBERWERX_API unsigned int jw_strcase_hashcode(const void *key){
    const char *s = (const char *)key;
    unsigned long h = 0;
    const char*   p;

    assert(s);
    for(p = (const char*)s; *p != '\0'; p += 1)
    {
        h = (h << 5) - h + (unsigned long)tolower(*p);
    }
    return (unsigned int) h;
}
JABBERWERX_API jw_htable_cmpfunc jw_strcase_compare =
               (jw_htable_cmpfunc)strcasecmp;

JABBERWERX_API unsigned int jw_int_hashcode(const void *key)
{
    /* Taken from Thomas Wangs article on integer hash functions.
     * http://www.concentric.net/~Ttwang/tech/inthash.htm
     */
    /* NOTE: assumed to be int; casting to long to minimize warnings */
    unsigned long a = ((unsigned long) key) & 0xffffffff;
    a = (a+0x7ed55d16) + (a<<12);
    a = (a^0xc761c23c) ^ (a>>19);
    a = (a+0x165667b1) + (a<<5);
    a = (a+0xd3a2646c) ^ (a<<9);
    a = (a+0xfd7046c5) + (a<<3);
    a = (a^0xb55a4f09) ^ (a>>16);
    return (unsigned int)a;
}
JABBERWERX_API int jw_int_compare(const void *key1, const void *key2)
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
