/*
 * Copyright (c) 2015 SPUDlib authors.  See LICENSE file.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "test_utils.h"
#include "spud.h"

CTEST(spud, is_spud)
{
    int len = 1024;
    unsigned char buf[len];

    spud_header hdr;
    //should this be in init() instead?
    memcpy(hdr.magic, SpudMagicCookie, SPUD_MAGIC_COOKIE_SIZE);

    //copy the whole spud msg into the buffer..
    memcpy(buf, &hdr, sizeof(hdr));

    ASSERT_TRUE( spud_is_spud((const uint8_t *)&buf,len));
}

CTEST(spud, createId)
{
    int len = 1024;
    unsigned char buf[len];
    char idStr[len];
    spud_tube_id id;
    ls_err err;

    spud_header hdr;
    //should this be in init() instead?
    ASSERT_TRUE(spud_init(&hdr, NULL, &err));

    ASSERT_NULL(spud_id_to_string(idStr, 15, &hdr.tube_id));

    memset(idStr, 0, len);
    spud_id_to_string(idStr, len, &hdr.tube_id);
    ASSERT_EQUAL(strlen(idStr), SPUD_ID_STRING_SIZE);

    ASSERT_FALSE(spud_set_id(NULL, NULL, &err));
    ASSERT_FALSE(spud_set_id(&hdr, NULL, &err));

    spud_copy_id(&hdr.tube_id, &id);
    ASSERT_TRUE(spud_is_id_equal(&hdr.tube_id, &id));

    ASSERT_FALSE(spud_is_spud((const uint8_t *)&buf,len));


    //copy the whole spud msg into the buffer..
    memcpy(buf, &hdr, sizeof(hdr));

    ASSERT_TRUE(spud_is_spud((const uint8_t *)&buf,len));
}

CTEST(spud, isIdEqual)
{
    ls_err err;

    spud_header msgA;
    spud_header msgB;//Equal to A
    spud_header msgC;//New random

    ASSERT_TRUE(spud_init(&msgA, NULL, &err));
    ASSERT_TRUE(spud_init(&msgB, &msgA.tube_id, &err));
    ASSERT_TRUE(spud_init(&msgC, NULL, &err));

    ASSERT_TRUE( spud_is_id_equal(&msgA.tube_id, &msgB.tube_id));
    ASSERT_FALSE( spud_is_id_equal(&msgA.tube_id, &msgC.tube_id));
    ASSERT_FALSE( spud_is_id_equal(&msgB.tube_id, &msgC.tube_id));
}

CTEST(spud, parse)
{
    spud_message msg;
    ls_err err;
    uint8_t buf[] = { 0xd8, 0x00, 0x00, 0xd8,
                      0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
                      0x00,
                      0xa1, 0x00,
                      0x41, 0x61 };

    ASSERT_FALSE(spud_parse(NULL, 0, NULL, &err));
    ASSERT_FALSE(spud_parse(buf, 0, NULL, &err));
    ASSERT_FALSE(spud_parse(buf, 0, &msg, &err));
    ASSERT_TRUE(spud_parse(buf, 13, &msg, &err));
    ASSERT_FALSE(spud_parse(buf, 14, &msg, &err));
    ASSERT_FALSE(spud_parse(buf, 15, &msg, &err));
    ASSERT_TRUE(spud_parse(buf, sizeof(buf), &msg, &err));
    spud_unparse(&msg);
}
