#ifndef CN_CREATE_H
#define CN_CREATE_H

#ifdef  __cplusplus
extern "C" {
#endif

#include "cn-cbor/cn-cbor.h"

cn_cbor* cn_cbor_create_map(cn_alloc_func calloc_func, void *context,
	cn_cbor_errback *errp);
cn_cbor* cn_cbor_create_data(cn_alloc_func calloc_func, void *context, 
	const char* data, int len, cn_cbor_errback *errp);
cn_cbor* cn_cbor_create_int(cn_alloc_func calloc_func, void *context, 
	int value, cn_cbor_errback *errp);

//Put a cn_cbor value with an int key into the map.
void cn_cbor_mapput_int(cn_cbor* cb_map, cn_alloc_func calloc_func, void *context,
	int key, cn_cbor* cb_value, cn_cbor_errback *errp);
//Put a cn_cbor value with a string key into the map.
void cn_cbor_mapput_string(cn_cbor* cb_map, cn_alloc_func calloc_func, void *context,
	char* key, cn_cbor* cb_value, cn_cbor_errback *errp);


#ifdef  __cplusplus
}
#endif

#endif  /* CN_CREATE_H */
