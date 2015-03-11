
#include <check.h>

#include "ls_str.h"

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
      suite_add_tcase (s, tc_ls_string);
  }

  return s;
}
