/*
 * Copyright (c) 2015 SPUDlib authors.  See LICENSE file.
 */
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "cn-cbor/cn-cbor.h"
#include "../src/cn-cbor/cn-encoder.h"
#include "cn-cbor/cn-create.h"
#include "ls_mem.h"

#include "test_utils.h"

typedef struct _buffer {
    size_t sz;
    unsigned char *ptr;
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

CTEST(cbor, error)
{
    ASSERT_STR(cn_cbor_error_str[CN_CBOR_NO_ERROR], "CN_CBOR_NO_ERROR");
    ASSERT_STR(cn_cbor_error_str[CN_CBOR_ERR_OUT_OF_DATA], "CN_CBOR_ERR_OUT_OF_DATA");
    ASSERT_STR(cn_cbor_error_str[CN_CBOR_ERR_NOT_ALL_DATA_CONSUMED], "CN_CBOR_ERR_NOT_ALL_DATA_CONSUMED");
    ASSERT_STR(cn_cbor_error_str[CN_CBOR_ERR_ODD_SIZE_INDEF_MAP], "CN_CBOR_ERR_ODD_SIZE_INDEF_MAP");
    ASSERT_STR(cn_cbor_error_str[CN_CBOR_ERR_BREAK_OUTSIDE_INDEF], "CN_CBOR_ERR_BREAK_OUTSIDE_INDEF");
    ASSERT_STR(cn_cbor_error_str[CN_CBOR_ERR_MT_UNDEF_FOR_INDEF], "CN_CBOR_ERR_MT_UNDEF_FOR_INDEF");
    ASSERT_STR(cn_cbor_error_str[CN_CBOR_ERR_RESERVED_AI], "CN_CBOR_ERR_RESERVED_AI");
    ASSERT_STR(cn_cbor_error_str[CN_CBOR_ERR_WRONG_NESTING_IN_INDEF_STRING], "CN_CBOR_ERR_WRONG_NESTING_IN_INDEF_STRING");
    ASSERT_STR(cn_cbor_error_str[CN_CBOR_ERR_INVALID_PARAMETER], "CN_CBOR_ERR_INVALID_PARAMETER");
    ASSERT_STR(cn_cbor_error_str[CN_CBOR_ERR_OUT_OF_MEMORY], "CN_CBOR_ERR_OUT_OF_MEMORY");
}

CTEST(cbor, parse)
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
        ASSERT_TRUE(parse_hex(tests[i], &b));
        cb = cn_cbor_decode(b.ptr, b.sz, NULL, &err);
        ASSERT_NOT_NULL(cb);

        enc_sz = cbor_encoder_write(encoded, 0, sizeof(encoded), cb);
        ASSERT_EQUAL(enc_sz, b.sz);
        ASSERT_EQUAL(memcmp(b.ptr, encoded, enc_sz), 0);
        free(b.ptr);
        cn_cbor_free(cb, NULL);
    }
}

typedef struct _cbor_failure
{
    char *hex;
    cn_cbor_error err;
} cbor_failure;

CTEST(cbor, fail)
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
        ASSERT_TRUE(parse_hex(tests[i].hex, &b));
        cb = cn_cbor_decode(b.ptr, b.sz, NULL, &err);
        ASSERT_NULL(cb);
        ASSERT_EQUAL(err.err, tests[i].err);

        free(b.ptr);
        cn_cbor_free(cb, NULL);
    }
}

// Decoder loses float size information
CTEST(cbor, float)
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
        ASSERT_TRUE(parse_hex(tests[i], &b));
        cb = cn_cbor_decode(b.ptr, b.sz, NULL, &err);
        ASSERT_NOT_NULL(cb);

        free(b.ptr);
        cn_cbor_free(cb, NULL);
    }
}

CTEST(cbor, getset)
{
    buffer b;
    const cn_cbor *cb;
    const cn_cbor *val;
    cn_cbor_errback err;

    ASSERT_TRUE(parse_hex("a3436363630262626201616100", &b));
    cb = cn_cbor_decode(b.ptr, b.sz, NULL, &err);
    ASSERT_NOT_NULL(cb);
    val = cn_cbor_mapget_string(cb, "a");
    ASSERT_NOT_NULL(val);
    val = cn_cbor_mapget_string(cb, "bb");
    ASSERT_NOT_NULL(val);
    val = cn_cbor_mapget_string(cb, "ccc");
    ASSERT_NOT_NULL(val);
    val = cn_cbor_mapget_string(cb, "b");
    ASSERT_NULL(val);
    free(b.ptr);
    cn_cbor_free(cb, NULL);

    ASSERT_TRUE(parse_hex("a2006161206162", &b));
    cb = cn_cbor_decode(b.ptr, b.sz, NULL, &err);
    ASSERT_NOT_NULL(cb);
    val = cn_cbor_mapget_int(cb, 0);
    ASSERT_NOT_NULL(val);
    val = cn_cbor_mapget_int(cb, -1);
    ASSERT_NOT_NULL(val);
    val = cn_cbor_mapget_int(cb, 1);
    ASSERT_NULL(val);
    free(b.ptr);
    cn_cbor_free(cb, NULL);

    ASSERT_TRUE(parse_hex("8100", &b));
    cb = cn_cbor_decode(b.ptr, b.sz, NULL, &err);
    ASSERT_NOT_NULL(cb);
    val = cn_cbor_index(cb, 0);
    ASSERT_NOT_NULL(val);
    val = cn_cbor_index(cb, 1);
    ASSERT_NULL(val);
    val = cn_cbor_index(cb, -1);
    ASSERT_NULL(val);
    free(b.ptr);
    cn_cbor_free(cb, NULL);
}

