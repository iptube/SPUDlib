/**
 * \file
 * \brief
 * Datatypes and functions for hashtables.
 *
 * \b NOTE: Instances of ls_htable do not take ownership of keys and values.
 * Users MUST ensure any memory allocated for keys and values is released
 * when they are no longer in use.
 *
 * \b NOTE: This API is not thread-safe.  Users MUST ensure access to all
 * instances of a hashtable is limited to a single thread.
 *
 * Copyright (c) 2015 SPUDlib authors.  See LICENSE file.
 */

#pragma once

#include "ls_basics.h"
#include "ls_error.h"

/** An instance of a hashtable */
typedef struct _ls_htable ls_htable;

/** A node in the hashtable */
typedef struct _ls_hnode ls_hnode;


/**
 * Pointer to a function for generating hashcodes.
 *
 * \param key The key to generate a hashcode for
 * \retval int The hashcode for key
 * \see ls_int_hashcode
 * \see ls_str_hashcode
 * \see ls_strcase_hashcode
 */
typedef unsigned int (* ls_htable_hashfunc)(const void* key);

/**
 * Pointer to a function for comparing keys.
 *
 * \param key1 The first key to compare
 * \param key2 The second key to compare
 * \retval int less than 0 if key1 is before key2,
 *             greater than 0 if key1 is after key2,
 *             0 if key1 and key2 are equal
 * \see ls_int_compare
 */
typedef int (* ls_htable_cmpfunc)(const void* key1,
                                  const void* key2);

/**
 * Function pointer for walking all the elements in a hashtable
 *
 * \param user_data Optional data provided
 * \param key The current key being visited
 * \param data The current data being visited
 * \retval int 0 to stop walking the hashtable's elements, or 1 to continue.
 */
typedef int (* ls_htable_walkfunc)(void*       user_data,
                                   const void* key,
                                   void*       data);

/**
 * Function pointer for cleaning up a hashtable entry.
 * \param replace If true, the data for the given key is being replaced,
 *                not added.
 * \param destroy_key If true, any non-static data in the key should be
 *                destroyed.
 *                destroy_key will always be true if replace is false.
 *                destroy_key will be false if the new key has pointer equality
 *                with the old key.
 * \param key The old key being cleaned.
 * \param data The old data being cleaned.  This is usually freed in the called
 *                function.
 */
typedef void (* ls_htable_cleanfunc)(bool  replace,
                                     bool  destroy_key,
                                     void* key,
                                     void* data);

/**
 * Retrieves the key for the given hashtable node
 *
 * \invariant node != NULL
 * \param node The node to retrieve the key of
 * \retval void *The key of node
 */
LS_API const void*
ls_hnode_get_key(ls_hnode* node);

/**
 * Retrieves the value for the given hashtable node
 *
 * \invariant node != NULL
 * \param node The node to retrieve the value of
 * \retval void *The value of node
 */
LS_API void*
ls_hnode_get_value(ls_hnode* node);

/**
 * Changes the value of the given hashtable node
 *
 * \invariant node != NULL
 * \param[in] node The node to change the value of
 * \param[in] data The new value
 * \param[in] cleaner Function to call when data is replaced or deleted
 *        (provide NULL to ignore)
 */
LS_API void
ls_hnode_put_value(ls_hnode*           node,
                   void*               data,
                   ls_htable_cleanfunc cleaner);

/**
 * Creates a new hashtable.
 *
 * This function can generate the following errors (set when returning false):
 * \li \c LS_ERR_NO_MEMORY if the hashtable could not be allocated
 *
 * \invariant hash != NULL
 * \invariant cmp != NULL
 * \invariant tbl != NULL
 * \param[in] buckets Number of buckets to allocate for the hash table; this
 *                    value should be a prime number for maximum efficiency.  If
 *                    this value is 0, a default will be used.
 * \param[in] hash The key hashcode function to use.
 * \param[in] cmp The key comparison function to use.
 * \param[out] tbl The pointer to hold the initialized hashtable
 * \param[out] err The error information (provide NULL to ignore)
 * \retval bool true if successful, false otherwise.
 */
LS_API bool
ls_htable_create(int                buckets,
                 ls_htable_hashfunc hash,
                 ls_htable_cmpfunc  cmp,
                 ls_htable**        tbl,
                 ls_err*            err);

/**
 * Destroys a hashtable.
 *
 * <b>NOTE:</b> This function WILL clean up memory allocated
 * for the actual keys and values by calling the cleaner function supplied
 * when the values were inserted with ls_htable_put.  It is no longer necessary
 * to call ls_htable_clear first.
 *
 * \invariant tbl != NULL
 * \param tbl Hashtable to be destroyed.
 */
LS_API void
ls_htable_destroy(ls_htable* tbl);

/**
 * Returns the number of elements stored in the given hashtable
 *
 * \invariant tbl != NULL
 * \param tbl The hashtable
 * \retval unsigned int The number of elements in tbl
 */
LS_API unsigned int
ls_htable_get_count(ls_htable* tbl);

/**
 * Retrieves the node stored in the hashtable.
 *
 * \invariant tbl != NULL
 * \param tbl the hashtable to look in.
 * \param key the key value to search on.
 * \retval ls_hnode the node corresponding to the specified key,
 *         or NULL if not found.
 */
LS_API ls_hnode*
ls_htable_get_node(ls_htable*  tbl,
                   const void* key);

/**
 * Removes the node from the hashtable, calling whatever cleaner function
 * is registered.
 *
 * \invariant tbl != NULL
 * \invariant node != NULL
 * \param tbl The table to remove from
 * \param node The node to remove
 */
