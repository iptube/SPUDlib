#include <stdlib.h>
#include <stdio.h>

#include <netinet/in.h>
#include <netdb.h>

#include <check.h>

#include "spudlib.h"
#include "ls_error.h"

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

START_TEST (is_spud)
{
    int len = 1024;
    unsigned char buf[len];

    spud_header_t hdr;
    //should this be in init() instead?
    memcpy(hdr.magic, SpudMagicCookie, SPUD_MAGIC_COOKIE_SIZE);

    //copy the whole spud msg into the buffer..
    memcpy(buf, &hdr, sizeof(hdr));

    fail_unless( spud_isSpud((const uint8_t *)&buf,len),
                 "isSpud() failed");
}
END_TEST

START_TEST (createId)
{
    int len = 1024;
    unsigned char buf[len];
    char idStr[len];
    ls_err err;

    spud_header_t hdr;
    //should this be in init() instead?
    fail_unless(spud_init(&hdr, NULL, &err));

    printf("ID: %s\n", spud_idToString(idStr, len, &hdr.tube_id, NULL));

    fail_if(spud_isSpud((const uint8_t *)&buf,len),
            "isSpud() failed");


    //copy the whole spud msg into the buffer..
    memcpy(buf, &hdr, sizeof(hdr));

    fail_unless(spud_isSpud((const uint8_t *)&buf,len),
                "isSpud() failed");
}
END_TEST

START_TEST (isIdEqual)
{
    ls_err err;

    spud_header_t msgA;
    spud_header_t msgB;//Equal to A
    spud_header_t msgC;//New random

    //should this be in init() instead?
    fail_unless(spud_init(&msgA, NULL, &err));
    fail_unless(spud_init(&msgB, &msgA.tube_id, &err));
    fail_unless(spud_init(&msgC, NULL, &err));

    fail_unless( spud_isIdEqual(&msgA.tube_id, &msgB.tube_id));
    fail_if( spud_isIdEqual(&msgA.tube_id, &msgC.tube_id));
    fail_if( spud_isIdEqual(&msgB.tube_id, &msgC.tube_id));
}
END_TEST



Suite * spudlib_suite (void)
{
  Suite *s = suite_create ("sockaddr");

  {/* Core test case */
      TCase *tc_core = tcase_create ("Core");
      tcase_add_checked_fixture (tc_core, spudlib_setup, spudlib_teardown);
      tcase_add_test (tc_core, empty);
      tcase_add_test (tc_core, is_spud);
      tcase_add_test (tc_core, createId);
      tcase_add_test (tc_core, isIdEqual);

      suite_add_tcase (s, tc_core);
  }

  return s;
}
