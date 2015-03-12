#ifndef CN_ENCODER_C
#define CN_ENCODER_C

#ifdef  __cplusplus
extern "C" {
#endif
#ifdef EMACS_INDENTATION_HELPER
} /* Duh. */
#endif

#include <arpa/inet.h>
#include <string.h>
#include <strings.h>

#include "cn-cbor/cn-encoder.h"
#include "cbor.h"

#define hton8p(p) (*(uint8_t*)(p))
#define hton16p(p) (htons(*(uint16_t*)(p)))
#define hton32p(p) (htonl(*(uint32_t*)(p)))
static uint64_t hton64p(const uint8_t *p) {
  /* TODO: does this work on both BE and LE systems? */
  uint64_t ret = hton32p(p);
  ret <<= 32;
  ret |= hton32p(p+4);
  return ret;
}

#define ensure_writable(sz) if (buf_offset + count + (sz) >= buf_size) { \
  return -1; \
}

#define write_byte_and_data(b, data, sz) \
buf[buf_offset+count] = (b); \
count++; \
memcpy(buf+buf_offset+count, (data), (sz)); \
count += sz;

#define write_byte(b) \
ensure_writable(1); \
buf[buf_offset+count] = (b); \
count++;

static uint8_t _xlate[] = {
  IB_PRIM,     /* CN_CBOR_NULL */
  IB_PRIM,     /* CN_CBOR_FALSE */
  IB_PRIM,     /* CN_CBOR_TRUE */
  IB_UNSIGNED, /* CN_CBOR_UINT */
  IB_NEGATIVE, /* CN_CBOR_INT */
  IB_BYTES,    /* CN_CBOR_BYTES */
  IB_TEXT,     /* CN_CBOR_TEXT */
  IB_BYTES,    /* CN_CBOR_BYTES_CHUNKED */
  IB_TEXT,     /* CN_CBOR_TEXT_CHUNKED */
  IB_ARRAY,    /* CN_CBOR_ARRAY */
  IB_MAP,      /* CN_CBOR_MAP */
  IB_TAG,      /* CN_CBOR_TAG */
  IB_PRIM,     /* CN_CBOR_SIMPLE */
  0xFF,        /* CN_CBOR_DOUBLE */
  0xFF         /* CN_CBOR_INVALID */
};

ssize_t cbor_encoder_write_head(uint8_t *buf,
                                size_t buf_offset,
                                size_t buf_size,
                                cn_cbor_type typ,
                                uint64_t val) {
  ssize_t count = 0;
  uint8_t ib;

  if (typ >= sizeof(_xlate)) {
    return -1;
  }

  ib = _xlate[typ];
  if (ib == 0xFF) {
    return -1;
  }

  if (val < 24) {
    write_byte(ib + (uint8_t)val);
  } else if (val < 256) {
    ensure_writable(2);
    buf[buf_offset+count] = ib | 24;
    count++;
    buf[buf_offset+count] = (uint8_t)val;
    count++;
  } else if (val < 65536) {
    uint16_t be16 = (uint16_t)val;
    ensure_writable(3);
    be16 = hton16p(&be16);
    write_byte_and_data(ib | 25, (const void*)&be16, 2);
  } else if (val < 0x100000000L) {
    uint32_t be32 = (uint32_t)val;
    ensure_writable(5);
    be32 = hton32p(&be32);
    write_byte_and_data(ib | 26, (const void*)&be32, 4);
  } else {
    uint64_t be64;
    ensure_writable(9);
    be64 = hton64p((const uint8_t*)&val);
    write_byte_and_data(ib | 27, (const void*)&be64, 8);
  }
  return count;
}

ssize_t cbor_encoder_write_double(uint8_t *buf,
                                  size_t buf_offset,
                                  size_t buf_size,
                                  double val) {
  uint64_t be64;
  /* Copy the same problematic implementation from the decoder. */
  union {
    double d;
    uint64_t u;
  } u64;
  ssize_t count = 0;
  /* TODO: decide if we can write smaller float sizes */
  /* approach: mask off 0x0000000000000f7fUL (the exponent), shift it over, */
  /* re-byte-order it if needed, subtract 1023 (the double exponent bias), */
  /* then see if it's abs is less than 256 (single) or 32(half) */
  /* Do something similar with mantissa. */
  /* Also, check -0, +-inf, +-NaN, and sub-normals */

  /* Note: This currently makes the tests fail */
  ensure_writable(9);
  u64.d = val;
  be64 = hton64p((const uint8_t*)&u64.u);

  write_byte_and_data(IB_PRIM | 27, (const void*)&be64, 8);
  return count;
}

