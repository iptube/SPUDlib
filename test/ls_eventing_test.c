#include <check.h>

#include "ls_eventing.h"

Suite * ls_eventing_suite (void);

START_TEST (ls_eventing_test)
{
    fail_if( false );
}
END_TEST



Suite * ls_eventing_suite (void)
{
  Suite *s = suite_create ("ls_eventing");
  {/* eventing test case */
      TCase *tc_ls_eventing = tcase_create ("eventing");
      tcase_add_test (tc_ls_eventing, ls_eventing_test);
      
      suite_add_tcase (s, tc_ls_eventing);
  }

  return s;
}
