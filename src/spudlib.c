#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include "spudlib.h"
#include "../config.h"

bool spud_isSpud(const uint8_t *payload, size_t length)
{
    if (length < sizeof(spud_header_t)) {
        return false;
    }
    return (memcmp(payload, (void *)SpudMagicCookie, SPUD_MAGIC_COOKIE_SIZE) == 0);
}

bool spud_init(spud_header_t *hdr, spud_flags_id_t *id, ls_err *err)
{
    memcpy(hdr->magic, SpudMagicCookie, SPUD_MAGIC_COOKIE_SIZE);
    SPUD_SET_FLAGS(hdr->flags_id, 0);
    if (id != NULL) {
        return spud_setId(hdr, id, err);
    } else {
        return spud_createId(&hdr->flags_id, err);
    }
}

static bool get_randBuf(void *buf, size_t sz, ls_err *err)
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
        LS_ERROR(err, -errno);
        return false;
    }

    nread = fread(buf, sz, 1, rfile);
    fclose(rfile);
    return nread == sz;
#else
  #error New random source needed
#endif
}

bool spud_createId(spud_flags_id_t *id, ls_err *err)
{
    uint8_t flags;
    if (id == NULL) {
        LS_ERROR(err, LS_ERR_INVALID_ARG);
        return false;
    }
    flags = SPUD_GET_FLAGS(*id);

    if (!get_randBuf(id, sizeof(*id), err)) {
        return false;
    }
    SPUD_SET_FLAGS(*id, flags);
    return true;
}

bool spud_cast(const uint8_t *payload, size_t length, spud_message_t *msg, ls_err *err)
{
    if ((payload == NULL) || (msg == NULL) || !spud_isSpud(payload, length)) {
        LS_ERROR(err, LS_ERR_INVALID_ARG);
        return false;
    }
    msg->header = (spud_header_t *)payload;
    msg->length = length - sizeof(spud_header_t);
    msg->data = (msg->length>0) ? (payload+sizeof(spud_header_t)) : NULL;
    return true;
}

bool spud_setId(spud_header_t *hdr, const spud_flags_id_t *id, ls_err *err)
{
    uint8_t flags;
    if (hdr == NULL || id == NULL) {
        LS_ERROR(err, LS_ERR_INVALID_ARG);
        return false;
    }
    flags = SPUD_GET_FLAGS(hdr->flags_id);
    memcpy(&hdr->flags_id, id, SPUD_FLAGS_ID_SIZE);
    SPUD_SET_FLAGS(hdr->flags_id, flags);
    return true;
}

bool spud_isIdEqual(const spud_flags_id_t *a,const spud_flags_id_t *b)
{
    return ((a->octet[0] & SPUD_FLAGS_EXCLUDE_MASK) ==
            (b->octet[0] & SPUD_FLAGS_EXCLUDE_MASK)) &&
           (memcmp(&a->octet[1], &b->octet[1], SPUD_FLAGS_ID_SIZE-1) == 0);
}

char* spud_idToString(char* buf, size_t len, const spud_flags_id_t *id)
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

bool spud_copyId(const spud_flags_id_t *src, spud_flags_id_t *dest, ls_err *err) {
    if (src == NULL || dest == NULL) {
        LS_ERROR(err, LS_ERR_INVALID_ARG);
        return false;
    }

    memcpy(dest, src, sizeof(spud_flags_id_t));
    dest->octet[0] &= SPUD_FLAGS_EXCLUDE_MASK;
    return true;
}