LS_API void
ls_htable_remove_node(ls_htable* tbl,
                      ls_hnode*  node);

/**
 * Retrieves a value stored in the hashtable.
 *
 * \invariant tbl != NULL
 * \param tbl the hashtable to look in.
 * \param key the key value to search on.
 * \retval void * Value corresponding to the specified key, NULL if not found.
 */
LS_API void*
ls_htable_get(ls_htable*  tbl,
              const void* key);

/**
 * Associates a key with a value in this hashtable. If there is already
 * a value for this key, it is replaced, and the previous cleaner function
 * (if any) is called for the previous value.  If required for the new value,
 * a new cleaner function should be provided even when replacing.
 *
 * This function can generate the following errors (set when returning false):
 * \li \c LS_ERR_NO_MEMORY if the hashtable could not be allocated
 *
 * \invariant tbl != NULL
 * \param[in] tbl Hashtable to add/update.
 * \param[in] key Key to use for the value in the table.
 * \param[in] value Value to add for this key.
 * \param[in] cleaner Function to call when the item is deleted or replaced.
 *                    (provide NULL to ignore)
 * \param[out] err The error information (provide NULL to ignore)
 * \retval bool if successful; false otherwise.
 */
LS_API bool
ls_htable_put(ls_htable*          tbl,
              const void*         key,
              void*               value,
              ls_htable_cleanfunc cleaner,
              ls_err*             err);

/**
 * Removes an entry from a hashtable, given its key.
 * Note, the cleaner function will be called.
 *
 * \invariant tbl != NULL
 * \param tbl Hashtable to remove from.
 * \param key Key of value to remove.
 */
LS_API void
ls_htable_remove(ls_htable*  tbl,
                 const void* key);

/**
 * Frees all elements in a hashtable, calling the cleaner function for
 * each item to free it.
 *
 * \invariant tbl != NULL
 * \param tbl Hash table to clear out.
 */
LS_API void
ls_htable_clear(ls_htable* tbl);

/**
 * Returns the first element in the hashtable.
 *
 * \invariant tbl != NULL
 * \param tbl the hashtable to look in
 * \retval ls_hnode the first node in the hashtable or NULL if there
 *                          isn't one.
 */
LS_API ls_hnode*
ls_htable_get_first_node(ls_htable* tbl);

/**
 * Returns the next node in the hashtable
 *
 * \invariant tbl != NULL
 * \param tbl the hashtable to look in
 * \param cur the current node
 * \retval ls_hnode a pointer to the next node or NULL if there isn't one.
 */
LS_API ls_hnode*
ls_htable_get_next_node(ls_htable* tbl,
                        ls_hnode*  cur);

/**
 * Iterates through a hashtable, calling a callback function for each element
 * stored in it.
 * \param tbl  Hashtable to walk.
 * \param func Function to be called for each node.
 * \param user_data Value to use as the first parameter for the callback
 *                  function.
 * \return int Number of nodes visited up to and including the one for which
 *             the callback function returned 0, if it did
 */
LS_API unsigned int
ls_htable_walk(ls_htable*         tbl,
               ls_htable_walkfunc func,
               void*              user_data);

/**
 * Generates hashcodes for strings (case-sensitive).
 *
 * \param key The NULL-terminated string to hash
 * \retval unsigned int The hashcode for s
 */
LS_API unsigned int
ls_str_hashcode(const void* key);

/**
 * Compares string keys (case-sensitive). This is a casting of strcmp to
 * overcome warnings about incompatible pointer types, and to provide a
 * compliment to {@link ls_str_hashcode()}.
 *
 * \param key1 The first NULL-terminated key to compare
 * \param key2 The second NULL-terminated key to compare
 * \retval int less than 0 if key1 is before key2;
 *             greater than 0 if key1 is after key2;
 *             0 if key1 is equal to key2
 */
LS_API extern ls_htable_cmpfunc ls_str_compare;

/**
 * Generates hashcodes for strings (case-insensitive).
 *
 * \param key The NULL-terminated string to hash
 * \retval unsigned int The hashcode for s
 */
LS_API unsigned int
ls_strcase_hashcode(const void* key);

/**
 * Compares string keys (case-insensitive). This is a casting of
 * strcasecmp to overcome warnings about incompatible pointer types, and
 * to provide a compliment to {@link ls_strcase_hashcode()}.
 *
 * \param key1 The first NULL-terminated key to compare
 * \param key2 The second NULL-terminated key to compare
 * \retval int less than 0 if key1 is before key2;
 *             greater than 0 if key1 is after key2;
 *             0 if key1 is equal to key2
 */
LS_API extern ls_htable_cmpfunc ls_strcase_compare;

/**
 * Generates hashcodes for integers.
 *
 * \param key The integer to hash
 * \retval unsigned int The hashcode for i
 */
LS_API unsigned int
ls_int_hashcode(const void* key);

/**
 * Compares two integers for relative positioning.
 *
 * \param key1 The first integer to compare
 * \param key2 The second integer to compare
 * \retval int less than 0 if i1 is before i2;
 *             greater than 0 if i1 is after i2;
 *             0 if i1 and i2 are equal
 */
LS_API int
ls_int_compare(const void* key1,
               const void* key2);

/**
 * Calls ls_data_free only on the data associated with a node.  Use this when
 * the keys are always static strings, and ls_data_free(data) is correct.
 *
 * \param replace Ignored
 * \param destroy_key Ignored
 * \param key Ignored
 * \param data The data that will be freed with ls_data_free.
 */
LS_API void
ls_htable_free_data_cleaner(bool  replace,
                            bool  destroy_key,
                            void* key,
                            void* data);
