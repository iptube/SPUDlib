/*
 * Copyright (c) 2015 SPUDlib authors.  See LICENSE file.
 */

#include <stdlib.h>
#include <stdio.h>

#include <check.h>

#include "spud.h"

Suite * spud_suite (void);

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

    spud_header hdr;
    //should this be in init() instead?
    memcpy(hdr.magic, SpudMagicCookie, SPUD_MAGIC_COOKIE_SIZE);

    //copy the whole spud msg into the buffer..
    memcpy(buf, &hdr, sizeof(hdr));

    fail_unless( spud_is_spud((const uint8_t *)&buf,len),
                 "isSpud() failed");
}
END_TEST

START_TEST (createId)
{
    int len = 1024;
    unsigned char buf[len];
    char idStr[len];
    spud_tube_id id;
    ls_err err;

    spud_header hdr;
    //should this be in init() instead?
    fail_unless(spud_init(&hdr, NULL, &err));

    ck_assert(spud_id_to_string(idStr, 15, &hdr.tube_id) == NULL);

    memset(idStr, 0, len);
    spud_id_to_string(idStr, len, &hdr.tube_id);
    ck_assert_int_eq(strlen(idStr), SPUD_ID_STRING_SIZE);

    fail_if(spud_set_id(NULL, NULL, &err));
    fail_if(spud_set_id(&hdr, NULL, &err));

    spud_copy_id(&hdr.tube_id, &id);
    fail_unless(spud_is_id_equal(&hdr.tube_id, &id));

    fail_if(spud_is_spud((const uint8_t *)&buf,len),
            "isSpud() failed");


    //copy the whole spud msg into the buffer..
    memcpy(buf, &hdr, sizeof(hdr));

    fail_unless(spud_is_spud((const uint8_t *)&buf,len),
                "isSpud() failed");
}
END_TEST

START_TEST (isIdEqual)
{
    ls_err err;

    spud_header msgA;
    spud_header msgB;//Equal to A
    spud_header msgC;//New random

    fail_unless(spud_init(&msgA, NULL, &err));
    fail_unless(spud_init(&msgB, &msgA.tube_id, &err));
    fail_unless(spud_init(&msgC, NULL, &err));

    fail_unless( spud_is_id_equal(&msgA.tube_id, &msgB.tube_id));
    fail_if( spud_is_id_equal(&msgA.tube_id, &msgC.tube_id));
    fail_if( spud_is_id_equal(&msgB.tube_id, &msgC.tube_id));
}
END_TEST

START_TEST (spud_parse_test)
{
    spud_message msg;
    ls_err err;
    uint8_t buf[] = { 0xd8, 0x00, 0x00, 0xd8,
                      0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
                      0x00,
                      0xa1, 0x00,
                      0x41, 0x61 };

    fail_if(spud_parse(NULL, 0, NULL, &err));
    fail_if(spud_parse(buf, 0, NULL, &err));
    fail_if(spud_parse(buf, 0, &msg, &err));
    fail_unless(spud_parse(buf, 13, &msg, &err));
    fail_if(spud_parse(buf, 14, &msg, &err));
    fail_if(spud_parse(buf, 15, &msg, &err));
    fail_unless(spud_parse(buf, sizeof(buf), &msg, &err));
    spud_unparse(&msg);
}
END_TEST

Suite * spud_suite (void)
{
  Suite *s = suite_create ("spud");

  {/* Core test case */
      TCase *tc_core = tcase_create ("Core");
      tcase_add_test (tc_core, empty);
      tcase_add_test (tc_core, is_spud);
      tcase_add_test (tc_core, createId);
      tcase_add_test (tc_core, isIdEqual);
      tcase_add_test (tc_core, spud_parse_test);

      suite_add_tcase (s, tc_core);
  }

  return s;
}
