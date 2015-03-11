#include <string.h>
#include <stdlib.h>
#include <check.h>

#include "ls_htable.h"
#include "ls_mem.h"
#include "test_utils.h"

Suite * ls_htable_suite (void);

typedef struct _test_walk_data
{
    ls_htable           *table;
    ls_htable_cmpfunc   cmp;
    unsigned int        visited;
    int                 *results;
} walk_data;

int test_htable_nullwalk(void *data, const void *key, void *value)
{
    UNUSED_PARAM(data);
    UNUSED_PARAM(key);
    UNUSED_PARAM(value);
    return 1;
}

int test_htable_stopwalk(void *data, const void *key, void *value)
{
    UNUSED_PARAM(data);
    UNUSED_PARAM(key);
    UNUSED_PARAM(value);
    return 0;
}

int test_htable_walk(void *data, const void *key, void *value)
{
    walk_data *wd = (walk_data*)data;

    wd->results[wd->visited] = (wd->cmp(ls_htable_get(wd->table, key), value) == 0);
    wd->visited++;

    return 1;
}

static void *pvalue;

static void test_htable_store_pvalue(bool replace, bool destroy_key, void *key, void *value)
{
    UNUSED_PARAM(replace);
    UNUSED_PARAM(destroy_key);
    UNUSED_PARAM(key);
    pvalue = value;
}

static bool key_destroy = false;
static void test_htable_destroy_key(bool replace, bool destroy_key, void *key, void *value)
{
    UNUSED_PARAM(replace);
    if (destroy_key)
    {
        free(key);
        key_destroy = true;
    }
    free(value);
}

static bool _oom_test(ls_err *err)
{
    ls_htable *table;

    // initial bucket count of 1 to force a resize and increase coverage
    if (!ls_htable_create(1, ls_str_hashcode, ls_str_compare, &table, err))
    {
        return false;
    }

    if (!ls_htable_put(table, "key1", "value one", NULL, err)
     || !ls_htable_put(table, "key1", "value one again", NULL, err)
     || !ls_htable_put(table, "key2", "value two", NULL, err)
     || !ls_htable_put(table, "key3", "value three", NULL, err)
     || !ls_htable_put(table, "key4", "value four", NULL, err))
    {
        ls_htable_destroy(table);
        return false;
    }

    if (4 != ls_htable_get_count(table))
    {
        ls_htable_destroy(table);
        return false;
    }

    ls_htable_destroy(table);

    return true;
}

START_TEST (ls_htable_hash_str_test)
{
    /* simply test that the hashcodes are different/same */
    ck_assert(ls_str_hashcode("key1") == ls_str_hashcode("key1"));
    ck_assert(ls_str_hashcode("key2") != ls_str_hashcode("key1"));
    ck_assert(ls_str_hashcode("Key1") != ls_str_hashcode("key1"));
    ck_assert(ls_str_hashcode("KEY1") != ls_str_hashcode("key1"));
    ck_assert(ls_str_hashcode("KEY_ONE") != ls_str_hashcode("key1"));
    ck_assert(ls_str_hashcode("") != ls_str_hashcode("key1"));

    ck_assert(ls_str_compare("key1", "key1") == 0);
    ck_assert(ls_str_compare("key2", "key1") > 0);
    ck_assert(ls_str_compare("Key1", "key1") < 0);
    ck_assert(ls_str_compare("KEY1", "key1") < 0);
    ck_assert(ls_str_compare("KEY_ONE", "key1") < 0);
    ck_assert(ls_str_compare("", "key1") < 0);
}
END_TEST

START_TEST (ls_htable_hash_strcase_test)
{
    /* simply test that the hashcodes are different/same */
    ck_assert(ls_strcase_hashcode("key1") == ls_strcase_hashcode("key1"));
    ck_assert(ls_strcase_hashcode("key2") != ls_strcase_hashcode("key1"));
    ck_assert(ls_strcase_hashcode("Key1") == ls_strcase_hashcode("key1"));
    ck_assert(ls_strcase_hashcode("KEY1") == ls_strcase_hashcode("key1"));
    ck_assert(ls_strcase_hashcode("KEY_ONE") != ls_strcase_hashcode("key1"));
    ck_assert(ls_strcase_hashcode("") != ls_strcase_hashcode("key1"));

    ck_assert(ls_strcase_compare("key1", "key1") == 0);
    ck_assert(ls_strcase_compare("key2", "key1") > 0);
    ck_assert(ls_strcase_compare("Key1", "key1") == 0);
    ck_assert(ls_strcase_compare("KEY1", "key1") == 0);
    ck_assert(ls_strcase_compare("KEY_ONE", "key1") > 0);
    ck_assert(ls_strcase_compare("", "key1") < 0);
}
END_TEST

