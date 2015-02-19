#ifndef SPUDLIB_H
#define SPUDLIB_H


#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define SPUD_MSG_ID_SIZE                7

#define SPUD_MAGIC_COOKIE_ARRAY         {0xd8, 0x00, 0x00, 0xd8}
static const uint8_t SpudMagicCookie[]   = SPUD_MAGIC_COOKIE_ARRAY;
#define SPUD_MAGIC_COOKIE_SIZE sizeof(SpudMagicCookie)/sizeof(SpudMagicCookie[0])

/*
 *  0                   1                   2                   3
 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                magic cookie = 0xd80000d8                      |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |cmd|a|p| pri   |            tube ID                            |
 *  +-+-+-+-+-+-+-+-+          (56 bit)                             +
 *  |                                                               |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                           CBOR Map                            |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *
 *  ** Note this differs from:
 *  https://tools.ietf.org/html/draft-hildebrand-spud-prototype-01
 */


struct SpudMagicCookie {
    uint8_t cookie[SPUD_MAGIC_COOKIE_SIZE];
};


struct SpudMsgId {
    //56 bits
    uint8_t octet[SPUD_MSG_ID_SIZE];
};

/*
 * SPUD message header.
 */
struct SpudMsgHdr
{
    struct SpudMagicCookie magic;
    uint8_t       flags;
    struct SpudMsgId id;
};


/* Decoded  SPUD message */
struct SpudMsg
{
    struct SpudMsgHdr msgHdr;
    //CBOR MAP
};


bool spud_isSpud(const uint8_t *payload, uint16_t length);

bool spud_createId(struct SpudMsgId *id);

bool spud_isIdEqual(const struct SpudMsgId *a,const struct SpudMsgId *b);

bool spud_setId(struct SpudMsg *msg,const struct SpudMsgId *id);

char* spud_idToString(char* buf, size_t len, const struct SpudMsgId *id);
#endif