static void* cn_test_calloc(size_t count, size_t size, void *context) {
    ls_pool *pool = context;
    void *ret;
    ls_err err;

    assert(pool);
    assert(count > 0);
    assert(size > 0);
    if (!ls_pool_calloc(pool,
                       count,
                       size,
                       &ret,
                       &err)) {
       return NULL;
   }

    return ret;
}

static void dummy_free(void *ptr, void *context) {
    UNUSED_PARAM(ptr);
    UNUSED_PARAM(context);
}

CTEST(cbor, alloc)
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
    cn_cbor_context ctx;

    ASSERT_TRUE(ls_pool_create(2048, &pool, &lerr));
    ctx.calloc_func = cn_test_calloc;
    ctx.free_func = dummy_free;
    ctx.context = pool;

    for (i=0; i<sizeof(tests)/sizeof(char*); i++) {
        ASSERT_TRUE(parse_hex(tests[i], &b));
        cb = cn_cbor_decode(b.ptr, b.sz, &ctx, &err);
        ASSERT_NOT_NULL(cb);

        free(b.ptr);
    }
    ls_pool_destroy(pool);
}

// static const char *CBOR_TYPES[] = {
//       "CN_CBOR_NULL",
//       "CN_CBOR_FALSE",   "CN_CBOR_TRUE",
//       "CN_CBOR_UINT",    "CN_CBOR_INT",
//       "CN_CBOR_BYTES",   "CN_CBOR_TEXT",
//       "CN_CBOR_BYTES_CHUNKED",   "CN_CBOR_TEXT_CHUNKED", /* += 2 */
//       "CN_CBOR_ARRAY",   "CN_CBOR_MAP",
//       "CN_CBOR_TAG",
//       "CN_CBOR_SIMPLE",  "CN_CBOR_DOUBLE",
//       "CN_CBOR_INVALID"
// };

// static void print_cbor_tree(const cn_cbor *cb, int depth)
// {
//     cn_cbor *p = (cn_cbor*) cb;
//     const char *spaces = "                                                     ";
//
//     printf("%.*s%s\n", depth*2, spaces, CBOR_TYPES[p->type]);
//     for (p = p->first_child; p; p = p->next)
//     {
//         print_cbor_tree(p, depth+1);
//     }
// }

CTEST(cbor, create)
{
    cn_cbor_errback err;
    const cn_cbor* val;
    const char* data = "abc";
    cn_cbor *cb_map = cn_cbor_map_create(NULL, &err);
    cn_cbor *cb_int;
    cn_cbor *cb_data;

    ASSERT_NOT_NULL(cb_map);
    ASSERT_TRUE(err.err == CN_CBOR_NO_ERROR);

    cb_int = cn_cbor_int_create(256, NULL, &err);
    ASSERT_NOT_NULL(cb_int);
	ASSERT_TRUE(err.err == CN_CBOR_NO_ERROR);

	cb_data = cn_cbor_data_create(data, 4, NULL, &err);
	ASSERT_NOT_NULL(cb_data);
	ASSERT_TRUE(err.err == CN_CBOR_NO_ERROR);

    cn_cbor_mapput_int(cb_map, 5, cb_int, NULL, &err);
    ASSERT_TRUE(err.err == CN_CBOR_NO_ERROR);
    ASSERT_TRUE(cb_map->length == 2);

    cn_cbor_mapput_int(cb_map, 7, cb_data, NULL, &err);
    ASSERT_TRUE(err.err == CN_CBOR_NO_ERROR);
    ASSERT_TRUE(cb_map->length == 4);

    val = cn_cbor_mapget_int(cb_map, 5);
    ASSERT_NOT_NULL(val);
	ASSERT_TRUE(val->v.sint == 256);

    val = cn_cbor_mapget_int(cb_map, 7);
    ASSERT_NOT_NULL(val);
	ASSERT_STR(val->v.str, "abc");

    cn_cbor_free(cb_map, NULL);
}

CTEST(cbor, create_pool)
{
    cn_cbor_errback err;
    const char* data = "abc";
    cn_cbor *cb_map;
    ls_err lerr;
    ls_pool *pool;
    unsigned char buf[32];
    ssize_t encoded_size;
    unsigned char expected[] = "\xa2\x24\x19\x01\x00\x63" "foo\x63" "abc";
    cn_cbor_context ctx;

    ASSERT_TRUE(ls_pool_create(2048, &pool, &lerr));
    ctx.calloc_func = cn_test_calloc;
    ctx.free_func = dummy_free;
    ctx.context = pool;

    cb_map = cn_cbor_map_create(&ctx, &err);
    cn_cbor_mapput_int(cb_map, -5, cn_cbor_int_create(256, &ctx, &err), &ctx, &err);
    cn_cbor_mapput_string(cb_map, "foo", cn_cbor_string_create(data, &ctx, &err), &ctx, &err);

    encoded_size = cbor_encoder_write(buf, 0, sizeof(buf), cb_map);
    ASSERT_TRUE(encoded_size > 0);
    ASSERT_DATA(expected, sizeof(expected)-1, buf, encoded_size);

    ls_pool_destroy(pool);
}

CTEST(cbor, create_errors)
{
    cn_cbor_errback err;
    cn_cbor *ci;
    ci = cn_cbor_int_create(65536, NULL, &err);
    cn_cbor_mapput_int(ci, 12, NULL, NULL, &err);
    ASSERT_EQUAL(err.err, CN_CBOR_ERR_INVALID_PARAMETER);
    cn_cbor_mapput_string(ci, "foo", NULL, NULL, &err);
    ASSERT_EQUAL(err.err, CN_CBOR_ERR_INVALID_PARAMETER);
}