START_TEST (ls_htable_hash_int_test)
{
    /* simply test that the hashcodes are different/same */
    ck_assert(ls_int_hashcode((void*)25) == ls_int_hashcode((void*)25));
    ck_assert(ls_int_hashcode((void*)42) != ls_int_hashcode((void*)25));
    ck_assert(ls_int_hashcode((void*)-1231) != ls_int_hashcode((void*)25));
    ck_assert(ls_int_hashcode((void*)0) != ls_int_hashcode((void*)25));
    ck_assert(ls_int_hashcode((void*)1236423) != ls_int_hashcode((void*)25));

    ck_assert(ls_int_compare((void*)25, (void*)25) == 0);
    ck_assert(ls_int_compare((void*)42, (void*)25) > 0);
    ck_assert(ls_int_compare((void*)-1231, (void*)25) < 0);
    ck_assert(ls_int_compare((void*)0, (void*)25) < 0);
    ck_assert(ls_int_compare((void*)1236423, (void*)25) > 0);
}
END_TEST

START_TEST (ls_htable_create_destroy_test)
{
    ls_htable   *table;
    ls_err      err;

    ck_assert(ls_htable_create(7,
                             ls_int_hashcode,
                             ls_int_compare,
                             &table,
                             &err));
    ck_assert(table);

    ls_htable_destroy(table);
    ck_assert(1); /* confirm we did not assert */
}
END_TEST

START_TEST (ls_htable_basics_test)
{
    ls_htable   *table;
    ls_err      err;
    ck_assert(ls_htable_create(7,
                             ls_str_hashcode,
                             ls_str_compare,
                             &table,
                             &err));

    ck_assert(ls_htable_get_count(table) == 0);
    ck_assert(ls_htable_get(table, "key1") == NULL);
    ck_assert(ls_htable_get(table, "key2") == NULL);

    pvalue = NULL;
    ck_assert(ls_htable_put(table, "key1", "value one", test_htable_store_pvalue, &err));
    ck_assert(pvalue == NULL);
    ck_assert(ls_htable_get_count(table) == 1);
    ck_assert(strcmp(ls_htable_get(table, "key1"), "value one") == 0);
    ck_assert(ls_htable_get(table, "key2") == NULL);

    ck_assert(ls_htable_put(table, "key2", "value two", test_htable_store_pvalue, &err));
    ck_assert(pvalue == NULL);
    ck_assert(ls_htable_get_count(table) == 2);
    ck_assert(strcmp(ls_htable_get(table, "key1"), "value one") == 0);
    ck_assert(strcmp(ls_htable_get(table, "key2"), "value two") == 0);

    ck_assert(ls_htable_put(table, "key1", "val 1", test_htable_store_pvalue, &err));
    ck_assert(strcmp((const char *)pvalue, "value one") == 0);
    ck_assert(ls_htable_get_count(table) == 2);
    ck_assert(strcmp(ls_htable_get(table, "key1"), "val 1") == 0);
    ck_assert(strcmp(ls_htable_get(table, "key2"), "value two") == 0);

    pvalue = NULL;
    ls_htable_remove(table, "key1");
    ck_assert(strcmp(pvalue, "val 1") == 0);
    ck_assert(ls_htable_get_count(table) == 1);
    ck_assert(ls_htable_get(table, "key1") == NULL);
    ck_assert(strcmp(ls_htable_get(table, "key2"), "value two") == 0);

    pvalue = NULL;
    ck_assert(ls_htable_put(table, "key1", "first value", test_htable_store_pvalue, &err));
    ck_assert(pvalue == NULL);
    ck_assert(ls_htable_get_count(table) == 2);
    ck_assert(strcmp(ls_htable_get(table, "key1"), "first value") == 0);
    ck_assert(strcmp(ls_htable_get(table, "key2"), "value two") == 0);

    ls_htable_clear(table);
    ck_assert(ls_htable_get_count(table) == 0);
    ck_assert(ls_htable_get(table, "key1") == NULL);
    ck_assert(ls_htable_get(table, "key2") == NULL);

    ls_htable_destroy(table);
}
END_TEST

