/*
 * Copyright (c) 2015 SPUDlib authors.  See LICENSE file.
 */

#include <string.h>
#include <stdlib.h>

#include "ls_htable.h"
#include "ls_mem.h"
#include "test_utils.h"

typedef struct _test_walk_data
{
  ls_htable*        table;
  ls_htable_cmpfunc cmp;
  unsigned int      visited;
  int*              results;
} walk_data;

int
test_htable_nullwalk(void*       data,
                     const void* key,
                     void*       value)
{
  UNUSED_PARAM(data);
  UNUSED_PARAM(key);
  UNUSED_PARAM(value);
  return 1;
}

int
test_htable_stopwalk(void*       data,
                     const void* key,
                     void*       value)
{
  UNUSED_PARAM(data);
  UNUSED_PARAM(key);
  UNUSED_PARAM(value);
  return 0;
}

int
test_htable_walk(void*       data,
                 const void* key,
                 void*       value)
{
  walk_data* wd = (walk_data*)data;

  wd->results[wd->visited] =
    (wd->cmp(ls_htable_get(wd->table, key), value) == 0);
  wd->visited++;

  return 1;
}

static void* pvalue;

static void
test_htable_store_pvalue(bool  replace,
                         bool  destroy_key,
                         void* key,
                         void* value)
{
  UNUSED_PARAM(replace);
  UNUSED_PARAM(destroy_key);
  UNUSED_PARAM(key);
  pvalue = value;
}

static bool key_destroy = false;
static void
test_htable_destroy_key(bool  replace,
                        bool  destroy_key,
                        void* key,
                        void* value)
{
  UNUSED_PARAM(replace);
  if (destroy_key)
  {
    free(key);
    key_destroy = true;
  }
  free(value);
}

static bool
_oom_test(ls_err* err)
{
  ls_htable* table;

  /* initial bucket count of 1 to force a resize and increase coverage */
  if ( !ls_htable_create(1, ls_str_hashcode, ls_str_compare, &table, err) )
  {
    return false;
  }

  if ( !ls_htable_put(table, "key1", "value one", NULL, err)
       || !ls_htable_put(table, "key1", "value one again", NULL, err)
       || !ls_htable_put(table, "key2", "value two", NULL, err)
       || !ls_htable_put(table, "key3", "value three", NULL, err)
       || !ls_htable_put(table, "key4", "value four", NULL, err) )
  {
    ls_htable_destroy(table);
    return false;
  }

  if ( 4 != ls_htable_get_count(table) )
  {
    ls_htable_destroy(table);
    return false;
  }

  ls_htable_destroy(table);

  return true;
}

CTEST(ls_htable, hash_str)
{
  /* simply test that the hashcodes are different/same */
  ASSERT_EQUAL( ls_str_hashcode("key1"), ls_str_hashcode("key1") );
  ASSERT_NOT_EQUAL( ls_str_hashcode("key2"),    ls_str_hashcode("key1") );
  ASSERT_NOT_EQUAL( ls_str_hashcode("Key1"),    ls_str_hashcode("key1") );
  ASSERT_NOT_EQUAL( ls_str_hashcode("KEY1"),    ls_str_hashcode("key1") );
  ASSERT_NOT_EQUAL( ls_str_hashcode("KEY_ONE"), ls_str_hashcode("key1") );
  ASSERT_NOT_EQUAL( ls_str_hashcode(""),        ls_str_hashcode("key1") );

  ASSERT_EQUAL(ls_str_compare("key1", "key1"), 0);
  ASSERT_TRUE(ls_str_compare("key2", "key1") > 0);
  ASSERT_TRUE(ls_str_compare("Key1", "key1") < 0);
  ASSERT_TRUE(ls_str_compare("KEY1", "key1") < 0);
  ASSERT_TRUE(ls_str_compare("KEY_ONE", "key1") < 0);
  ASSERT_TRUE(ls_str_compare("", "key1") < 0);
}

CTEST(ls_htable, hash_strcase)
{
  /* simply test that the hashcodes are different/same */
  ASSERT_EQUAL( ls_strcase_hashcode("key1"), ls_strcase_hashcode("key1") );
  ASSERT_NOT_EQUAL( ls_strcase_hashcode("key2"), ls_strcase_hashcode("key1") );
  ASSERT_EQUAL( ls_strcase_hashcode("Key1"), ls_strcase_hashcode("key1") );
  ASSERT_EQUAL( ls_strcase_hashcode("KEY1"), ls_strcase_hashcode("key1") );
  ASSERT_NOT_EQUAL( ls_strcase_hashcode("KEY_ONE"),
                    ls_strcase_hashcode("key1") );
  ASSERT_NOT_EQUAL( ls_strcase_hashcode(""),
                    ls_strcase_hashcode("key1") );

  ASSERT_EQUAL(ls_strcase_compare("key1", "key1"), 0);
  ASSERT_TRUE(ls_strcase_compare("key2", "key1") > 0);
  ASSERT_EQUAL(ls_strcase_compare("Key1", "key1"), 0);
  ASSERT_EQUAL(ls_strcase_compare("KEY1", "key1"), 0);
  ASSERT_TRUE(ls_strcase_compare("KEY_ONE", "key1") > 0);
  ASSERT_TRUE(ls_strcase_compare("", "key1") < 0);
}

