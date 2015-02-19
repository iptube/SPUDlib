#include <string.h>
#include <stdio.h>
#include <stdint.h>

#include "spudlib.h"

bool spud_isSpud(const uint8_t *payload, uint16_t length)
{
    return ( memcmp(payload, (void *)SpudMagicCookie, SPUD_MAGIC_COOKIE_SIZE) == 0 );
}

bool spud_createId(struct SpudMsgId *id)
{
    FILE *rfile = fopen("/dev/random", "r");
    
    if (!rfile) {
        perror("Error opening /dev/random");
        //Do something else?
        return false;
    }
    size_t i;
    for (i=0; i<sizeof(*id); i++) {
        id->octet[i] |= fgetc(rfile);
    }
    
    fclose(rfile);
    return true;
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
    if(len < SPUD_MSG_ID_SIZE){
        return NULL;
    }
    size_t i;
    for(i=0;i<SPUD_MSG_ID_SIZE;i++){
        sprintf(buf+i,"%02x",id->octet[i]);
    }
    return buf;
}
