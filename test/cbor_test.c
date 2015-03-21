/*
 * Copyright (c) 2015 SPUDlib authors.  See LICENSE file.
 */
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#include <check.h>
#include "cn-cbor/cn-cbor.h"
#include "../src/cn-cbor/cn-encoder.h"
#include "ls_mem.h"

Suite * cbor_suite (void);

typedef struct _buffer {
    size_t sz;
    char *ptr;
} buffer;

static bool parse_hex(char *inp, buffer *b)
{
    int len = strlen(inp);
    size_t i;
    if (len%2 != 0) {
        b->sz = -1;
        b->ptr = NULL;
        return false;
    }
    b->sz  = len / 2;
    b->ptr = malloc(b->sz);
    for (i=0; i<b->sz; i++) {
        sscanf(inp+(2*i), "%02hhx", &b->ptr[i]);
    }
    return true;
}

START_TEST (cbor_error_test)
{
    ck_assert_str_eq(cn_cbor_error_str[CN_CBOR_NO_ERROR], "CN_CBOR_NO_ERROR");
    ck_assert_str_eq(cn_cbor_error_str[CN_CBOR_ERR_OUT_OF_DATA], "CN_CBOR_ERR_OUT_OF_DATA");
    ck_assert_str_eq(cn_cbor_error_str[CN_CBOR_ERR_NOT_ALL_DATA_CONSUMED], "CN_CBOR_ERR_NOT_ALL_DATA_CONSUMED");
    ck_assert_str_eq(cn_cbor_error_str[CN_CBOR_ERR_ODD_SIZE_INDEF_MAP], "CN_CBOR_ERR_ODD_SIZE_INDEF_MAP");
    ck_assert_str_eq(cn_cbor_error_str[CN_CBOR_ERR_BREAK_OUTSIDE_INDEF], "CN_CBOR_ERR_BREAK_OUTSIDE_INDEF");
    ck_assert_str_eq(cn_cbor_error_str[CN_CBOR_ERR_MT_UNDEF_FOR_INDEF], "CN_CBOR_ERR_MT_UNDEF_FOR_INDEF");
    ck_assert_str_eq(cn_cbor_error_str[CN_CBOR_ERR_RESERVED_AI], "CN_CBOR_ERR_RESERVED_AI");
    ck_assert_str_eq(cn_cbor_error_str[CN_CBOR_ERR_WRONG_NESTING_IN_INDEF_STRING], "CN_CBOR_ERR_WRONG_NESTING_IN_INDEF_STRING");
    ck_assert_str_eq(cn_cbor_error_str[CN_CBOR_ERR_OUT_OF_MEMORY], "CN_CBOR_ERR_OUT_OF_MEMORY");
}
END_TEST

START_TEST (cbor_parse_test)
{
    cn_cbor_errback err;
    char *tests[] = {
        "00",       // 0
        "01",       // 1
        "17",       // 23
        "1818",     // 24
        "190100",   // 256
        "1a00010000", // 65536
        "1b0000000100000000", // 4294967296
        "20",       // -1
        "37",       // -24
        "3818",     // -25
        "390100",   // -257
        "3a00010000", // -65537
        "3b0000000100000000", // -4294967297
        "4161",     // h"a"
        "6161",     // "a"
        "8100",     // [0]
        "818100",   // [[0]]
        "a1616100",	// {"a":0}
        "d8184100", // tag
        "f4",	    // false
        "f5",	    // true
        "f6",	    // null
        "f7",	    // undefined
        "f8ff",     // simple(255)
        "fb3ff199999999999a", // 1.1
        "fb7ff8000000000000", // NaN
        "5f42010243030405ff", // (_ h'0102', h'030405')
        "7f61616161ff", // (_ "a", "a")
        "9fff",     // [_ ]
        "bf61610161629f0203ffff", // {_ "a": 1, "b": [_ 2, 3]}
    };
    const cn_cbor *cb;
    buffer b;
    size_t i;
    unsigned char encoded[1024];
    ssize_t enc_sz;

    for (i=0; i<sizeof(tests)/sizeof(char*); i++) {
        ck_assert(parse_hex(tests[i], &b));
        cb = cn_cbor_decode(b.ptr, b.sz, NULL, NULL, &err);
        ck_assert_msg(cb != NULL, tests[i]);

        enc_sz = cbor_encoder_write(encoded, 0, sizeof(encoded), cb);
        ck_assert_int_eq(enc_sz, b.sz);
        ck_assert_int_eq(memcmp(b.ptr, encoded, enc_sz), 0);
        free(b.ptr);
        cn_cbor_free(cb);
    }
}
END_TEST

typedef struct _cbor_failure
{
    char *hex;
    cn_cbor_error err;
} cbor_failure;

