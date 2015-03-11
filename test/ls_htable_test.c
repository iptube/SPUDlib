#include <string.h>
#include <check.h>

#include "ls_htable.h"
#include "ls_mem.h"

Suite * ls_htable_suite (void);

typedef struct _test_walk_data
{
    ls_htable           table;
    ls_htable_cmpfunc   cmp;
    unsigned int        visited;
    int                 *results;
} walk_data;

int test_htable_walk(void *data, const void *key, void *value)
{
    walk_data *wd = (walk_data*)data;

    wd->results[wd->visited] = (wd->cmp(ls_htable_get(wd->table, key), value) == 0);
    wd->visited++;

    return 1;
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
    ls_htable   table;
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
    ls_htable   table;
    ls_err      err;
    void                *pvalue;
    ck_assert(ls_htable_create(7,
                             ls_str_hashcode,
                             ls_str_compare,
                             &table,
                             &err));

    ck_assert(ls_htable_get_count(table) == 0);
    ck_assert(ls_htable_get(table, "key1") == NULL);
    ck_assert(ls_htable_get(table, "key2") == NULL);

    ck_assert(ls_htable_put(table, "key1", "value one", &pvalue, &err));
    ck_assert(pvalue == NULL);
    ck_assert(ls_htable_get_count(table) == 1);
    ck_assert(strcmp(ls_htable_get(table, "key1"), "value one") == 0);
    ck_assert(ls_htable_get(table, "key2") == NULL);

    ck_assert(ls_htable_put(table, "key2", "value two", &pvalue, &err));
    ck_assert(pvalue == NULL);
    ck_assert(ls_htable_get_count(table) == 2);
    ck_assert(strcmp(ls_htable_get(table, "key1"), "value one") == 0);
    ck_assert(strcmp(ls_htable_get(table, "key2"), "value two") == 0);

    ck_assert(ls_htable_put(table, "key1", "val 1", &pvalue, &err));
    ck_assert(strcmp((const char *)pvalue, "value one") == 0);
    ck_assert(ls_htable_get_count(table) == 2);
    ck_assert(strcmp(ls_htable_get(table, "key1"), "val 1") == 0);
    ck_assert(strcmp(ls_htable_get(table, "key2"), "value two") == 0);

    ck_assert(strcmp(ls_htable_remove(table, "key1"), "val 1") == 0);
    ck_assert(ls_htable_get_count(table) == 1);
    ck_assert(ls_htable_get(table, "key1") == NULL);
    ck_assert(strcmp(ls_htable_get(table, "key2"), "value two") == 0);

    ck_assert(ls_htable_put(table, "key1", "first value", &pvalue, &err));
    ck_assert(pvalue == NULL);
    ck_assert(ls_htable_get_count(table) == 2);
    ck_assert(strcmp(ls_htable_get(table, "key1"), "first value") == 0);
    ck_assert(strcmp(ls_htable_get(table, "key2"), "value two") == 0);

    ls_htable_clear(table, NULL, NULL);
    ck_assert(ls_htable_get_count(table) == 0);
    ck_assert(ls_htable_get(table, "key1") == NULL);
    ck_assert(ls_htable_get(table, "key2") == NULL);

    ls_htable_destroy(table);
}
END_TEST

START_TEST (ls_htable_node_test)
{
    ls_htable   table;
    ls_hnode    node;
    ls_err      err;
    ck_assert(ls_htable_create(7,
                             ls_str_hashcode,
                             ls_str_compare,
                             &table,
                             &err));

    ls_htable_put(table, "key1", "value one", NULL, &err);
    ls_htable_put(table, "key2", "value two", NULL, &err);
    ls_htable_put(table, "key3", "value three", NULL, &err);

    node = ls_htable_get_node(table, "key1");
    ck_assert(strcmp(ls_hnode_get_key(node), "key1") == 0);
    ck_assert(strcmp(ls_hnode_get_value(node), "value one") == 0);

    ck_assert(strcmp(ls_hnode_put_value(node, "val1"), "value one") == 0);
    ck_assert(strcmp(ls_hnode_get_value(node), "val1") == 0);

    ls_htable_clear(table, NULL, NULL);
    ls_htable_destroy(table);
}
END_TEST

START_TEST (ls_htable_iteration_test)
{
    ls_htable    table;
    ls_hnode     node;
    unsigned int count;
    ls_err       err;

    ck_assert(ls_htable_create(7,
                             ls_str_hashcode,
                             ls_str_compare,
                             &table,
                             &err));

    node = ls_htable_get_first_node(table);
    ck_assert(node == NULL);

    ls_htable_put(table, "key1", "value one", NULL, &err);
    ls_htable_put(table, "key2", "value two", NULL, &err);
    ls_htable_put(table, "key3", "value three", NULL, &err);

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

    ls_htable_clear(table, NULL, NULL);
    ls_htable_destroy(table);
}
END_TEST

START_TEST (ls_htable_walk_test)
{
    ls_htable    table;
    walk_data    wd;
    unsigned int idx;
    ls_err       err;

    ck_assert(ls_htable_create(7,
                             ls_str_hashcode,
                             ls_str_compare,
                             &table,
                             &err));

    ls_htable_put(table, "key1", "value one", NULL, &err);
    ls_htable_put(table, "key2", "value two", NULL, &err);
    ls_htable_put(table, "key3", "value three", NULL, &err);
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
    ls_htable_clear(table, NULL, NULL);
    ls_htable_destroy(table);
}
END_TEST

Suite * ls_htable_suite (void)
{
  Suite *s = suite_create ("ls_htable");
  {/* Htable test case */
      TCase *tc_ls_htable = tcase_create ("htable");
      tcase_add_test (tc_ls_htable, ls_htable_hash_str_test);
      tcase_add_test (tc_ls_htable, ls_htable_hash_strcase_test);
      tcase_add_test (tc_ls_htable, ls_htable_hash_int_test);
      tcase_add_test (tc_ls_htable, ls_htable_create_destroy_test);
      tcase_add_test (tc_ls_htable, ls_htable_basics_test);
      tcase_add_test (tc_ls_htable, ls_htable_node_test);
      tcase_add_test (tc_ls_htable, ls_htable_iteration_test);
      tcase_add_test (tc_ls_htable, ls_htable_walk_test);

      suite_add_tcase (s, tc_ls_htable);
  }

  return s;
}
