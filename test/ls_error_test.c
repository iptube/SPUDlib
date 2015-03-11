#include <check.h>

#include "ls_error.h"

Suite * ls_error_suite (void);

START_TEST (ls_error_test)
{
    fail_if( false );
}
END_TEST



Suite * ls_error_suite (void)
{
  Suite *s = suite_create ("ls_error");
  {/* Error test case */
      TCase *tc_ls_error = tcase_create ("Error");
      tcase_add_test (tc_ls_error, ls_error_test);
      
      suite_add_tcase (s, tc_ls_error);
  }

  return s;
}
