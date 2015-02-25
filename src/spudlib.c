#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include "spudlib.h"
#include "../config.h"

bool spud_isSpud(const uint8_t *payload, uint16_t length)
{
    if (length < sizeof(struct SpudMsgHdr)) {
        return false;
    }
    return (memcmp(payload, (void *)SpudMagicCookie, SPUD_MAGIC_COOKIE_SIZE) == 0);
}

bool spud_init(struct SpudMsg *msg, struct SpudMsgFlagsId *id)
{
    memcpy(msg->msgHdr.magic, SpudMagicCookie, SPUD_MAGIC_COOKIE_SIZE);
    SPUD_SET_FLAGS(msg->msgHdr.flags_id, 0);
    if (id != NULL) {
        return spud_setId(msg, id);
    } else {
        return spud_createId(&msg->msgHdr.flags_id);
    }
}

static bool get_randBuf(void *buf, size_t sz)
{
#ifdef HAVE_ARC4RANDOM
    arc4random_buf(buf, sz);
    return true;
#elif defined(HAVE__DEV_URANDOM)
    // TODO: think about pre-reading entropy to avoid blocking I/O here.
    // *certainly* don't open/close the file every time.
    // Also, read these:
    // http://insanecoding.blogspot.com/2014/05/a-good-idea-with-bad-usage-devurandom.html
    // http://www.2uo.de/myths-about-urandom/
    // --- Yes. Move to openSSL or someting?
    FILE *rfile = fopen("/dev/urandom", "r");
    size_t nread;
    if (!rfile) {
        perror("Error opening /dev/urandom");
        //Do something else?
        return false;
    }

    nread = fread(id, sz, 1, rfile);
    fclose(rfile);
    return nread == sz;
#else
  #error New random source needed
#endif
}

bool spud_createId(struct SpudMsgFlagsId *id)
{
    uint8_t flags;
    if (id == NULL) {
        return false;
    }
    flags = SPUD_GET_FLAGS(*id);

    if (!get_randBuf(id, sizeof(*id))) {
        return false;
    }
    SPUD_SET_FLAGS(*id, flags);
    return true;
}


bool spud_setId(struct SpudMsg *msg,const struct SpudMsgFlagsId *id)
{
    uint8_t flags;
    if(msg == NULL || id == NULL){
        return false;
    }
    flags = SPUD_GET_FLAGS(msg->msgHdr.flags_id);
    memcpy(&msg->msgHdr.flags_id, id, SPUD_FLAGS_ID_SIZE);
    SPUD_SET_FLAGS(msg->msgHdr.flags_id, flags);
    return true;
}

bool spud_isIdEqual(const struct SpudMsgFlagsId *a,const struct SpudMsgFlagsId *b)
{
    return ((a->octet[0] & SPUD_FLAGS_EXCLUDE_MASK) ==
            (b->octet[0] & SPUD_FLAGS_EXCLUDE_MASK)) &&
           (memcmp(&a->octet[1], &b->octet[1], SPUD_FLAGS_ID_SIZE-1) == 0);
}

char* spud_idToString(char* buf, size_t len, const struct SpudMsgFlagsId *id)
{
    size_t i;

    if(len < SPUD_ID_STRING_SIZE+1){
        return NULL;
    }

    for(i=0;i<SPUD_FLAGS_ID_SIZE;i++){
        sprintf(buf+2*i,"%02x",
        (i==0) ? (id->octet[i] & SPUD_FLAGS_EXCLUDE_MASK) : id->octet[i]);
    }
    return buf;
}
