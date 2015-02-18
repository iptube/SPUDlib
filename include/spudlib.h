#ifndef SPUDLIB_H
#define SPUDLIB_H


#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define SPUD_MAGIC_COOKIE_ARRAY         {0xd8, 0x00, 0x00, 0xd8}

#define SPUD_MSG_ID_SIZE                7

static const uint8_t SpudMagicCookie[]   = SPUD_MAGIC_COOKIE_ARRAY;
static const size_t  SpudMagicCookieSize = sizeof(SpudMagicCookie)/sizeof(SpudMagicCookie[0]);


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
    uint8_t cookie[SpudMagicCookieSize];
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

#endif
