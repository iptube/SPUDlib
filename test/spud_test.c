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

	spud_header_t hdr;
	//should this be in init() instead?
	spud_init(&hdr, NULL);

	printf("ID: %s\n", spud_idToString(idStr, len, &hdr.flags_id));

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
	spud_header_t msgA;
	spud_header_t msgB;//Equal to A
	spud_header_t msgC;//New random

	//should this be in init() instead?
	spud_init(&msgA, NULL);
	spud_init(&msgB, &msgA.flags_id);
	spud_init(&msgC, NULL);

	fail_unless( spud_isIdEqual(&msgA.flags_id, &msgB.flags_id));
	fail_if( spud_isIdEqual(&msgA.flags_id, &msgC.flags_id));
	fail_if( spud_isIdEqual(&msgB.flags_id, &msgC.flags_id));
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