CTEST(ls_htable, hash_int)
{
  /* simply test that the hashcodes are different/same */
  ASSERT_EQUAL( ls_int_hashcode( (void*)25 ), ls_int_hashcode( (void*)25 ) );
  ASSERT_NOT_EQUAL( ls_int_hashcode( (void*)42 ),
                    ls_int_hashcode( (void*)25 ) );
  ASSERT_NOT_EQUAL( ls_int_hashcode( (void*)-1231 ),
                    ls_int_hashcode( (void*)25 ) );
  ASSERT_NOT_EQUAL( ls_int_hashcode( (void*)0 ),
                    ls_int_hashcode( (void*)25 ) );
  ASSERT_NOT_EQUAL( ls_int_hashcode( (void*)1236423 ),
                    ls_int_hashcode( (void*)25 ) );

  ASSERT_EQUAL(ls_int_compare( (void*)25, (void*)25 ), 0);
  ASSERT_TRUE(ls_int_compare( (void*)42, (void*)25 ) > 0);
  ASSERT_TRUE(ls_int_compare( (void*)-1231, (void*)25 ) < 0);
  ASSERT_TRUE(ls_int_compare( (void*)0, (void*)25 ) < 0);
  ASSERT_TRUE(ls_int_compare( (void*)1236423, (void*)25 ) > 0);
}

CTEST(ls_htable, create_destroy)
{
  ls_htable* table;
  ls_err     err;

  ASSERT_TRUE( ls_htable_create(7,
                                ls_int_hashcode,
                                ls_int_compare,
                                &table,
                                &err) );
  ASSERT_NOT_NULL(table);

  ls_htable_destroy(table);
}

CTEST(ls_htable, basics)
{
  ls_htable* table;
  ls_err     err;
  ASSERT_TRUE( ls_htable_create(7,
                                ls_str_hashcode,
                                ls_str_compare,
                                &table,
                                &err) );

  ASSERT_EQUAL(ls_htable_get_count(table), 0);
  ASSERT_NULL( ls_htable_get(table, "key1") );
  ASSERT_NULL( ls_htable_get(table, "key2") );

  pvalue = NULL;
  ASSERT_TRUE( ls_htable_put(table, "key1", "value one",
                             test_htable_store_pvalue, &err) );
  ASSERT_NULL(pvalue);
  ASSERT_EQUAL(ls_htable_get_count(table),                        1);
  ASSERT_EQUAL(strcmp(ls_htable_get(table, "key1"), "value one"), 0);
  ASSERT_NULL( ls_htable_get(table, "key2") );

  ASSERT_TRUE( ls_htable_put(table, "key2", "value two",
                             test_htable_store_pvalue, &err) );
  ASSERT_NULL(pvalue);
  ASSERT_EQUAL(ls_htable_get_count(table),                        2);
  ASSERT_EQUAL(strcmp(ls_htable_get(table, "key1"), "value one"), 0);
  ASSERT_EQUAL(strcmp(ls_htable_get(table, "key2"), "value two"), 0);

  ASSERT_TRUE( ls_htable_put(table, "key1", "val 1", test_htable_store_pvalue,
                             &err) );
  ASSERT_EQUAL(strcmp( (const char*)pvalue, "value one" ),        0);
  ASSERT_EQUAL(ls_htable_get_count(table),                        2);
  ASSERT_EQUAL(strcmp(ls_htable_get(table, "key1"), "val 1"),     0);
  ASSERT_EQUAL(strcmp(ls_htable_get(table, "key2"), "value two"), 0);

  pvalue = NULL;
  ls_htable_remove(table, "key1");
  ASSERT_EQUAL(strcmp(pvalue, "val 1"),    0);
  ASSERT_EQUAL(ls_htable_get_count(table), 1);
  ASSERT_NULL( ls_htable_get(table, "key1") );
  ASSERT_EQUAL(strcmp(ls_htable_get(table, "key2"), "value two"), 0);

  pvalue = NULL;
  ASSERT_TRUE( ls_htable_put(table, "key1", "first value",
                             test_htable_store_pvalue, &err) );
  ASSERT_NULL(pvalue);
  ASSERT_EQUAL(ls_htable_get_count(table),                          2);
  ASSERT_EQUAL(strcmp(ls_htable_get(table, "key1"), "first value"), 0);
  ASSERT_EQUAL(strcmp(ls_htable_get(table, "key2"), "value two"),   0);

  ls_htable_clear(table);
  ASSERT_EQUAL(ls_htable_get_count(table), 0);
  ASSERT_NULL( ls_htable_get(table, "key1") );
  ASSERT_NULL( ls_htable_get(table, "key2") );

  ls_htable_destroy(table);
}

