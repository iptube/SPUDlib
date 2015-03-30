#ifndef CN_CREATE_H
#define CN_CREATE_H

#ifdef  __cplusplus
extern "C" {
#endif

#include "cn-cbor/cn-cbor.h"

cn_cbor* cn_cbor_map_create(CBOR_CONTEXT_COMMA cn_cbor_errback *errp);

cn_cbor* cn_cbor_data_create(const char* data, int len
							 CBOR_CONTEXT,
							 cn_cbor_errback *errp);
cn_cbor* cn_cbor_int_create(int value
							CBOR_CONTEXT,
	                        cn_cbor_errback *errp);

void cn_cbor_mapput_int(cn_cbor* cb_map,
	                    int key, cn_cbor* cb_value
						CBOR_CONTEXT,
						cn_cbor_errback *errp);

void cn_cbor_mapput_string(cn_cbor* cb_map,
	                       char* key, cn_cbor* cb_value
						   CBOR_CONTEXT,
						   cn_cbor_errback *errp);

#ifdef  __cplusplus
}
#endif

#endif  /* CN_CREATE_H */
