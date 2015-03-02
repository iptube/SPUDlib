#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "ls_error.h"
#include "cn-cbor.h"

#define SPUD_TUBE_ID_SIZE               8
#define SPUD_ID_STRING_SIZE             (2*SPUD_TUBE_ID_SIZE)
#define SPUD_FLAGS_SELECT_MASK          0xF0
#define SPUD_FLAGS_EXCLUDE_MASK         0x0F
#define SPUD_GET_FLAGS(flags_id) \
  (flags_id).octet[0] & SPUD_FLAGS_SELECT_MASK
#define SPUD_SET_FLAGS(flags_id, flags) \
  (flags_id).octet[0] = ((flags_id).octet[0] & SPUD_FLAGS_EXCLUDE_MASK) | (flags)

#define SPUD_MAGIC_COOKIE_ARRAY         {0xd8, 0x00, 0x00, 0xd8}
static const uint8_t SpudMagicCookie[]   = SPUD_MAGIC_COOKIE_ARRAY;
#define SPUD_MAGIC_COOKIE_SIZE sizeof(SpudMagicCookie)/sizeof(SpudMagicCookie[0])

// OR these with the top byte of flags_id
typedef enum {
    SPUD_DATA  = 0x00 << 6,
    SPUD_OPEN  = 0x01 << 6,
    SPUD_CLOSE = 0x02 << 6,
    SPUD_ACK   = 0x03 << 6
} spud_command_t;

#define SPUD_ADEC    0x20
#define SPUD_PDEC    0x10
#define SPUD_COMMAND 0xC0

/*
 *  0                   1                   2                   3
 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                magic cookie = 0xd80000d8                      |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                          tube ID                              |
 *  +                          (60 bit)                             +
 *  |                                                               |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |cmd|a|p|  resv |           CBOR Map                            |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *
 *  From:
 *  https://tools.ietf.org/html/draft-hildebrand-spud-prototype-02
 */


typedef struct _spud_tube_id_t {
    // 64 bits
    uint8_t octet[SPUD_TUBE_ID_SIZE];
} spud_tube_id_t;

/*
 * SPUD message header.
 */
typedef struct _spud_header_t
{
    uint8_t magic[SPUD_MAGIC_COOKIE_SIZE];
    spud_tube_id_t tube_id;
    uint8_t flags;
} spud_header_t;


/* Decoded  SPUD message */
typedef struct _spud_message_t
{
    spud_header_t *header;
    //CBOR MAP
    const cn_cbor *cbor;
} spud_message_t;


bool spud_isSpud(const uint8_t *payload, size_t length);

bool spud_parse(const uint8_t *payload, size_t length, spud_message_t *msg, ls_err *err);

bool spud_init(spud_header_t *hdr, spud_tube_id_t *id, ls_err *err);

bool spud_createId(spud_tube_id_t *id, ls_err *err);

bool spud_isIdEqual(const spud_tube_id_t *a,
                    const spud_tube_id_t *b);

bool spud_setId(spud_header_t *hdr, const spud_tube_id_t *id, ls_err *err);

char* spud_idToString(char* buf, size_t len, const spud_tube_id_t *id, ls_err *err);

bool spud_copyId(const spud_tube_id_t *src, spud_tube_id_t *dest, ls_err *err);