CTEST(ls_htable, node)
{
  ls_htable* table;
  ls_hnode*  node;
  ls_err     err;
  ASSERT_TRUE( ls_htable_create(7,
                                ls_str_hashcode,
                                ls_str_compare,
                                &table,
                                &err) );

  ls_htable_put(table, "key1", "value one",   test_htable_store_pvalue, &err);
  ls_htable_put(table, "key2", "value two",   test_htable_store_pvalue, &err);
  ls_htable_put(table, "key3", "value three", test_htable_store_pvalue, &err);

  node = ls_htable_get_node(table, "key1");
  ASSERT_EQUAL(strcmp(ls_hnode_get_key(node), "key1"),        0);
  ASSERT_EQUAL(strcmp(ls_hnode_get_value(node), "value one"), 0);

  pvalue = NULL;
  ls_hnode_put_value(node, "val1", test_htable_store_pvalue);
  ASSERT_EQUAL(strcmp(pvalue, "value one"),              0);
  ASSERT_EQUAL(strcmp(ls_hnode_get_value(node), "val1"), 0);

  pvalue = NULL;
  ls_hnode_put_value(node, "value 1", test_htable_store_pvalue);
  ASSERT_EQUAL(strcmp(pvalue, "val1"),                      0);
  ASSERT_EQUAL(strcmp(ls_hnode_get_value(node), "value 1"), 0);

  pvalue = NULL;
  ls_hnode_put_value(node, "first value", NULL);
  ASSERT_EQUAL(strcmp(pvalue, "value 1"),                       0);
  ASSERT_EQUAL(strcmp(ls_hnode_get_value(node), "first value"), 0);

  pvalue = NULL;
  ls_hnode_put_value(node, "value #1", test_htable_store_pvalue);
  ASSERT_NULL(pvalue);
  ASSERT_EQUAL(strcmp(ls_hnode_get_value(node), "value #1"), 0);

  ls_htable_remove_node(table, node);
  ASSERT_EQUAL(ls_htable_get_count(table), 2);

  ls_htable_destroy(table);
}

CTEST(ls_htable, iteration)
{
  ls_htable*   table;
  ls_hnode*    node;
  unsigned int count;
  ls_err       err;

  /* initial bucket count of 1 to force a resize and increase coverage */
  ASSERT_TRUE( ls_htable_create(1, ls_str_hashcode,
                                ls_str_compare, &table, &err) );

  node = ls_htable_get_first_node(table);
  ASSERT_NULL(node);

  ASSERT_TRUE( ls_htable_put(table, "key1", ls_data_strdup("value one"),
                             ls_htable_free_data_cleaner, NULL) );
  ASSERT_TRUE( ls_htable_put(table, "key2", ls_data_strdup("value two"),
                             ls_htable_free_data_cleaner, NULL) );
  ASSERT_TRUE( ls_htable_put(table, "key3", ls_data_strdup("value three"),
                             ls_htable_free_data_cleaner, NULL) );

  count = 1;
  node  = ls_htable_get_first_node(table);
  while ( ( node = ls_htable_get_next_node(table, node) ) != NULL )
  {
    count++;
    ASSERT_NOT_NULL( ls_hnode_get_key(node) );
    ASSERT_NOT_NULL( ls_hnode_get_value(node) );
  }
  ASSERT_TRUE( count == ls_htable_get_count(table) );

  ls_htable_remove(table, "key2");

  count = 1;
  node  = ls_htable_get_first_node(table);
  while ( ( node = ls_htable_get_next_node(table, node) ) != NULL )
  {
    count++;
    ASSERT_NOT_NULL( ls_hnode_get_key(node) );
    ASSERT_NOT_NULL( ls_hnode_get_value(node) );
  }
  ASSERT_EQUAL( count, ls_htable_get_count(table) );

  ls_htable_destroy(table);
}