START_TEST (cbor_fail_test)
{
    cn_cbor_errback err;
    cbor_failure tests[] = {
        {"81", CN_CBOR_ERR_OUT_OF_DATA},
        {"0000", CN_CBOR_ERR_NOT_ALL_DATA_CONSUMED},
        {"bf00ff", CN_CBOR_ERR_ODD_SIZE_INDEF_MAP},
        {"ff", CN_CBOR_ERR_BREAK_OUTSIDE_INDEF},
        {"1f", CN_CBOR_ERR_MT_UNDEF_FOR_INDEF},
        {"1c", CN_CBOR_ERR_RESERVED_AI},
        {"7f4100", CN_CBOR_ERR_WRONG_NESTING_IN_INDEF_STRING},
    };
    const cn_cbor *cb;
    buffer b;
    size_t i;

    for (i=0; i<sizeof(tests)/sizeof(cbor_failure); i++) {
        ck_assert(parse_hex(tests[i].hex, &b));
        cb = cn_cbor_decode(b.ptr, b.sz, NULL, NULL, &err);
        ck_assert_msg(cb == NULL, tests[i].hex);
        ck_assert_int_eq(err.err,tests[i].err   );

        free(b.ptr);
        cn_cbor_free(cb);
    }
}
END_TEST

// Decoder loses float size information
START_TEST (cbor_float_test)
{
    cn_cbor_errback err;
    char *tests[] = {
        "f9c400", // -4.0
        "fa47c35000", // 100000.0
        "f97e00", // Half NaN, half beast
        "f9fc00", // -Inf
        "f97c00", // Inf
    };
    const cn_cbor *cb;
    buffer b;
    size_t i;

    for (i=0; i<sizeof(tests)/sizeof(char*); i++) {
        ck_assert(parse_hex(tests[i], &b));
        cb = cn_cbor_decode(b.ptr, b.sz, NULL, NULL, &err);
        ck_assert_msg(cb != NULL, tests[i]);

        free(b.ptr);
        cn_cbor_free(cb);
    }
}
END_TEST

START_TEST (cbor_getset_test)
{
    buffer b;
    const cn_cbor *cb;
    const cn_cbor *val;
    cn_cbor_errback err;

    ck_assert(parse_hex("a3436363630262626201616100", &b));
    cb = cn_cbor_decode(b.ptr, b.sz, NULL, NULL, &err);
    ck_assert(cb!=NULL);
    val = cn_cbor_mapget_string(cb, "a");
    ck_assert(val != NULL);
    val = cn_cbor_mapget_string(cb, "bb");
    ck_assert(val != NULL);
    val = cn_cbor_mapget_string(cb, "ccc");
    ck_assert(val != NULL);
    val = cn_cbor_mapget_string(cb, "b");
    ck_assert(val == NULL);
    free(b.ptr);
    cn_cbor_free(cb);

    ck_assert(parse_hex("a2006161206162", &b));
    cb = cn_cbor_decode(b.ptr, b.sz, NULL, NULL, &err);
    ck_assert(cb!=NULL);
    val = cn_cbor_mapget_int(cb, 0);
    ck_assert(val != NULL);
    val = cn_cbor_mapget_int(cb, -1);
    ck_assert(val != NULL);
    val = cn_cbor_mapget_int(cb, 1);
    ck_assert(val == NULL);
    free(b.ptr);
    cn_cbor_free(cb);

    ck_assert(parse_hex("8100", &b));
    cb = cn_cbor_decode(b.ptr, b.sz, NULL, NULL, &err);
    ck_assert(cb!=NULL);
    val = cn_cbor_index(cb, 0);
    ck_assert(val != NULL);
    val = cn_cbor_index(cb, 1);
    ck_assert(val == NULL);
    val = cn_cbor_index(cb, -1);
    ck_assert(val == NULL);
    free(b.ptr);
    cn_cbor_free(cb);
}
END_TEST

static void* cn_test_alloc(size_t count, size_t size, void *context) {
    ls_pool *pool = context;
    void *ret;
    ls_err err;
    fail_unless(ls_pool_calloc(pool, count, size, &ret, &err));
    return ret;
}

START_TEST (cbor_alloc_test)
{
    cn_cbor_errback err;
    char *tests[] = {
        "f9c400", // -4.0
        "fa47c35000", // 100000.0
        "f97e00", // Half NaN, half beast
        "f9fc00", // -Inf
        "f97c00", // Inf
    };
    const cn_cbor *cb;
    buffer b;
    size_t i;
    ls_pool *pool;
    ls_err lerr;

    fail_unless(ls_pool_create(2048, &pool, &lerr));

    for (i=0; i<sizeof(tests)/sizeof(char*); i++) {
        ck_assert(parse_hex(tests[i], &b));
        cb = cn_cbor_decode(b.ptr, b.sz, cn_test_alloc, pool, &err);
        ck_assert_msg(cb != NULL, tests[i]);

        free(b.ptr);
    }
    ls_pool_destroy(pool);
}
END_TEST

Suite * cbor_suite (void)
{
    Suite *s = suite_create ("cbor");
    {
        TCase *tc_cbor_parse = tcase_create ("parse");
        tcase_add_test (tc_cbor_parse, cbor_error_test);
        tcase_add_test (tc_cbor_parse, cbor_parse_test);
        tcase_add_test (tc_cbor_parse, cbor_fail_test);
        tcase_add_test (tc_cbor_parse, cbor_float_test);
        tcase_add_test (tc_cbor_parse, cbor_getset_test);
        tcase_add_test (tc_cbor_parse, cbor_alloc_test);

        suite_add_tcase (s, tc_cbor_parse);
    }
    return s;
}
