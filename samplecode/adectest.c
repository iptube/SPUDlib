/*
 * Copyright (c) 2015 SPUDlib authors.  See LICENSE file.
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

#ifdef ANDROID
#include <jni.h>
#include <android/log.h>
#include "stdio.h"
#define TAG "hiut"
#define LOGV(...) __android_log_print(ANDROID_LOG_VERBOSE, TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)

#define ESC_7C 	""

#else
#define LOGI(...) printf(__VA_ARGS__)
#define LOGV(...) ls_log(LS_LOG_VERBOSE, __VA_ARGS__)
#define LOGE(...) ls_log(LS_LOG_ERROR, __VA_ARGS__)

#define ESC_7C 	"\033[7C"

#endif

#define MAXBUFLEN 2048


int adectest(int argc, char **argv)
{
    if (argc==0) fprintf(stderr,"1");
    if (argv==NULL) fprintf(stderr,"2");
    cn_cbor **cbor=ls_data_malloc(sizeof(cn_cbor*));
    
    static uint8_t ip[]   = {192, 168, 0, 0};   
    static uint8_t token[] = {42, 42, 42, 42, 42}; 
    static char * url= "http://example.com";
    
    path_create_mandatory_keys(cbor, ip, 4, token, 5, url);
    
    if (cbor==NULL) printf("3");
    
    uint8_t *buf=ls_data_malloc(sizeof(uint8_t)*1500);
    
    int cborlen=cbor_encoder_write(buf, 0, 1500, *cbor);

    printf("0x");
    for (int i=0;i<cborlen;i++) {
        printf("%02x",buf[i]);
    }    
    printf("\n");        

    ls_data_free(*cbor);
    ls_data_free(cbor);
    
    
    return 0;
}

#ifdef ANDROID
int traceroute(const char* hostname, int port)
{
    char *argv[2];
    int argc = 2;

    argv[1] = (char*)malloc(1024);
    strcpy(argv[1], hostname);

    adectest(argc, argv);

    free(argv[1]);
    return 0;
}

#else
int main(int argc, char **argv)
{
    return adectest(argc, argv);
}
#endif