CTEST(ls_htable, walk)
{
  ls_htable*   table;
  walk_data    wd;
  unsigned int idx;
  ls_err       err;

  ASSERT_TRUE( ls_htable_create(7,
                                ls_str_hashcode,
                                ls_str_compare,
                                &table,
                                &err) );

  ASSERT_TRUE( ls_htable_put(table, "key1", "value one", NULL, &err) );
  ASSERT_TRUE( ls_htable_put(table, "key2", "value two", NULL, &err) );
  ASSERT_TRUE( ls_htable_put(table, "key3", "value three", NULL, &err) );
  wd.table   = table;
  wd.cmp     = ls_str_compare;
  wd.visited = 0;
  wd.results = (int*)malloc( 3 * sizeof(int) );

  ls_htable_walk(table, test_htable_walk, &wd);
  ASSERT_EQUAL(ls_htable_get_count(table), wd.visited);
  idx = 0;
  while (idx < wd.visited)
  {
    ASSERT_TRUE(wd.results[idx++]);
  }

  free(wd.results);
  ls_htable_destroy(table);
}

CTEST(ls_htable, collisions)
{
  ls_htable* table;
  ls_hnode*  nodea, * nodeb, * node;

  /* "09Vi" and "08vJ" collide, which WILL change if the string hashcode */
  /* function changes */
  char* a = "09Vi";
  char* b = "08vJ";

  ASSERT_TRUE( ls_htable_create(5,
                                ls_str_hashcode,
                                ls_str_compare,
                                &table,
                                NULL) );

  ASSERT_EQUAL( ls_str_hashcode(a), ls_str_hashcode(b) );
  ASSERT_TRUE( ls_htable_put(table, a, "1", NULL, NULL) );
  ASSERT_NULL( ls_htable_get(table, b) );
  ASSERT_TRUE( ls_htable_put(table, b, "2", NULL, NULL) );
  ASSERT_NOT_NULL( ls_htable_get(table, b) );
  ASSERT_NOT_NULL( ls_htable_get(table, a) );
  ASSERT_EQUAL(ls_htable_walk(table, test_htable_nullwalk, NULL), 2);
  ASSERT_EQUAL(ls_htable_walk(table, test_htable_stopwalk, NULL), 1);

  nodea = ls_htable_get_node(table, a);
  nodeb = ls_htable_get_node(table, b);
  node  = ls_htable_get_next_node(table, nodea);
  if (node != NULL)
  {
    ASSERT_TRUE(node == nodeb);
  }
  else
  {
    node = ls_htable_get_next_node(table, nodeb);
    ASSERT_TRUE(node == nodea);
  }

  ls_htable_remove(table, "non-existant");
  ls_htable_remove(table, a);
  ASSERT_TRUE( ls_htable_put(table, a, "3", NULL, NULL) );
  ls_htable_remove(table, b);
  ls_htable_remove(table, a);

  ls_htable_destroy(table);
}

CTEST(ls_htable, cleaner_edges)
{
  ls_htable* table;
  ls_err     err;

  ASSERT_TRUE( ls_htable_create(-1,
                                ls_str_hashcode,
                                ls_str_compare,
                                &table,
                                &err) );
  ASSERT_EQUAL(ls_htable_walk(table, test_htable_nullwalk, NULL), 0);
  ASSERT_TRUE( ls_htable_put(table, "key1", "value one", NULL, &err) );
  ASSERT_TRUE( ls_htable_put(table, "key1", "value one prime", NULL, &err) );
  ASSERT_TRUE( ls_htable_put(table, "key1", NULL, NULL, &err) );
  ASSERT_TRUE( ls_htable_put(table, "key1", "value one", NULL, &err) );
  ls_htable_remove(table, "key1");
  ASSERT_TRUE( ls_htable_put(table, "key1", NULL, NULL, &err) );
  ls_htable_remove(table, "key1");
  ASSERT_TRUE( ls_htable_put(table, "key1", "value one", NULL, &err) );
  ls_htable_clear(table);
  ls_htable_destroy(table);
}

CTEST(ls_htable, destroy_key)
{
  ls_htable* table;
  ls_err     err;

  ASSERT_TRUE( ls_htable_create(-1,
                                ls_str_hashcode,
                                ls_str_compare,
                                &table,
                                &err) );
  key_destroy = false;
  char* k = strdup("key1");
  ASSERT_TRUE( ls_htable_put(table, k, strdup("value one"),
                             test_htable_destroy_key, &err) );
  ASSERT_FALSE(key_destroy);
  ASSERT_TRUE( ls_htable_put(table, k, strdup("value two"),
                             test_htable_destroy_key, &err) );
  ASSERT_FALSE(key_destroy);
  ASSERT_TRUE( ls_htable_put(table, strdup(k), strdup("value two"),
                             test_htable_destroy_key, &err) );
  ASSERT_TRUE( key_destroy);
  key_destroy = false;
  ls_htable_destroy(table);
  ASSERT_TRUE(key_destroy);
}

CTEST(ls_htable, oom)
{
  OOM_SIMPLE_TEST( _oom_test(&err) );
  OOM_TEST_INIT();
  OOM_TEST( NULL, _oom_test(NULL) );
}
