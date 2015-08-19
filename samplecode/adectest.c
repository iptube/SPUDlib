/*
 * Copyright (c) 2015 SPUDlib authors.  See LICENSE file.
 *
 * adectest used to debog cbor creation, adectest.sh can be used to
 * decode the created binary.
 */

#include <string.h>
#include <stdio.h>

#include <netinet/ip.h>
#include <pthread.h>
#include <signal.h>
#include <stdlib.h>

#include "spud.h"
#include "tube.h"
#include "ls_error.h"
#include "ls_log.h"
#include "ls_sockaddr.h"
#include "../src/cn-cbor/cn-encoder.h"

#define MAXBUFLEN 2048


int
adectest()
{
  cn_cbor** cbor = ls_data_malloc( sizeof(cn_cbor*) );

  uint8_t ip[]    = {192, 168, 0, 0};
  uint8_t token[] = {42, 42, 42, 42, 42};
  char*   url     = "http://example.com";

  path_create_mandatory_keys(cbor, ip, 4, token, 5, url);

  if (cbor == NULL)
  {
    printf("3");
  }

  uint8_t* buf = ls_data_malloc(sizeof(uint8_t) * 1500);

  int cborlen = cbor_encoder_write(buf, 0, 1500, *cbor);

  printf("0x");
  for (int i = 0; i < cborlen; i++)
  {
    printf("%02x",buf[i]);
  }
  printf("\n");

  ls_data_free(*cbor);
  ls_data_free(cbor);


  return 0;
}


int
main(void)
{
  return adectest();
}
