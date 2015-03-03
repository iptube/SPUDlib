#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <errno.h>

#include "spudlib.h"
#include "../config.h"

bool spud_isSpud(const uint8_t *payload, size_t length)
{
    if (length < sizeof(spud_header_t)) {
        return false;
    }
    return (memcmp(payload, (void *)SpudMagicCookie, SPUD_MAGIC_COOKIE_SIZE) == 0);
}

bool spud_init(spud_header_t *hdr, spud_tube_id_t *id, ls_err *err)
{
    memcpy(hdr->magic, SpudMagicCookie, SPUD_MAGIC_COOKIE_SIZE);
    hdr->flags = 0;
    if (id != NULL) {
        return spud_setId(hdr, id, err);
    } else {
        return spud_createId(&hdr->tube_id, err);
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

    nread = fread(buf, 1, sz, rfile);

    fclose(rfile);
    // Only true if size is 1. (did not understand man page)
    // If this is untrue, something horrible has happened, and we should just
    // stop.
    return (nread == sz );
#else
  #error New random source needed
#endif
}

bool spud_createId(spud_tube_id_t *id, ls_err *err)
{
    if (id == NULL) {
        LS_ERROR(err, LS_ERR_INVALID_ARG);
        return false;
    }

    if (!get_randBuf(id, sizeof(*id), err)) {
        return false;
    }
    return true;
}

bool spud_parse(const uint8_t *payload, size_t length, spud_message_t *msg, ls_err *err)
{
    cn_cbor_errback cbor_err;
    if ((payload == NULL) || (msg == NULL) || !spud_isSpud(payload, length)) {
        LS_ERROR(err, LS_ERR_INVALID_ARG);
        return false;
    }
    msg->header = (spud_header_t *)payload;
    if (length > sizeof(spud_header_t)) {
        msg->cbor = cn_cbor_decode((const char*)payload+sizeof(spud_header_t),
                                   length - sizeof(spud_header_t),
                                   &cbor_err);
        if (!msg->cbor) {
            if (err != NULL) {
                err->code = LS_ERR_USER + cbor_err.err;
                err->message = cn_cbor_error_str[cbor_err.err];
                err->function = __func__;
                err->file = __FILE__;
                err->line = __LINE__;
            }
            return false;
        }
    } else {
        msg->cbor = NULL;
    }
    return true;
}

bool spud_setId(spud_header_t *hdr, const spud_tube_id_t *id, ls_err *err)
{
    if (hdr == NULL || id == NULL) {
        LS_ERROR(err, LS_ERR_INVALID_ARG);
        return false;
    }
    memcpy(&hdr->tube_id, id, SPUD_TUBE_ID_SIZE);
    return true;
}

bool spud_isIdEqual(const spud_tube_id_t *a, const spud_tube_id_t *b)
{
    if (a == NULL || b == NULL) {
        return false;
    }

    return memcmp(a, b, SPUD_TUBE_ID_SIZE) == 0;
}

char* spud_idToString(char* buf, size_t len, const spud_tube_id_t *id, ls_err *err)
{
    size_t i;

    if(len < SPUD_ID_STRING_SIZE+1){
        LS_ERROR(err, LS_ERR_INVALID_ARG);
        return NULL;
    }

    for(i=0;i<SPUD_TUBE_ID_SIZE;i++){
        sprintf(buf+2*i,"%02x", id->octet[i]);
    }
    buf[i] = 0;
    return buf;
}

bool spud_copyId(const spud_tube_id_t *src, spud_tube_id_t *dest, ls_err *err) {
    if (src == NULL || dest == NULL) {
        LS_ERROR(err, LS_ERR_INVALID_ARG);
        return false;
    }

    memcpy(dest, src, sizeof(spud_tube_id_t));
    return true;
}
