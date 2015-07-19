/*
 * Copyright (c) 2015 SPUDlib authors.  See LICENSE file.
 */

#include "map.h"

#include <assert.h>
#include <errno.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>

#include "config.h"
#include "ls_log.h"
#include "ls_sockaddr.h"
#include "cn-cbor/cn-cbor.h"

static spud_map_t *spud_map_alloc(void);
static bool cbor_map_add_string(cn_cbor *map, cn_cbor_context *ctx,
                                const char *key, const char *str, ls_err *err);
static const char *cbor_map_get_string(cn_cbor *map, const char *key);
static bool spud_map_add_data(spud_map_t *map, const char *key,
                              const uint8_t *data, size_t data_sz, ls_err *err);
static bool spud_map_add_string(spud_map_t *map, const char *key,
                                const char *str, ls_err *err);
static const uint8_t *spud_map_get_data(spud_map_t *map, const char *key,
                                        size_t *pdata_sz);
static cn_cbor *spud_map_create_warnings(spud_map_t *map);
static bool spud_map_add_int(spud_map_t *map, const char *key, int64_t val,
                             ls_err *err);

//
// spud_map_t life cycle
//

LS_API void spud_map_reset(spud_map_t *map)
{
    assert(map);

    map->ctx.calloc_func = NULL;
    map->ctx.free_func = NULL;
    map->ctx.context = NULL;

    map->cbor = NULL;
}

LS_API bool spud_map_create_ctx(spud_map_t *map, ls_err *err)
{
    assert(map != NULL);

    if (!ls_pool_create(128, (ls_pool **)&map->ctx.context, err)) {
        LS_ERROR(err, LS_ERR_NO_MEMORY);
        return false;
    }

    if ((map->cbor = cn_cbor_map_create(&map->ctx, NULL)) == NULL) {
        ls_pool_destroy(map->ctx.context);
        LS_ERROR(err, LS_ERR_NO_MEMORY);
        return false;
    }

    return true;
}

LS_API bool spud_map_create(spud_map_t **pmap, ls_err *err)
{
    assert(pmap != NULL);

    spud_map_t *map = spud_map_alloc();

    if (map == NULL) {
        LS_ERROR(err, LS_ERR_NO_MEMORY);
        return false;
    }

    if (!spud_map_create_ctx(map, err)) {
        spud_map_free(map);
        return false;
    }

    // Copy out the handler
    *pmap = map;

    return true;
}

LS_API void spud_map_free_ctx(spud_map_t *map)
{
    assert(map != NULL);

    // We don't need to explicitly free the .cbor member since it'll be
    // deallocated along with the pool.
    if (map->ctx.context) {
        ls_pool_destroy(map->ctx.context);
        map->ctx.context = NULL;
    }
}

LS_API void spud_map_free(spud_map_t *map)
{
    if (map) {
        spud_map_free_ctx(map);
        ls_data_free(map);
    }
}

//
// spud_map_t setters
//
LS_API bool spud_map_add_inactivity_timer(spud_map_t *map, int64_t msec,
                                          ls_err *err)
{
    return spud_map_add_int(map, "inactivity-timer", msec, err);
}

LS_API bool spud_map_add_icmp_type(spud_map_t *map, int64_t icmp_type,
                                   ls_err *err)
{
    return spud_map_add_int(map, "icmp-type", icmp_type, err);
}

LS_API bool spud_map_add_icmp_code(spud_map_t *map, int64_t icmp_code,
                                   ls_err *err)
{
    return spud_map_add_int(map, "icmp-code", icmp_code, err);
}

LS_API bool spud_map_add_translated_external_address(spud_map_t *map,
                                                     const uint8_t *ip_addr,
                                                     size_t ip_addr_sz,
                                                     ls_err *err)
{
    return spud_map_add_data(map, "translated-external-address", ip_addr,
                             ip_addr_sz, err);
}

LS_API bool spud_map_add_internal_address(spud_map_t *map,
                                          const uint8_t *ip_addr,
                                          size_t ip_addr_sz, ls_err *err)
{
    return spud_map_add_data(map, "internal-address", ip_addr, ip_addr_sz, err);
}

LS_API bool spud_map_add_translated_external_port(spud_map_t *map,
                                                  uint16_t port, ls_err *err)
{
    return spud_map_add_int(map, "translated-external-port", port, err);
}

LS_API bool spud_map_add_internal_port(spud_map_t *map, uint16_t port,
                                       ls_err *err)
{
    return spud_map_add_int(map, "internal-port", port, err);
}

