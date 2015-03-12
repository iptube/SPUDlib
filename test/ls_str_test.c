/*
 * Copyright (c) 2015 SPUDlib authors.  See LICENSE file.
 */

#include <check.h>

#include "../src/ls_str.h"

Suite * ls_str_suite (void);

START_TEST (ls_string_atoi)
{
    char num[] = "12";

    fail_unless( ls_atoi(num, 3) == 12 );
    fail_unless( ls_atoi(NULL, 3) == 3 );

}
END_TEST

START_TEST (ls_string_strlen)
{
    char test[] = "ABCDEFG";

    fail_unless( ls_strlen(NULL) == 0 );
    fail_unless( ls_strlen(test) == 7 );
}
END_TEST

START_TEST (ls_string_strnlen)
{
    char test[] = "ABCDEFG";

    fail_unless( ls_strnlen(NULL, 12) == 0 );
    fail_unless( ls_strnlen(test, 12) == 7 );
}
END_TEST

START_TEST (ls_string_strcmp)
{
    char a[] = "ABCDEFG";
    char aa[] = "ABCDEFG";
    char b[] = "ABCDEFGH";
    char bb[] = "ABCDEFGH";

    fail_unless( ls_strcmp(a,a) == 0 );
    fail_unless( ls_strcmp(b,b) == 0 );
    fail_unless( ls_strcmp(a,aa) == 0 );
    fail_unless( ls_strcmp(b,bb) == 0 );
    fail_if( ls_strcmp(a,b) == 0 );
    fail_if( ls_strcmp(aa,b) == 0 );
    fail_if( ls_strcmp(aa,bb) == 0 );
}
END_TEST

START_TEST (ls_string_strcasecmp)
{
    char a[] = "ABCDEFG";
    char aa[] = "abcdefg";
    char b[] = "ABCDEFGH";
    char bb[] = "abcdefgh";

    fail_unless( ls_strcasecmp(a,a) == 0 );
    fail_unless( ls_strcasecmp(b,b) == 0 );
    fail_unless( ls_strcasecmp(a,aa) == 0 );
    fail_unless( ls_strcasecmp(b,bb) == 0 );
    fail_if( ls_strcasecmp(a,b) == 0 );
    fail_if( ls_strcasecmp(aa,b) == 0 );
    fail_if( ls_strcasecmp(aa,bb) == 0 );
}
END_TEST

START_TEST (ls_string_strncmp)
{
    char a[] = "ABCDEFG";
    char aa[] = "ABCDEFG";
    char b[] = "ABCDEFGH";
    char bb[] = "ABCDEFGH";

    fail_unless( ls_strncmp(a,a,12) == 0 );
    fail_unless( ls_strncmp(b,b,3) == 0 );
    fail_unless( ls_strncmp(a,aa,15) == 0 );
    fail_unless( ls_strncmp(b,bb,4) == 0 );
    fail_if( ls_strncmp(a,b,17) == 0 );
    fail_if( ls_strncmp(aa,b,34) == 0 );
    fail_if( ls_strncmp(aa,bb,12) == 0 );
}
END_TEST

START_TEST (ls_str_atoi_test)
{
    ck_assert_int_eq(ls_atoi("24", 0), 24);
    ck_assert_int_eq(ls_atoi("-42", 0), -42);
    ck_assert_int_eq(ls_atoi("", 0), 0);
    ck_assert_int_eq(ls_atoi(NULL, 0), 0);

    ck_assert_int_eq(ls_atoi("24", 5), 24);
    ck_assert_int_eq(ls_atoi("-42", 5), -42);
    ck_assert_int_eq(ls_atoi("", 5), 0);
    ck_assert_int_eq(ls_atoi(NULL, 5), 5);
}
END_TEST

