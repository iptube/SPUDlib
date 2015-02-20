#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include "spudlib.h"
#include "../config.h"

bool spud_isSpud(const uint8_t *payload, uint16_t length)
{
    return ( memcmp(payload, (void *)SpudMagicCookie, SPUD_MAGIC_COOKIE_SIZE) == 0 );
}

bool spud_createId(struct SpudMsgId *id)
{
#ifdef HAVE_ARC4RANDOM
    arc4random_buf(id, sizeof(*id));
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

    nread = fread(id, sizeof(*id), 1, rfile);
    fclose(rfile);
    return nread == sizeof(*id);
#else
  #error New random source needed
#endif
}


bool spud_setId(struct SpudMsg *msg,const struct SpudMsgId *id)
{
    if(msg == NULL || id == NULL){
        return false;
    }
    memcpy(&msg->msgHdr.id, id, SPUD_MSG_ID_SIZE);

    return true;
}

bool spud_isIdEqual(const struct SpudMsgId *a,const struct SpudMsgId *b)
{
    return (memcmp(a, b, SPUD_MSG_ID_SIZE) == 0);
}

char* spud_idToString(char* buf, size_t len, const struct SpudMsgId *id)
{
    size_t i;

    if(len < SPUD_MSG_ID_SIZE*2+1){
        return NULL;
    }

    for(i=0;i<SPUD_MSG_ID_SIZE;i++){
        sprintf(buf+2*i,"%02x",id->octet[i]);
    }
    return buf;
}
