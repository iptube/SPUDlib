#include <check.h>

#include "ls_mem.h"

Suite * ls_mem_suite (void);

START_TEST (ls_mem_test)
{
    fail_if( false );
}
END_TEST



Suite * ls_mem_suite (void)
{
  Suite *s = suite_create ("ls_mem");
  {/* Error test case */
      TCase *tc_ls_mem = tcase_create ("mem");
      tcase_add_test (tc_ls_mem, ls_mem_test);
      
      suite_add_tcase (s, tc_ls_mem);
  }

  return s;
}
