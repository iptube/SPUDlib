#ifndef CN_CREATE_C
#define CN_CREATE_C

#ifdef  __cplusplus
extern "C" {
#endif


#include "cn-cbor/cn-encoder.h"
#include "cbor.h"
#include "cn-cbor/cn-create.h"


#define CN_CBOR_FAIL(code) do { errp->err = code; errp->pos = 0;  goto fail; } while(0)

static void *_cbor_calloc(size_t count, size_t size, void *context) {
    return calloc(count, size);
}

cn_cbor* cn_cbor_create_map(cn_alloc_func calloc_func, void *context, cn_cbor_errback *errp) {
	uint8_t* cb_buf;
	
	errp->err = CN_CBOR_NO_ERROR;
	
	if (calloc_func == NULL) {
      calloc_func = _cbor_calloc;
  	}

	cb_buf = calloc_func(1, sizeof(cn_cbor), context);

	if (!cb_buf)
    	CN_CBOR_FAIL(CN_CBOR_ERR_OUT_OF_MEMORY);
	
	cn_cbor* ret = (cn_cbor*)cb_buf;
	ret->type = CN_CBOR_MAP;
	ret->length = 0;
	ret->parent = NULL;  
	
	return ret;
	
	fail:
		return NULL;
}

cn_cbor* cn_cbor_create_data(cn_alloc_func calloc_func, void *context, const char* data,
	int len, cn_cbor_errback *errp) {
	
	uint8_t* cb_buf;
	
	errp->err = CN_CBOR_NO_ERROR;
	
	if (calloc_func == NULL) {
      calloc_func = _cbor_calloc;
  	}

	cb_buf = calloc_func(1, sizeof(cn_cbor), context);

	if (!cb_buf)
    	CN_CBOR_FAIL(CN_CBOR_ERR_OUT_OF_MEMORY);
	
	cn_cbor* ret = (cn_cbor*)cb_buf;
	ret->type = CN_CBOR_BYTES;
	ret->length = len;
	ret->parent = NULL;
	ret->v.str = data;
	
	return ret;
	
	fail:
		return NULL;
}

cn_cbor* cn_cbor_create_int(cn_alloc_func calloc_func, void *context, 
	int value, cn_cbor_errback *errp){
	
	uint8_t* cb_buf;
	
	errp->err = CN_CBOR_NO_ERROR;
	
	if (calloc_func == NULL) {
      calloc_func = _cbor_calloc;
  	}

	cb_buf = calloc_func(1, sizeof(cn_cbor), context);

	if (!cb_buf)
    	CN_CBOR_FAIL(CN_CBOR_ERR_OUT_OF_MEMORY);
	
	cn_cbor* ret = (cn_cbor*)cb_buf;
	ret->type = CN_CBOR_INT;
	ret->parent = NULL;
	ret->v.sint = value;
		
	return ret;
	
	fail:
		return NULL;
}

void cn_cbor_mapput_int(cn_cbor* cb_map, cn_alloc_func calloc_func, void *context,
 int key, cn_cbor* cb_value, cn_cbor_errback *errp) {
	uint8_t* cb_buf_key;
	cn_cbor* cb_key;

	//Make sure input is a map. Otherwise
	if(!cb_map || cb_map->type != CN_CBOR_MAP)
		CN_CBOR_FAIL(CN_CBOR_ERR_INVALID_PARAMETER);
		
	if (calloc_func == NULL) {
      calloc_func = _cbor_calloc;
  	}
	
	errp->err = CN_CBOR_NO_ERROR;
	errp->pos = 0;
	
	//Create key element
	cb_buf_key = calloc_func(1, sizeof(cn_cbor), context);
	if (!cb_buf_key)
    	CN_CBOR_FAIL(CN_CBOR_ERR_OUT_OF_MEMORY);
	
    cb_key = (cn_cbor*) cb_buf_key;
    cb_key->type = CN_CBOR_INT;
    cb_key->v.sint = key;
    cb_key->length = 0;
	
	//Connect key and value and insert them into the map.
	cb_key->next = cb_value;
	cb_value->parent = cb_key;
	cb_value->next = NULL;
	
	if(!cb_map->last_child){ //This is the first item
		cb_map->next = cb_key;
		cb_map->first_child = cb_key;
		cb_map->last_child = cb_value;
		cb_key->parent = cb_map;
	}
	else{ //There are already items in the map.
		cb_key->parent = cb_map->last_child;
		cb_map->last_child->next = cb_key;
		cb_map->last_child = cb_value;
	}
	
	cb_map->length++;
	
	return;
	
	fail:
		return;

}

#ifdef  __cplusplus
}
#endif

#endif  /* CN_CBOR_C */
