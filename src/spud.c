/*
 * Copyright (c) 2015 SPUDlib authors.  See LICENSE file.
 */

#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <errno.h>
#include <assert.h>

#include "spud.h"
#include "config.h"

bool spud_is_spud(const uint8_t *payload, size_t length)
{
    if (length < sizeof(spud_header)) {
        return false;
    }
    return (memcmp(payload, (void *)SpudMagicCookie, SPUD_MAGIC_COOKIE_SIZE) == 0);
}

bool spud_init(spud_header *hdr, spud_tube_id *id, ls_err *err)
{
    memcpy(hdr->magic, SpudMagicCookie, SPUD_MAGIC_COOKIE_SIZE);
    hdr->flags = 0;
    if (id != NULL) {
        return spud_set_id(hdr, id, err);
    } else {
        return spud_create_id(&hdr->tube_id, err);
    }
}

static bool get_rand_buf(void *buf, size_t sz, ls_err *err)
{
#ifdef HAVE_ARC4RANDOM
    UNUSED_PARAM(err);
    arc4random_buf(buf, sz);
    return true;
#elif defined(HAVE__DEV_URANDOM)
    /* TODO: think about pre-reading entropy to avoid blocking I/O here. */
    /* *certainly* don't open/close the file every time. */
    /* Also, read these: */
    /* http://insanecoding.blogspot.com/2014/05/a-good-idea-with-bad-usage-devurandom.html */
    /* http://www.2uo.de/myths-about-urandom/ */
    /* --- Yes. Move to openSSL or someting? */
    FILE *rfile = fopen("/dev/urandom", "r");
    size_t nread;
    if (!rfile) {
        LS_ERROR(err, -errno);
        return false;
    }
    nread = fread(buf, 1, sz, rfile);
    fclose(rfile);
    /* Only true if size is 1. (did not understand man page) */
    /* If this is untrue, something horrible has happened, and we should just */
    /* stop. */
    return (nread == sz );
#else
  #error New random source needed
#endif
}

bool spud_create_id(spud_tube_id *id, ls_err *err)
{
    if (id == NULL) {
        LS_ERROR(err, LS_ERR_INVALID_ARG);
        return false;
    }

    if (!get_rand_buf(id, sizeof(*id), err)) {
        return false;
    }
    return true;
}

bool spud_parse(const uint8_t *payload, size_t length, spud_message *msg, ls_err *err)
{
    cn_cbor_errback cbor_err;
    if ((payload == NULL) || (msg == NULL) || !spud_is_spud(payload, length)) {
        LS_ERROR(err, LS_ERR_INVALID_ARG);
        return false;
    }
    msg->header = (spud_header *)payload;
    if (length > sizeof(spud_header)) {
        msg->cbor = cn_cbor_decode((const char*)payload+sizeof(spud_header),
                                   length - sizeof(spud_header),
                                   NULL, NULL,
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

void spud_unparse(spud_message *msg)
{
    msg->header = NULL;
    if (msg->cbor) {
        cn_cbor_free(msg->cbor);
    }
    msg->cbor = NULL;
}

bool spud_set_id(spud_header *hdr, const spud_tube_id *id, ls_err *err)
{
    if (hdr == NULL || id == NULL) {
        LS_ERROR(err, LS_ERR_INVALID_ARG);
        return false;
    }
    memcpy(&hdr->tube_id, id, SPUD_TUBE_ID_SIZE);
    return true;
}

bool spud_is_id_equal(const spud_tube_id *a, const spud_tube_id *b)
{
    if (a == NULL || b == NULL) {
        return false;
    }

    return memcmp(a, b, SPUD_TUBE_ID_SIZE) == 0;
}

char* spud_id_to_string(char* buf, size_t len, const spud_tube_id *id)
{
    // almost an assert
    if (len < SPUD_ID_STRING_SIZE+1){
        return NULL;
    }

    snprintf(buf, len, "%02x%02x%02x%02x%02x%02x%02x%02x",
             id->octet[0],
             id->octet[1],
             id->octet[2],
             id->octet[3],
             id->octet[4],
             id->octet[5],
             id->octet[6],
             id->octet[7]);
    return buf;
}

void spud_copy_id(const spud_tube_id *src, spud_tube_id *dest) {
    assert(src);
    assert(dest);

    memcpy(dest, src, sizeof(spud_tube_id));
}
