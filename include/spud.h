/*
 * Copyright (c) 2015 SPUDlib authors.  See LICENSE file.
 */

#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "ls_error.h"
#include "cn-cbor/cn-cbor.h"

#define SPUD_TUBE_ID_SIZE               8
#define SPUD_ID_STRING_SIZE             (2*SPUD_TUBE_ID_SIZE)

#define SPUD_MAGIC_COOKIE_ARRAY         {0xd8, 0x00, 0x00, 0xd8}
static const uint8_t SpudMagicCookie[]   = SPUD_MAGIC_COOKIE_ARRAY;
#define SPUD_MAGIC_COOKIE_SIZE sizeof(SpudMagicCookie)/sizeof(SpudMagicCookie[0])

/* OR these with the top byte of flags_id */
typedef enum {
    SPUD_DATA  = 0x00 << 6,
    SPUD_OPEN  = 0x01 << 6,
    SPUD_CLOSE = 0x02 << 6,
    SPUD_ACK   = 0x03 << 6
} spud_command;

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


typedef struct _spud_tube_id {
    /* 64 bits */
    uint8_t octet[SPUD_TUBE_ID_SIZE];
} spud_tube_id;

/*
 * SPUD message header.
 */
typedef struct _spud_header
{
    uint8_t magic[SPUD_MAGIC_COOKIE_SIZE];
    spud_tube_id tube_id;
    uint8_t flags;
} spud_header;


/* Decoded  SPUD message */
typedef struct _spud_message
{
    spud_header *header;
    /* CBOR MAP */
    const cn_cbor *cbor;
} spud_message;


bool spud_is_spud(const uint8_t *payload, size_t length);

bool spud_parse(const uint8_t *payload, size_t length, spud_message *msg, ls_err *err);
void spud_unparse(spud_message *msg);

bool spud_init(spud_header *hdr, spud_tube_id *id, ls_err *err);

bool spud_create_id(spud_tube_id *id, ls_err *err);

bool spud_is_id_equal(const spud_tube_id *a,
                      const spud_tube_id *b);

bool spud_set_id(spud_header *hdr, const spud_tube_id *id, ls_err *err);

char* spud_id_to_string(char* buf, size_t len, const spud_tube_id *id, ls_err *err);

bool spud_copy_id(const spud_tube_id *src, spud_tube_id *dest, ls_err *err);
