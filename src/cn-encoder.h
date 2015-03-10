#ifndef CN_ENCODER_H
#define CN_ENCODER_H

#ifdef  __cplusplus
extern "C" {
#endif
#ifdef EMACS_INDENTATION_HELPER
} /* Duh. */
#endif

#include <stdlib.h>
#include <sys/types.h>

#include "cn-cbor.h"

/* returns -1 on fail, or number of bytes written */
ssize_t cbor_encoder_write_head(uint8_t *buf,
                                size_t buf_offset,
                                size_t buf_size,
                                cn_cbor_type typ,
                                uint64_t val);

ssize_t cbor_encoder_write_negative(uint8_t *buf,
                                    size_t buf_offset,
                                    size_t buf_size,
                                    int64_t val);

ssize_t cbor_encoder_write_double(uint8_t *buf,
                                  size_t buf_offset,
                                  size_t buf_size,
                                  double val);

ssize_t cbor_encoder_write(uint8_t *buf,
                           size_t buf_offset,
                           size_t buf_size,
                           const cn_cbor *cb);

#ifdef  __cplusplus
}
#endif

#endif  /* CN_ENCODER_H */
