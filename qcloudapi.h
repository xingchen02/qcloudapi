#ifndef __QCLOUDAPI_H
#define __QCLOUDAPI_H

#include <string.h>
#include <stdio.h>
#include <time.h>
#include <errno.h>
#include <stdint.h> 
#include <openssl/sha.h>
#include <openssl/hmac.h>
#include <curl/curl.h>

/* Error values */
enum cloudApiError {
	ERR_SUCCESS = 0,
	ERR_NOMEM = 1,
	ERR_INVAL = 2,
};

struct httpUserData {
    char *data;
    size_t size;
};

//default http head param define
//you can define private headparam if you need in your code
struct defaultHeadParam{
	  char *host; //host must be the first param if define your private head
    char *service;
    char *action;
    char *version;
};

//no default http body param, use your private define


//cloud api
struct cloudApi{
    char *SECRET_ID;
    char *SECRET_KEY;
    void *httpHeadParam;
    void *httpBodyParam;
    int (*httpBodyCb)(struct cloudApi *cloud, void *bodyParam);         //register your private httpbody func
    int (*httpSignatureHeadCb)(struct cloudApi *cloud, void *headParam);//redister your private httphead func
    char *httpBody;                                                     // init or assign in your private httpbody func
    struct curl_slist *httpSignatureHead;                               // init or assign in your private httphead func
    struct httpUserData *httpResponseData;                              // save the http reply value
};

struct cloudApi *cloudApiNew(void);
void cloudApiFree(struct cloudApi *cloud);

//default http signature head handle function, you can define yourself by reference to this one
int defaultHttpSignatureHead(struct cloudApi *cloud, void *headParam);    

int cloudApiSecretSet(struct cloudApi *cloud, const char *SECRET_ID, const char *SECRET_KEY);

int cloudApiHttpBodyInit(struct cloudApi *cloud, void *bodyParam, int (*bodyCb)(struct cloudApi *cloud, void *bodyParam));
int cloudApiHttpHeadInit(struct cloudApi *cloud, void *headParam, int (*headCb)(struct cloudApi *cloud, void *headParam));
char *cloudApiHttpPost(struct cloudApi *cloud);

#endif
