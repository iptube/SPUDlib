/*
 * Copyright (c) 2015 SPUDlib authors.  See LICENSE file.
 */

#include <string.h>
#include "test_utils.h"
#include "map.h"

CTEST(map, basic_create_and_free)
{
    spud_map_t *map;

    ASSERT_TRUE(spud_map_create(&map, NULL));

    spud_map_free(map);
}

CTEST(map, add_ip_address)
{
    spud_map_t map;
    const uint8_t ip[] = {192, 168, 0, 0};

    spud_map_reset(&map);

    ASSERT_TRUE(spud_map_create_ctx(&map, NULL));
    ASSERT_TRUE(spud_map_add_ip_address(&map, ip, sizeof ip, NULL));

    spud_map_free_ctx(&map);
}

CTEST(map, add_token)
{
    spud_map_t map;
    const uint8_t token[] = {0xd, 0xe, 0xa, 0xd, 0xb, 0xe, 0xe, 0xf};

    spud_map_reset(&map);

    ASSERT_TRUE(spud_map_create_ctx(&map, NULL));
    ASSERT_TRUE(spud_map_add_token(&map, token, sizeof token, NULL));

    spud_map_free_ctx(&map);
}

CTEST(map, add_url)
{
    spud_map_t map;
    const char *url = "http://www.example.org/spud";

    spud_map_reset(&map);

    ASSERT_TRUE(spud_map_create_ctx(&map, NULL));
    ASSERT_TRUE(spud_map_add_url(&map, url, NULL));

    spud_map_free_ctx(&map);
}

CTEST(map, add_warnings)
{
    spud_map_t map;

    struct warning_s {
        const char *tag;
        const char *msg;
    };

    struct warning_s warnings[] = {{"it", "avvertimento"},
                                   {"es", "advertencia"},
                                   {"de", "warnung"},
                                   {"fr", "avertissement"}};

    spud_map_reset(&map);
    ASSERT_TRUE(spud_map_create_ctx(&map, NULL));

    size_t i;
    for (i = 0; i < sizeof warnings / sizeof(struct warning_s); ++i) {
        ASSERT_TRUE(
            spud_map_add_warning(&map, warnings[i].tag, warnings[i].msg, NULL));
    }

    for (i = 0; i < sizeof warnings / sizeof(struct warning_s); ++i) {
        const char *msg = spud_map_get_warning(&map, warnings[i].tag);
        ASSERT_NOT_NULL(msg);
        ASSERT_STR(warnings[i].msg, msg);
    }

    spud_map_free_ctx(&map);
}

CTEST(map, encode_decode_check)
{
    spud_map_t map_out;
    spud_map_reset(&map_out);

    const uint8_t ip0[] = {192, 168, 0, 0};
    const uint8_t token0[] = {0xd, 0xe, 0xa, 0xd, 0xb, 0xe, 0xe, 0xf};
    const char *url = "http://www.example.org/spud";

    ASSERT_TRUE(spud_map_create_ctx(&map_out, NULL));
    ASSERT_TRUE(spud_map_add_ip_address(&map_out, ip0, sizeof ip0, NULL));
    ASSERT_TRUE(spud_map_add_token(&map_out, token0, sizeof token0, NULL));
    ASSERT_TRUE(spud_map_add_url(&map_out, url, NULL));

    spud_map_free_ctx(&map_out);

    uint8_t out[SPUD_MAP_MAX_SERIALIZED_SZ];
    size_t out_sz = sizeof out;

    spud_map_t map_in;
    spud_map_reset(&map_in);

    ASSERT_TRUE(spud_map_encode(&map_out, out, &out_sz, NULL));
    ASSERT_TRUE(spud_map_decode(out, out_sz, &map_in, NULL));

    // Check decoded is as expected
    ASSERT_STR(url, spud_map_get_url(&map_in));

    size_t ip1_sz;
    const uint8_t *ip1 = spud_map_get_ip_address(&map_in, &ip1_sz);
    ASSERT_NOT_NULL(ip1);                       // got something
    ASSERT_EQUAL(sizeof ip0, ip1_sz);           // same length as the original
    ASSERT_DATA(ip0, sizeof ip0, ip1, ip1_sz);  // and same contents

    size_t token1_sz;
    const uint8_t *token1 = spud_map_get_token(&map_in, &token1_sz);
    ASSERT_NOT_NULL(token1);  // ditto
    ASSERT_EQUAL(sizeof token0, token1_sz);
    ASSERT_DATA(token0, sizeof token0, token1, token1_sz);

    spud_map_free_ctx(&map_in);
}