START_TEST (ls_htable_node_test)
{
    ls_htable   *table;
    ls_hnode    *node;
    ls_err      err;
    ck_assert(ls_htable_create(7,
                             ls_str_hashcode,
                             ls_str_compare,
                             &table,
                             &err));

    ls_htable_put(table, "key1", "value one", test_htable_store_pvalue, &err);
    ls_htable_put(table, "key2", "value two", test_htable_store_pvalue, &err);
    ls_htable_put(table, "key3", "value three", test_htable_store_pvalue, &err);

    node = ls_htable_get_node(table, "key1");
    ck_assert(strcmp(ls_hnode_get_key(node), "key1") == 0);
    ck_assert(strcmp(ls_hnode_get_value(node), "value one") == 0);

    pvalue = NULL;
    ls_hnode_put_value(node, "val1", test_htable_store_pvalue);
    ck_assert(strcmp(pvalue, "value one") == 0);
    ck_assert(strcmp(ls_hnode_get_value(node), "val1") == 0);

    pvalue = NULL;
    ls_hnode_put_value(node, "value 1", test_htable_store_pvalue);
    ck_assert(strcmp(pvalue, "val1") == 0);
    ck_assert(strcmp(ls_hnode_get_value(node), "value 1") == 0);

    pvalue = NULL;
    ls_hnode_put_value(node, "first value", NULL);
    ck_assert(strcmp(pvalue, "value 1") == 0);
    ck_assert(strcmp(ls_hnode_get_value(node), "first value") == 0);

    pvalue = NULL;
    ls_hnode_put_value(node, "value #1", test_htable_store_pvalue);
    ck_assert(NULL == pvalue);
    ck_assert(strcmp(ls_hnode_get_value(node), "value #1") == 0);

    ls_htable_remove_node(table, node);
    ck_assert_int_eq(ls_htable_get_count(table), 2);

    ls_htable_destroy(table);
}
END_TEST

START_TEST (ls_htable_iteration_test)
{
    ls_htable    *table;
    ls_hnode     *node;
    unsigned int count;
    ls_err       err;

    // initial bucket count of 1 to force a resize and increase coverage
    ck_assert(ls_htable_create(
                        1, ls_str_hashcode, ls_str_compare, &table, &err));

    node = ls_htable_get_first_node(table);
    ck_assert(node == NULL);

    ck_assert(ls_htable_put(table, "key1", ls_data_strdup("value one"),
                          ls_htable_free_data_cleaner, NULL));
    ck_assert(ls_htable_put(table, "key2", ls_data_strdup("value two"),
                          ls_htable_free_data_cleaner, NULL));
    ck_assert(ls_htable_put(table, "key3", ls_data_strdup("value three"),
                          ls_htable_free_data_cleaner, NULL));

    count = 1;
    node = ls_htable_get_first_node(table);
    while ((node = ls_htable_get_next_node(table, node)) != NULL)
    {
        count++;
        ck_assert(ls_hnode_get_key(node) != NULL);
        ck_assert(ls_hnode_get_value(node) != NULL);
    }
    ck_assert(count == ls_htable_get_count(table));

    ls_htable_remove(table, "key2");

    count = 1;
    node = ls_htable_get_first_node(table);
    while ((node = ls_htable_get_next_node(table, node)) != NULL)
    {
        count++;
        ck_assert(ls_hnode_get_key(node) != NULL);
        ck_assert(ls_hnode_get_value(node) != NULL);
    }
    ck_assert(count == ls_htable_get_count(table));

    ls_htable_destroy(table);
}
END_TEST

START_TEST (ls_htable_walk_test)
{
    ls_htable    *table;
    walk_data    wd;
    unsigned int idx;
    ls_err       err;

    ck_assert(ls_htable_create(7,
                             ls_str_hashcode,
                             ls_str_compare,
                             &table,
                             &err));

    ck_assert(ls_htable_put(table, "key1", "value one", NULL, &err));
    ck_assert(ls_htable_put(table, "key2", "value two", NULL, &err));
    ck_assert(ls_htable_put(table, "key3", "value three", NULL, &err));
    wd.table = table;
    wd.cmp = ls_str_compare;
    wd.visited = 0;
    wd.results = (int*)malloc(3 * sizeof(int));

    ls_htable_walk(table, test_htable_walk, &wd);
    ck_assert(ls_htable_get_count(table) == wd.visited);
    idx = 0;
    while (idx < wd.visited)
    {
        ck_assert(wd.results[idx++]);
    }

    free(wd.results);
    ls_htable_destroy(table);
}
END_TEST

