#ifndef SPUDLIB_H
#define SPUDLIB_H


#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define SPUD_FLAGS_ID_SIZE              8
#define SPUD_ID_STRING_SIZE             (2*SPUD_FLAGS_ID_SIZE)
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
 *  |cmd|a|p|                  tube ID                              |
 *  +-+-+-+-+                  (60 bit)                             +
 *  |                                                               |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                           CBOR Map                            |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *
 *  From:
 *  https://tools.ietf.org/html/draft-hildebrand-spud-prototype-01
 */


typedef struct _spud_flags_id_t {
    //56 bits
    uint8_t octet[SPUD_FLAGS_ID_SIZE];
} spud_flags_id_t;

/*
 * SPUD message header.
 */
typedef struct _spud_header_t
{
    uint8_t magic[SPUD_MAGIC_COOKIE_SIZE];
    spud_flags_id_t flags_id;
} spud_header_t;


/* Decoded  SPUD message */
typedef struct _spud_message_t
{
    spud_header_t *header;
    //CBOR MAP
    size_t length;
    const uint8_t *data;
} spud_message_t;


bool spud_isSpud(const uint8_t *payload, size_t length);

bool spud_cast(const uint8_t *payload, size_t length, spud_message_t *msg);

bool spud_init(spud_header_t *hdr, spud_flags_id_t *id);

bool spud_createId(spud_flags_id_t *id);

bool spud_isIdEqual(const spud_flags_id_t *a,
                    const spud_flags_id_t *b);

bool spud_setId(spud_header_t *hdr, const spud_flags_id_t *id);

char* spud_idToString(char* buf, size_t len, const spud_flags_id_t *id);
#endif