ssize_t cbor_encoder_write_negative(uint8_t *buf,
                                    size_t buf_offset,
                                    size_t buf_size,
                                    int64_t val) {
  return cbor_encoder_write_head(buf, buf_offset, buf_size, CN_CBOR_INT, -val - 1);
}


#define ADVANCE(st) ret = (st); \
if (ret < 0) { return -1; } \
count += ret;

static ssize_t _write_children(uint8_t *buf,
                               size_t buf_offset,
                               size_t buf_size,
                               const cn_cbor *cb){
   ssize_t count = 0;
   ssize_t ret = 0;
   cn_cbor *child;

   for (child=cb->first_child; child; child = child->next) {
     ADVANCE(cbor_encoder_write(buf, buf_offset+count, buf_size, child));
   }

   return count;
}

ssize_t cbor_encoder_write(uint8_t *buf,
                           size_t buf_offset,
                           size_t buf_size,
                           const cn_cbor *cb) {
  ssize_t count = 0;
  ssize_t ret = 0;

  switch (cb->type) {
  case CN_CBOR_ARRAY:
    if (cb->flags & CN_CBOR_FL_INDEF) {
      write_byte(_xlate[cb->type] | AI_INDEF);
      ADVANCE(_write_children(buf, buf_offset+count, buf_size, cb));
      write_byte(0xFF);
    } else {
      ADVANCE(cbor_encoder_write_head(buf, buf_offset, buf_size, cb->type, cb->length));
      ADVANCE(_write_children(buf, buf_offset+count, buf_size, cb));
    }
    break;
  case CN_CBOR_MAP:
    if (cb->flags & CN_CBOR_FL_INDEF) {
      write_byte(_xlate[cb->type] | AI_INDEF);
      ADVANCE(_write_children(buf, buf_offset+count, buf_size, cb));
      write_byte(0xFF);
    } else {
      ADVANCE(cbor_encoder_write_head(buf, buf_offset, buf_size, cb->type, cb->length/2));
      ADVANCE(_write_children(buf, buf_offset+count, buf_size, cb));
    }
    break;
  case CN_CBOR_BYTES_CHUNKED:
  case CN_CBOR_TEXT_CHUNKED:
    write_byte(_xlate[cb->type] | AI_INDEF);
    ADVANCE(_write_children(buf, buf_offset+count, buf_size, cb));
    write_byte(0xFF);
    break;

  case CN_CBOR_TAG:
    ADVANCE(cbor_encoder_write_head(buf, buf_offset, buf_size, cb->type, cb->v.uint));
    ADVANCE(_write_children(buf, buf_offset+count, buf_size, cb));
    break;

  case CN_CBOR_TEXT:
  case CN_CBOR_BYTES:
    ADVANCE(cbor_encoder_write_head(buf, buf_offset, buf_size, cb->type, cb->length));
    ensure_writable(cb->length);
    memcpy(buf+buf_offset+count, cb->v.str, cb->length);
    count += cb->length;
    break;

  case CN_CBOR_NULL:
  case CN_CBOR_FALSE:
  case CN_CBOR_TRUE:
  case CN_CBOR_UINT:
  case CN_CBOR_SIMPLE:
    ADVANCE(cbor_encoder_write_head(buf, buf_offset, buf_size, cb->type, cb->v.uint));
    break;

  case CN_CBOR_INT:
    ADVANCE(cbor_encoder_write_negative(buf, buf_offset, buf_size, cb->v.sint));
    break;
  case CN_CBOR_DOUBLE:
    ADVANCE(cbor_encoder_write_double(buf, buf_offset, buf_size, cb->v.dbl));
    break;

  case CN_CBOR_INVALID:
    return -1;
  }

  return count;
}

#ifdef  __cplusplus
}
#endif

#endif  /* CN_CBOR_C */