LS_API bool spud_map_add_path_element_description(spud_map_t *map,
                                                  const char *description,
                                                  ls_err *err)
{
    return spud_map_add_string(map, "description", description, err);
}

LS_API bool spud_map_add_path_element_version(spud_map_t *map,
                                              const char *version, ls_err *err)
{
    return spud_map_add_string(map, "version", version, err);
}

LS_API bool spud_map_add_path_element_caps(spud_map_t *map, const uint8_t *caps,
                                           size_t caps_sz, ls_err *err)
{
    return spud_map_add_data(map, "caps", caps, caps_sz, err);
}

LS_API bool spud_map_add_path_element_ttl(spud_map_t *map, int64_t ttl,
                                          ls_err *err)
{
    return spud_map_add_int(map, "ttl", ttl, err);
}

LS_API bool spud_map_add_mtu(spud_map_t *map, int64_t mtu, ls_err *err)
{
    return spud_map_add_int(map, "mtu", mtu, err);
}

LS_API bool spud_map_add_max_byte_rate(spud_map_t *map, int64_t mbr,
                                       ls_err *err)
{
    return spud_map_add_int(map, "max-byte-rate", mbr, err);
}

LS_API bool spud_map_add_max_packet_rate(spud_map_t *map, int64_t mpr,
                                         ls_err *err)
{
    return spud_map_add_int(map, "max-packet-rate", mpr, err);
}

LS_API bool spud_map_add_latency(spud_map_t *map, int64_t msec, ls_err *err)
{
    return spud_map_add_int(map, "latency", msec, err);
}

LS_API bool spud_map_add_icmp(spud_map_t *map, const uint8_t *icmp_pkt,
                              size_t icmp_pkt_sz, ls_err *err)
{
    return spud_map_add_data(map, "icmp", icmp_pkt, icmp_pkt_sz, err);
}

LS_API bool spud_map_add_ip_address(spud_map_t *map, const uint8_t *ip_addr,
                                    size_t ip_addr_sz, ls_err *err)
{
    return spud_map_add_data(map, "ipaddr", ip_addr, ip_addr_sz, err);
}

LS_API bool spud_map_add_token(spud_map_t *map, const uint8_t *token,
                               size_t token_sz, ls_err *err)
{
    return spud_map_add_data(map, "token", token, token_sz, err);
}

LS_API bool spud_map_add_url(spud_map_t *map, const char *url, ls_err *err)
{
    return spud_map_add_string(map, "url", url, err);
}

LS_API bool spud_map_add_warning(spud_map_t *map, const char *lang_tag,
                                 const char *msg, ls_err *err)
{
    cn_cbor *warning_map =
        (cn_cbor *)cn_cbor_mapget_string(map->cbor, "warning");

    if (warning_map == NULL) {
        // Create a new warning map and stick it into the cbor map
        if ((warning_map = spud_map_create_warnings(map)) == NULL) {
            LS_ERROR(err, LS_ERR_NO_MEMORY);
            return false;
        }
    }

    return cbor_map_add_string(warning_map, &map->ctx, lang_tag, msg, err);
}

//
// spud_map_t getters
//

LS_API const char *spud_map_get_url(spud_map_t *map)
{
    assert(map != NULL);
    assert(map->cbor != NULL);

    return cbor_map_get_string(map->cbor, "url");
}

LS_API const uint8_t *spud_map_get_ip_address(spud_map_t *map,
                                              size_t *pip_addr_sz)
{
    assert(map != NULL);
    assert(map->cbor != NULL);
    assert(pip_addr_sz != NULL);

    return spud_map_get_data(map, "ipaddr", pip_addr_sz);
}

LS_API const uint8_t *spud_map_get_token(spud_map_t *map, size_t *ptoken_sz)
{
    assert(map != NULL);
    assert(map->cbor != NULL);
    assert(ptoken_sz != NULL);

    return spud_map_get_data(map, "token", ptoken_sz);
}

LS_API const char *spud_map_get_warning(spud_map_t *map, const char *lang_tag)
{
    assert(map != NULL);
    assert(map->cbor != NULL);

    cn_cbor *warning_map =
        (cn_cbor *)cn_cbor_mapget_string(map->cbor, "warning");

    if (warning_map == NULL) {
        return NULL;
    }

    return cbor_map_get_string(warning_map, lang_tag);
}

//
// spud_map_t encode/decode
//

