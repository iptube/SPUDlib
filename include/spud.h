/**
 * \file
 * \brief
 * SPUD protocol constants and aux functions
 *
 * Copyright (c) 2015 SPUDlib authors.  See LICENSE file.

 * SPUD Header Format
 *  From:
 *  https://tools.ietf.org/html/draft-hildebrand-spud-prototype-02
 *
\verbatim

    0                   1                   2                   3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |                magic cookie = 0xd80000d8                      |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |                          tube ID                              |
    +                          (64 bit)                             +
    |                                                               |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |cmd|a|p|  resv |           CBOR Map                            |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
\endverbatim 
 */

#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "ls_error.h"
#include "cn-cbor/cn-cbor.h"

/** Size of the tube ID in bytes */
#define SPUD_TUBE_ID_SIZE               8

/** Size of hex representation of tube ID (excludes terminator) */
#define SPUD_ID_STRING_SIZE             (2*SPUD_TUBE_ID_SIZE)

/** SPUD magic number.  Chosen to be invalid in most other UDP-borne protocols. */
#define SPUD_MAGIC_COOKIE_ARRAY         {0xd8, 0x00, 0x00, 0xd8}
static const uint8_t SpudMagicCookie[]   = SPUD_MAGIC_COOKIE_ARRAY;

/** Size of the magic number */
#define SPUD_MAGIC_COOKIE_SIZE sizeof(SpudMagicCookie)/sizeof(SpudMagicCookie[0])

/**
 * Codes for tube establishment/teardown.
 * OR with top byte of flags_id.
 */
typedef enum {
    /** Tube already open */
    SPUD_DATA  = 0x00 << 6,
    /**  New tube */
    SPUD_OPEN  = 0x01 << 6,
    /** Delete this tube */
    SPUD_CLOSE = 0x02 << 6,
    /** Responder confirms open */
    SPUD_ACK   = 0x03 << 6
} spud_command;

/** Application-to-path declaration bit */
#define SPUD_ADEC    0x20
/** Path-to-application declaration bit */
#define SPUD_PDEC    0x10
/** OPEN/CLOSE/DATA/ACK codes */
#define SPUD_COMMAND 0xC0

/** Identifier for this tube.
 *  (Note: Scope TBD)
 */
typedef struct _spud_tube_id {
    /* 64 bits */
    uint8_t octet[SPUD_TUBE_ID_SIZE];
} spud_tube_id;

/**
 * SPUD fixed message header.  Everything following this is CBOR, including payload.
 */
typedef struct _spud_header
{
  /** magic number, should be 0xd8 00 00 d8 */
    uint8_t magic[SPUD_MAGIC_COOKIE_SIZE];
  /** tube identifier, scope TBD */
    spud_tube_id tube_id;
  /** command and direction flags */
    uint8_t flags;
} spud_header;


/** Decoded spud message (UDP payload)
 */
typedef struct _spud_message
{
  /** fixed header */
    spud_header *header;
  /** CBOR map */
    const cn_cbor *cbor;
} spud_message;

/**
 * Is this a spud packet?  Checks for valid length and magic number.
 * 
 * \param[in] payload The packet to be checked.
 * \param[in] length Length of the payload.
 * \return true if length > fixed header and magic # is correct.
 */
bool spud_is_spud(const uint8_t *payload, size_t length);

/** 
 * Decode a packet into header and parsed CBOR structure.
 *
 * \param[in] payload The received packet
 * \param[in] length  Bytes in payload
 * \param[out] msg  Parsed message is placed here
 * \param[in,out] err  If non-NULL on input, indicates error when returning false
 * \return true on success.
 */
bool spud_parse(const uint8_t *payload, size_t length, spud_message *msg, ls_err *err);

/**
 * Deallocate memory allocated in parsing.
 *
 * \param[in] msg Pointer to a spud_msg previously parsed by spud_parse()
 */
void spud_unparse(spud_message *msg);

/**
 * Initialize the fixed part of a SPUD header.
 *
 * \param[in] hdr The header to be init'd
 * \param[in] id  Tube ID to use. If NULL, a fresh (random) ID will be created 
 * \param[in,out] err If non-NULL on input, indicates error when returning false
 * \return true on success, else false and err describes the problem.
 */
bool spud_init(spud_header *hdr, spud_tube_id *id, ls_err *err);

/**
 * Create a new tube ID.
 *
 * \param[in] id  Where to put it
 * \param[in,out] err If non-NULL on input, indicates error when returning false
 * \return true on success.
 */
bool spud_create_id(spud_tube_id *id, ls_err *err);

/**
 * Compare tube IDs.
 *
 * \param[in] a  A tube ID
 * \param[in] b  A tube ID
 * \return  true if ID's are the same
 */
bool spud_is_id_equal(const spud_tube_id *a,
                      const spud_tube_id *b);

/**
 * Set tube ID field in header.
 * \param[in] hdr Header to be modified
 * \param[in] id  The Tube ID to use
 * \param[in,out] err If non-NULL on input, indicates error when returning false
 * \return  true on success
 */
bool spud_set_id(spud_header *hdr, const spud_tube_id *id, ls_err *err);

/**
 * Convert id to hex string.
 * \param[in] buf Space for the string
 * \param[in] len Size of buf, must be at least SPUD_ID_STRING_SIZE+1
 * \param[in] id The tube ID to convert
 * \return buf (input value) on success, else NULL
 */
char* spud_id_to_string(char* buf, size_t len, const spud_tube_id *id);

/**
 * Duplicate a tube ID.
 * \param[in] src The tube ID to copy, must be nonnull
 * \param[in] dest Where to put it, must be nonnull
 */
void spud_copy_id(const spud_tube_id *src, spud_tube_id *dest);
