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
  (flags_id).octet[0] = ((flags_id).octet[0] & SPUD_FLAGS_EXCLUDE_MASK) & (flags)

#define SPUD_MAGIC_COOKIE_ARRAY         {0xd8, 0x00, 0x00, 0xd8}
static const uint8_t SpudMagicCookie[]   = SPUD_MAGIC_COOKIE_ARRAY;
#define SPUD_MAGIC_COOKIE_SIZE sizeof(SpudMagicCookie)/sizeof(SpudMagicCookie[0])

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


struct SpudMsgFlagsId {
    //56 bits
    uint8_t octet[SPUD_FLAGS_ID_SIZE];
};

/*
 * SPUD message header.
 */
struct SpudMsgHdr
{
    uint8_t magic[SPUD_MAGIC_COOKIE_SIZE];
    struct SpudMsgFlagsId flags_id;
};


/* Decoded  SPUD message */
struct SpudMsg
{
    struct SpudMsgHdr msgHdr;
    //CBOR MAP
};


bool spud_isSpud(const uint8_t *payload, uint16_t length);

bool spud_init(struct SpudMsg *msg, struct SpudMsgFlagsId *id);

bool spud_createId(struct SpudMsgFlagsId *id);

bool spud_isIdEqual(const struct SpudMsgFlagsId *a,const struct SpudMsgFlagsId *b);

bool spud_setId(struct SpudMsg *msg,const struct SpudMsgFlagsId *id);

char* spud_idToString(char* buf, size_t len, const struct SpudMsgFlagsId *id);
#endif