LS_API bool spud_map_encode(const spud_map_t *map, uint8_t *out, size_t *out_sz,
                            ls_err *err)
{
    assert(map != NULL);
    assert(out != NULL);
    assert(out_sz != NULL);

    ssize_t rc = cbor_encoder_write(out, 0, *out_sz, map->cbor);

    if (rc < 0) {
        LS_ERROR(err, LS_ERR_OVERFLOW);
        return false;
    }

    *out_sz = (size_t)rc;

    return true;
}

// - wants a pre-allocated spud map
// - caller is responsible to check the semantic consistency of the map
LS_API bool spud_map_decode(const uint8_t *buf, size_t buf_sz, spud_map_t *map,
                            ls_err *err)
{
    assert(map != NULL);

    cn_cbor_errback cbor_err;
    cn_cbor *cbor =
        (cn_cbor *)cn_cbor_decode(buf, buf_sz, &map->ctx, &cbor_err);

    if (cbor == NULL) {
        if (err) {
            err->code = LS_ERR_USER + cbor_err.err;
            err->message = cn_cbor_error_str[cbor_err.err];
            err->function = __func__;
            err->file = __FILE__;
            err->line = __LINE__;
        }
        return false;
    }

    map->cbor = cbor, cbor = NULL;

    return true;
}

//
// private stuff
//

static spud_map_t *spud_map_alloc(void)
{
    spud_map_t *map = ls_data_malloc(sizeof(spud_map_t));

    if (map) {
        spud_map_reset(map);
    }

    return map;
}

static bool spud_map_add_int(spud_map_t *map, const char *key, int64_t i,
                             ls_err *err)
{
    assert(map != NULL);

    if (key) {
        cn_cbor *val = cn_cbor_int_create(i, &map->ctx, NULL);
        if (!val ||
            !cn_cbor_mapput_string(map->cbor, key, val, &map->ctx, NULL)) {
            LS_ERROR(err, LS_ERR_NO_MEMORY);
            return false;
        }
    }

    return true;
}

static bool cbor_map_add_string(cn_cbor *map, cn_cbor_context *ctx,
                                const char *key, const char *str, ls_err *err)
{
    assert(map != NULL);
    assert(ctx != NULL);

    if (key && str) {
        cn_cbor *val = cn_cbor_string_create(str, ctx, NULL);
        if (!val || !cn_cbor_mapput_string(map, key, val, ctx, NULL)) {
            LS_ERROR(err, LS_ERR_NO_MEMORY);
            return false;
        }
    }

    return true;
}

// naming is not perfect (string refers to the value; the type of the key is
// implicit...)
static bool spud_map_add_string(spud_map_t *map, const char *key,
                                const char *str, ls_err *err)
{
    assert(map != NULL);

    return cbor_map_add_string(map->cbor, &map->ctx, key, str, err);
}

static bool spud_map_add_data(spud_map_t *map, const char *key,
                              const uint8_t *data, size_t data_sz, ls_err *err)
{
    assert(map != NULL);

    if (key && data) {
        cn_cbor *val = cn_cbor_data_create(data, data_sz, &map->ctx, NULL);
        if (!val ||
            !cn_cbor_mapput_string(map->cbor, key, val, &map->ctx, NULL)) {
            LS_ERROR(err, LS_ERR_NO_MEMORY);
            return false;
        }
    }

    return true;
}

static const uint8_t *spud_map_get_data(spud_map_t *map, const char *key,
                                        size_t *pdata_sz)
{
    const cn_cbor *val = cn_cbor_mapget_string(map->cbor, key);

    if (val->type != CN_CBOR_BYTES) {
        return NULL;
    }

    *pdata_sz = val->length;

    return (const uint8_t *)val->v.str;
}

// assumes the callers has made sure that there's no warnings' map
static cn_cbor *spud_map_create_warnings(spud_map_t *map)
{
    assert(map != NULL);
    assert(map->cbor != NULL);

    cn_cbor *warning_map = cn_cbor_map_create(&map->ctx, NULL);

    if (warning_map == NULL ||
        !cn_cbor_mapput_string(map->cbor, "warning", warning_map, &map->ctx,
                               NULL)) {
        return NULL;
    }

    return warning_map;
}

static const char *cbor_map_get_string(cn_cbor *map, const char *key)
{
    const cn_cbor *val = cn_cbor_mapget_string(map, key);

    if (val->type != CN_CBOR_TEXT) {
        return NULL;
    }

    return val->v.str;
}