START_TEST (ls_htable_collisions_test)
{
    ls_htable    *table;
    ls_hnode     *nodea, *nodeb, *node;

    // "09Vi" and "08vJ" collide, which WILL change if the string hashcode
    // function changes
    char *a = "09Vi";
    char *b = "08vJ";

    ck_assert(ls_htable_create(5,
                             ls_str_hashcode,
                             ls_str_compare,
                             &table,
                             NULL));

    ck_assert_int_eq(ls_str_hashcode(a), ls_str_hashcode(b));
    ck_assert(ls_htable_put(table, a, "1", NULL, NULL));
    ck_assert(ls_htable_get(table, b) == NULL);
    ck_assert(ls_htable_put(table, b, "2", NULL, NULL));
    ck_assert(ls_htable_get(table, b) != NULL);
    ck_assert(ls_htable_get(table, a) != NULL);
    ck_assert_int_eq(ls_htable_walk(table, test_htable_nullwalk, NULL), 2);
    ck_assert_int_eq(ls_htable_walk(table, test_htable_stopwalk, NULL), 1);

    nodea = ls_htable_get_node(table, a);
    nodeb = ls_htable_get_node(table, b);
    node = ls_htable_get_next_node(table, nodea);
    if (node != NULL)
    {
        ck_assert(node == nodeb);
    }
    else
    {
        node = ls_htable_get_next_node(table, nodeb);
        ck_assert(node == nodea);
    }

    ls_htable_remove(table, "non-existant");
    ls_htable_remove(table, a);
    ck_assert(ls_htable_put(table, a, "3", NULL, NULL));
    ls_htable_remove(table, b);
    ls_htable_remove(table, a);

    ls_htable_destroy(table);
}
END_TEST

START_TEST (ls_htable_cleaner_edges_test)
{
    ls_htable    *table;
    ls_err       err;

    ck_assert(ls_htable_create(-1,
                             ls_str_hashcode,
                             ls_str_compare,
                             &table,
                             &err));
    ck_assert_int_eq(ls_htable_walk(table, test_htable_nullwalk, NULL), 0);
    ck_assert(ls_htable_put(table, "key1", "value one", NULL, &err));
    ck_assert(ls_htable_put(table, "key1", "value one prime", NULL, &err));
    ck_assert(ls_htable_put(table, "key1", NULL, NULL, &err));
    ck_assert(ls_htable_put(table, "key1", "value one", NULL, &err));
    ls_htable_remove(table, "key1");
    ck_assert(ls_htable_put(table, "key1", NULL, NULL, &err));
    ls_htable_remove(table, "key1");
    ck_assert(ls_htable_put(table, "key1", "value one", NULL, &err));
    ls_htable_clear(table);
    ls_htable_destroy(table);
}
END_TEST

START_TEST (ls_htable_destroy_key_test)
{
    ls_htable    *table;
    ls_err       err;

    ck_assert(ls_htable_create(-1,
                             ls_str_hashcode,
                             ls_str_compare,
                             &table,
                             &err));
    key_destroy = false;
    char *k = strdup("key1");
    ck_assert(ls_htable_put(table, k, strdup("value one"), test_htable_destroy_key, &err));
    ck_assert(!key_destroy);
    ck_assert(ls_htable_put(table, k, strdup("value two"), test_htable_destroy_key, &err));
    ck_assert(!key_destroy);
    ck_assert(ls_htable_put(table, strdup(k), strdup("value two"), test_htable_destroy_key, &err));
    ck_assert(key_destroy);
    key_destroy = false;
    ls_htable_destroy(table);
    ck_assert(key_destroy);
 }
END_TEST

START_TEST (ls_htable_oom_test)
{
    OOM_SIMPLE_TEST(_oom_test(&err));
    OOM_TEST_INIT();
    OOM_TEST(NULL, _oom_test(NULL));
}
END_TEST

Suite * ls_htable_suite (void)
{
  Suite *s = suite_create ("ls_htable");
  {/* Htable test case */
      TCase *tc_ls_htable = tcase_create ("htable");

      suite_add_tcase (s, tc_ls_htable);
  }

  return s;
}