START_TEST (ls_str_compare_test)
{
    char *str1, *str2;

    str1 = str2 = "a test string";
    ck_assert(ls_strcmp(str1, str2) == 0);
    ck_assert(ls_strcasecmp(str1, str2) == 0);
    str2 = "b test string";
    ck_assert(ls_strcmp(str1, str2) < 0);
    ck_assert(ls_strcasecmp(str1, str2) < 0);
    str1 = "c test";
    ck_assert(ls_strcmp(str1, str2) > 0);
    ck_assert(ls_strcasecmp(str1, str2) > 0);

    str1 = "a test string";
    str2 = "A test String";
    ck_assert(ls_strcmp(str1, str2) > 0);
    ck_assert(ls_strcasecmp(str1, str2) == 0);

    ck_assert(ls_strcmp(NULL, NULL) == 0);
    ck_assert(ls_strcasecmp(NULL, NULL) == 0);

    ck_assert(ls_strcmp(str1, NULL) > 0);
    ck_assert(ls_strcasecmp(str1, NULL) > 0);

    ck_assert(ls_strcmp(NULL, str2) < 0);
    ck_assert(ls_strcasecmp(NULL, str2) < 0);
}
END_TEST

START_TEST (ls_str_ncompare_test)
{
    char *str1, *str2, *str2case;

    str1 = str2 = "test string alpha";
    str2case = "Test String AlphA";
    ck_assert(ls_strncmp(str1, str2, 17) == 0);
    ck_assert(ls_strncasecmp(str1, str2, 17) == 0);
    ck_assert(ls_strncasecmp(str1, str2case, 17) == 0);

    str2 = "test string beta";
    str2case = "Test String BetA";
    ck_assert(ls_strncmp(str1, str2, 17) < 0);
    ck_assert(ls_strncasecmp(str1, str2case, 17) < 0);

    str2 = "test string al";
    str2case = "Test String Al";
    ck_assert(ls_strncmp(str1, str2, 17) > 0);
    ck_assert(ls_strncmp(str1, str2case, 17) > 0);

    ck_assert(ls_strncmp(NULL, NULL, 10) == 0);
    ck_assert(ls_strncasecmp(NULL, NULL, 10) == 0);
    ck_assert(ls_strncmp(str1, NULL, 10) > 0);
    ck_assert(ls_strncasecmp(str1, NULL, 10) > 0);
    ck_assert(ls_strncmp(NULL, str2, 10) < 0);
    ck_assert(ls_strncasecmp(NULL, str2, 10) < 0);
}
END_TEST

START_TEST (ls_str_length_test)
{
    char *str;

    str = "a test string";
    ck_assert_int_eq(ls_strlen(str), 13);

    str = "";
    ck_assert_int_eq(ls_strlen(str), 0);

    str = "another test string";
    ck_assert_int_eq(ls_strlen(str), 19);

    ck_assert_int_eq(ls_strlen(NULL), 0);

    str = "a test string";
    ck_assert_int_eq(ls_strnlen(str, 13), 13);

    str = "a test string";
    ck_assert_int_eq(ls_strnlen(str, 17), 13);

    str = "";
    ck_assert_int_eq(ls_strnlen(str, 4), 0);

    str = "another test string";
    ck_assert_int_eq(ls_strnlen(str, 12), 12);

    ck_assert_int_eq(ls_strnlen(NULL, 4), 0);
}
END_TEST

Suite * ls_str_suite (void)
{
  Suite *s = suite_create ("ls_str");


  {/* string test case */
      TCase *tc_ls_string = tcase_create ("String");
      tcase_add_test (tc_ls_string, ls_string_atoi);
      tcase_add_test (tc_ls_string, ls_string_strlen);
      tcase_add_test (tc_ls_string, ls_string_strnlen);
      tcase_add_test (tc_ls_string, ls_string_strcmp);
      tcase_add_test (tc_ls_string, ls_string_strcasecmp);
      tcase_add_test (tc_ls_string, ls_string_strncmp);
      tcase_add_test (tc_ls_string, ls_str_atoi_test);
      tcase_add_test (tc_ls_string, ls_str_compare_test);
      tcase_add_test (tc_ls_string, ls_str_ncompare_test);
      tcase_add_test (tc_ls_string, ls_str_length_test);
      suite_add_tcase (s, tc_ls_string);
  }

  return s;
}
