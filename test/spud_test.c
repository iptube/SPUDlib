#include <stdlib.h>
#include <stdio.h>

#include <netinet/in.h>
#include <netdb.h>

#include <check.h>


#include "spudlib.h"



void spudlib_setup (void);
void spudlib_teardown (void);
Suite * spudlib_suite (void);

void
spudlib_setup (void)
{

}



void
spudlib_teardown (void)
{
    
}



START_TEST (empty)
{
    
    fail_unless( 1==1,
                 "Test app fails");

}
END_TEST



Suite * spudlib_suite (void)
{
  Suite *s = suite_create ("sockaddr");

  {/* Core test case */
      TCase *tc_core = tcase_create ("Core");
      tcase_add_checked_fixture (tc_core, spudlib_setup, spudlib_teardown);
      tcase_add_test (tc_core, empty);
      suite_add_tcase (s, tc_core);
  }
  
  return s;
}
