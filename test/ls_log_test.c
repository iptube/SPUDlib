#include <check.h>

#include "ls_log.h"

Suite * ls_log_suite (void);

START_TEST (ls_log_test)
{
    fail_if( false );
}
END_TEST



Suite * ls_log_suite (void)
{
  Suite *s = suite_create ("ls_log");
  {/* Error test case */
      TCase *tc_ls_log = tcase_create ("log");
      tcase_add_test (tc_ls_log, ls_log_test);
      
      suite_add_tcase (s, tc_ls_log);
  }

  return s;
}
