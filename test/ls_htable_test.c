#include <check.h>

#include "ls_htable.h"

Suite * ls_htable_suite (void);

START_TEST (ls_htable_test)
{
    fail_if( false );
}
END_TEST



Suite * ls_htable_suite (void)
{
  Suite *s = suite_create ("ls_htable");
  {/* Htable test case */
      TCase *tc_ls_htable = tcase_create ("htable");
      tcase_add_test (tc_ls_htable, ls_htable_test);
      
      suite_add_tcase (s, tc_ls_htable);
  }

  return s;
}
