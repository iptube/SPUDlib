/*
 * Copyright (c) 2015 SPUDlib authors.  See LICENSE file.
 */

#ifndef _SPUD_MAP_H
#define _SPUD_MAP_H

#include "spud.h"

typedef struct spud_map_s {
    cn_cbor_context ctx;
    cn_cbor *cbor;
} spud_map_t;

#define SPUD_MAP_MAX_SERIALIZED_SZ 1500

//
// SPUD CBOR map life time
//
LS_API void spud_map_reset(spud_map_t *map);
LS_API bool spud_map_create(spud_map_t **pmap, ls_err *err);
LS_API bool spud_map_create_ctx(spud_map_t *map, ls_err *err);

LS_API void spud_map_free(spud_map_t *map);
LS_API void spud_map_free_ctx(spud_map_t *map);

//
// Setters for various path declarations
// (Section 8 of draft-hildebrand-spud-prototype-03)
//
LS_API bool spud_map_add_ip_address(spud_map_t *map, const uint8_t *ip_addr,
                                    size_t ip_addr_sz, ls_err *err);
LS_API bool spud_map_add_token(spud_map_t *map, const uint8_t *token,
                               size_t token_sz, ls_err *err);
LS_API bool spud_map_add_url(spud_map_t *map, const char *url, ls_err *err);
LS_API bool spud_map_add_warning(spud_map_t *map, const char *msg_tag,
                                 const char *msg, ls_err *err);
LS_API bool spud_map_add_icmp(spud_map_t *map, const uint8_t *icmp_pkt,
                              size_t icmp_pkt_sz, ls_err *err);
LS_API bool spud_map_add_icmp_type(spud_map_t *map, int64_t icmp_type,
                                   ls_err *err);
LS_API bool spud_map_add_icmp_code(spud_map_t *map, int64_t icmp_code,
                                   ls_err *err);
LS_API bool spud_map_add_translated_external_address(spud_map_t *map,
                                                     const uint8_t *ip_addr,
                                                     size_t ip_addr_sz,
                                                     ls_err *err);
LS_API bool spud_map_add_internal_address(spud_map_t *map,
                                          const uint8_t *ip_addr,
                                          size_t ip_addr_sz, ls_err *err);
LS_API bool spud_map_add_translated_external_port(spud_map_t *map,
                                                  uint16_t port, ls_err *err);
LS_API bool spud_map_add_internal_port(spud_map_t *map, uint16_t port,
                                       ls_err *err);
LS_API bool spud_map_add_inactivity_timer(spud_map_t *map, int64_t msec,
                                          ls_err *err);
LS_API bool spud_map_add_path_element_description(spud_map_t *map,
                                                  const char *description,
                                                  ls_err *err);
LS_API bool spud_map_add_path_element_version(spud_map_t *map,
                                              const char *version, ls_err *err);
LS_API bool spud_map_add_path_element_caps(spud_map_t *map, const uint8_t *caps,
                                           size_t caps_sz, ls_err *err);
LS_API bool spud_map_add_path_element_ttl(spud_map_t *map, int64_t ttl,
                                          ls_err *err);
LS_API bool spud_map_add_mtu(spud_map_t *map, int64_t mtu, ls_err *err);
LS_API bool spud_map_add_max_byte_rate(spud_map_t *map, int64_t mbr,
                                       ls_err *err);
LS_API bool spud_map_add_max_packet_rate(spud_map_t *map, int64_t mpr,
                                         ls_err *err);
LS_API bool spud_map_add_latency(spud_map_t *map, int64_t msec, ls_err *err);

//
// Getters (a very limited subset of what can be set)
//
LS_API const char *spud_map_get_url(spud_map_t *map, size_t *purl_len);
LS_API const uint8_t *spud_map_get_ip_address(spud_map_t *map,
                                              size_t *pip_addr_sz);
LS_API const uint8_t *spud_map_get_token(spud_map_t *map, size_t *ptoken_sz);
LS_API const char *spud_map_get_warning(spud_map_t *map, const char *lang_tag,
                                        size_t *pmsg_len);

//
// Encode/decode
//
LS_API bool spud_map_encode(const spud_map_t *map, uint8_t *out, size_t *out_sz,
                            ls_err *err);
LS_API bool spud_map_decode(const uint8_t *buf, size_t buf_sz, spud_map_t *map,
                            ls_err *err);

#endif  // !_SPUD_MAP_H
