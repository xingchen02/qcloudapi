#include "qcloudapi.h"

//************reference from:
//***************************https://cloud.tencent.com/document/api/213/30654*********//

static int get_data(time_t *timestamp, char *buff, int size)
{
    struct tm sttime;
    
    gmtime_r(timestamp, &sttime);

    strftime(buff, size, "%Y-%m-%d", &sttime);
 
    return ERR_SUCCESS;
}

static char *int2str(int64_t values)
{
    int len = 0;
    const char digits[11] = "0123456789";
    int64_t tvalue = values;
    
    while(tvalue >= 100)
    {
        tvalue /= 100;
        len += 2;
    }
    if (tvalue > 10)
        len += 2;
    else if(tvalue > 0)
        len++;
 
    char *crtn = malloc(sizeof(char)*(len+1));
    
    if(!crtn){
    	errno = ENOMEM;
    	return NULL;
    }
    	
    char *drtn = crtn;
    char *ertn;
    
    drtn += len;
    *drtn = '\0';
    do
    {
        *--drtn = digits[values%10];
    } while (values /= 10);
    
    ertn = strdup(drtn);
    free(crtn);
    
    return ertn;
}


static char *sha256Hex(const char *str)
{
    unsigned char hash[SHA256_DIGEST_LENGTH];
    int i;
    
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, str, strlen(str));
    SHA256_Final(hash, &sha256);
    
    char *sh256_out = malloc(SHA256_DIGEST_LENGTH*2 + 1);
    
    if(!sh256_out){
    	errno = ENOMEM;
    	return NULL;
    }	
    
    for(i = 0; i < SHA256_DIGEST_LENGTH; i++)
    {
    	  sprintf (sh256_out + (i*2),  "%02x" , hash[i]);
    }
    
    sh256_out[SHA256_DIGEST_LENGTH*2] = '\0';
    
    return sh256_out;
}

static char *HmacSha256(const char *key, const char *input)
{
    unsigned char hash[32];
    int i;
    
    HMAC_CTX *h;
#if OPENSSL_VERSION_NUMBER < 0x10100000L
    HMAC_CTX hmac;
    HMAC_CTX_init(&hmac);
    h = &hmac;
#else
    h = HMAC_CTX_new();
#endif

    HMAC_Init_ex(h, key, strlen(key), EVP_sha256(), NULL);
    HMAC_Update(h, input, strlen(input));
    unsigned int len = 32;
    HMAC_Final(h, hash, &len);

#if OPENSSL_VERSION_NUMBER < 0x10100000L
    HMAC_CTX_cleanup(h);
#else
    HMAC_CTX_free(h);
#endif

    char* crtn = malloc(sizeof(char)*(len+1));
    if(!crtn){
    	errno = ENOMEM;
    	return NULL;
    }	
    char* out_hash = crtn;
    
    for (i = 0; i < len; i++)
    {
    	  *(out_hash + i) = hash[i];
        //sprintf (out_hash + (i*1),  "%c" , hash[i]);
    }
    
    out_hash[len] = '\0';

    return (out_hash);
}

static char *HexEncode(const char *input, size_t len)
{
    static const char* const lut = "0123456789abcdef";
    int i;
    //maybe input have '\0' char inside, so can not use strlen here
    //size_t len = strlen(input); 

    char * output = malloc(sizeof(char) *2*len + 1);
    if(!output){
    	errno = ENOMEM;
    	return NULL;
    }	
    
    for (i = 0; i < len; ++i)
    {
        const unsigned char c = input[i];
        *(output + (i*2)) = lut[c >> 4];
        *(output + (i*2) + 1) = lut[c & 15];
        //sprintf (output + (i*2),  "%c" , lut[c >> 4]);
        //sprintf (output + (i*2)+1,  "%c" , lut[c & 15]);
    }
    
    output[2*len] = '\0';
    return output;
}

int defaultHttpSignatureHead(struct cloudApi *cloud, void *headParam)
{   
    const char* httpRequestMethod = "POST";
    const char* canonicalUri = "/";
    const char* canonicalQueryString = "";
    const char* signedHeaders = "content-type;host";
    
    const char *SECRET_ID = NULL;
    const char *SECRET_KEY = NULL;
    
    const char *service = NULL;
    const char *host = NULL;
    const char *action = NULL;
    const char *version = NULL;
    
    struct defaultHeadParam *defHeadParam = NULL;
    
    if(!cloud || !headParam) return ERR_INVAL;
    
    defHeadParam = (struct defaultHeadParam *)headParam;
    service = defHeadParam->service;
    host = defHeadParam->host;
    action = defHeadParam->action;
    version = defHeadParam->version;
    
    SECRET_ID = cloud->SECRET_ID;
    SECRET_KEY = cloud->SECRET_KEY;
    
    // ************* 步骤 1：拼接规范请求串 *************
    char canonicalHeaders[128] = {0};
    char canonicalRequest[256 + SHA256_DIGEST_LENGTH*2] = {0};
    const char* playload = NULL;
    char *hashedRequestPayload = NULL;
    
    //注意：CanonicalHeaders按规范需使用小写字母    
    sprintf(canonicalHeaders,"%s%s%s", "content-type:application/json; charset=utf-8\nhost:" , host , "\n");
    
    playload = cloud->httpBody;
    hashedRequestPayload = sha256Hex(playload);
    
    if(!hashedRequestPayload){
        return ERR_NOMEM;
    }
    
    snprintf(canonicalRequest, sizeof(canonicalRequest), "%s%s%s%s%s%s%s%s%s%s%s", httpRequestMethod , "\n" , canonicalUri , "\n" , canonicalQueryString , "\n"
            , canonicalHeaders , "\n" , signedHeaders , "\n" , hashedRequestPayload);
    
    free(hashedRequestPayload);
    
    printf("%s\n", "canonicalRequest-----------------------");
    printf("%s\n\n", canonicalRequest);
    
    // ************* 步骤 2：拼接待签名字符串 *************
    time_t timestamp = time(NULL); 
    char dataBuf[32] = {0};
    char timestampBuf[64] = {0};
    char credentialScope[128] = {0};
    char stringToSign[256] = {0};
    const char *algorithm = "TC3-HMAC-SHA256";
    char *hashedCanonicalRequest = NULL;
    
    const char *date = NULL;
    char *RequestTimestamp = NULL;
    
    get_data(&timestamp, dataBuf, sizeof(dataBuf));
    date = dataBuf;
    
    snprintf(credentialScope, sizeof(credentialScope), "%s%s%s%s%s", date , "/" , service , "/" , "tc3_request");
    hashedCanonicalRequest = sha256Hex(canonicalRequest);
    
    RequestTimestamp = int2str(timestamp);
    strncpy(timestampBuf, RequestTimestamp, sizeof(timestampBuf) -1);
    free(RequestTimestamp);
    RequestTimestamp = timestampBuf;
    
    snprintf(stringToSign, sizeof(stringToSign), "%s%s%s%s%s%s%s", algorithm , "\n" , RequestTimestamp , "\n" , credentialScope , "\n" , hashedCanonicalRequest);
    
    free(hashedCanonicalRequest);
    
    printf("%s\n", "stringToSign-----------------------");
    printf("%s\n\n", stringToSign);
    
    // ************* 步骤 3：计算签名 ***************
    char kKey[128] = {0};

    sprintf(kKey, "%s%s", "TC3" , SECRET_KEY);
    char * kDate = HmacSha256(kKey, date);
    char * kService = HmacSha256(kDate, service);
    char * kSigning = HmacSha256(kService, "tc3_request");
    char * signature = HexEncode(HmacSha256(kSigning, stringToSign), 32);
        
    free(kDate);
    free(kService);
    free(kSigning);
    
    printf("%s\n", "signature-----------------------");
    printf("%s\n\n", signature);
    
    // ************* 步骤 4：拼接 Authorization *************
    char authorization[1024] = {0};
    char headers[512] = {0};
    
    sprintf(authorization, "%s%s%s%s%s%s%s%s%s%s%s%s", algorithm , " " , "Credential=" , SECRET_ID , "/" , credentialScope , ", "
            , "SignedHeaders=" , signedHeaders , ", " , "Signature=" , signature);
    
    free(signature);
    
    printf("%s\n", "authorization-----------------------");        
    printf("%s\n\n", authorization);
    
    // ************* 步骤 5：拼接 Head *************
    
    if(cloud->httpSignatureHead){
    	curl_slist_free_all(cloud->httpSignatureHead);
    	cloud->httpSignatureHead = NULL;
    }
    	
    sprintf(headers, "Authorization: %s", authorization);
    cloud->httpSignatureHead = curl_slist_append(cloud->httpSignatureHead, headers);
    
    sprintf(headers, "Content-Type: application/json; charset=utf-8"); //must be same with canonicalHeaders
    cloud->httpSignatureHead = curl_slist_append(cloud->httpSignatureHead, headers);
    
    sprintf(headers, "Host: %s", host);
    cloud->httpSignatureHead = curl_slist_append(cloud->httpSignatureHead, headers);
    
    sprintf(headers, "X-TC-Action: %s", action);
    cloud->httpSignatureHead = curl_slist_append(cloud->httpSignatureHead, headers);
    
    sprintf(headers, "X-TC-Timestamp: %s", RequestTimestamp);
    cloud->httpSignatureHead = curl_slist_append(cloud->httpSignatureHead, headers);
    
    sprintf(headers, "X-TC-Version: %s", version);
    cloud->httpSignatureHead = curl_slist_append(cloud->httpSignatureHead, headers);
       
    return ERR_SUCCESS;
}

struct cloudApi *cloudApiNew(void)
{
	struct cloudApi *cloud = NULL;

	cloud = (struct cloudApi *)malloc(sizeof(struct cloudApi));
	if(cloud){
		cloud->SECRET_ID = NULL;
    cloud->SECRET_KEY = NULL;
    cloud->httpBody = NULL;
    cloud->httpSignatureHead = NULL;
    cloud->httpResponseData = NULL;
    cloud->httpHeadParam = NULL;
    cloud->httpBodyParam = NULL;
    cloud->httpBodyCb = NULL;
    cloud->httpSignatureHeadCb = NULL;
    
    /* we use curl, and curl_global_init not safe under multi thread, so init here*/ 
    curl_global_init(CURL_GLOBAL_ALL);
    
	}else{
		errno = ENOMEM;
	}
	return cloud;
}

void cloudApiFree(struct cloudApi *cloud)
{
	struct httpUserData *httpUserp;
	
	if(cloud){
		if(cloud->SECRET_ID) free(cloud->SECRET_ID);
		if(cloud->SECRET_KEY) free(cloud->SECRET_KEY);

    if(cloud->httpBody) free(cloud->httpBody);
    if(cloud->httpSignatureHead) curl_slist_free_all(cloud->httpSignatureHead);
    if(cloud->httpResponseData){
    	httpUserp = cloud->httpResponseData;
    	if(httpUserp->data) free(httpUserp->data);
    	
    	free(httpUserp);
    }
    
    curl_global_cleanup();
	}
}
  
    
int cloudApiSecretSet(struct cloudApi *cloud, const char *SECRET_ID, const char *SECRET_KEY)
{
  if(!cloud || !SECRET_ID || !SECRET_KEY)	return ERR_INVAL;
  
  cloud->SECRET_ID = strdup(SECRET_ID);
  if(! cloud->SECRET_ID)
  	return ERR_NOMEM;
  	
  cloud->SECRET_KEY = strdup(SECRET_KEY);
  if(!cloud->SECRET_KEY){
    free(cloud->SECRET_ID);	
    return ERR_NOMEM;
  }
  
  return ERR_SUCCESS;
}

int cloudApiHttpBodyInit(struct cloudApi *cloud, void *bodyParam, int (*bodyCb)(struct cloudApi *cloud, void *bodyParam))
{
	if(!cloud || !bodyParam)	return ERR_INVAL;
	
  cloud->httpBodyParam = bodyParam;
  
  if(bodyCb) cloud->httpBodyCb = bodyCb;
  
  return ERR_SUCCESS;
}

int cloudApiHttpHeadInit(struct cloudApi *cloud, void *headParam, int (*headCb)(struct cloudApi *cloud, void *headParam))
{
	if(!cloud || !headParam)	return ERR_INVAL;
		
	cloud->httpHeadParam = headParam;
	
	if(headCb) cloud->httpSignatureHeadCb = headCb;
		
	return ERR_SUCCESS;
}

static size_t httpDataCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
    size_t realsize = size * nmemb;
    struct httpUserData *httpUserp = (struct httpUserData *)userp;
    char *ptr = realloc(httpUserp->data, httpUserp->size + realsize + 1);

    if (ptr == NULL){
        printf("alloc error\n");
        return 0;
    }

    httpUserp->data = ptr;
    memcpy(&(httpUserp->data[httpUserp->size]), contents, realsize);
    httpUserp->size += realsize;
    httpUserp->data[httpUserp->size] = 0;

    return realsize;
}

static char *httpPost(struct cloudApi *cloud)
{
    CURL        *curl;
    CURLcode    res;
    
    struct      httpUserData        *httpUserData;
    struct      defaultHeadParam    *defHeadParam;
    struct      curl_slist          *httpHead;
    char        *host = NULL;
    char        *body = NULL;
    
    if(!cloud) return NULL;
    
    httpUserData = cloud->httpResponseData;
    defHeadParam = cloud->httpHeadParam;
    httpHead = cloud->httpSignatureHead;
    body = cloud->httpBody;
    
    if(!defHeadParam || !httpHead || !body) return NULL;
    
    host = defHeadParam->host;
    
    if(!host) return NULL;
    	
    if(!httpUserData){
        	cloud->httpResponseData = (struct httpUserData *)malloc(sizeof(struct httpUserData));
        	if(!cloud->httpResponseData) return NULL;
        	
        	httpUserData = cloud->httpResponseData;       	
    }else{
          free(httpUserData->data);	
    }
    
    httpUserData->data = malloc(1);  /* will be grown as needed by the realloc in callback func */
    httpUserData->size = 0;               /* no data at this point */

    /* get a curl handle */ 
    curl = curl_easy_init();
    
    if(!curl){
    	fprintf(stderr, "curl_easy_init() failed\n");
    	curl_global_cleanup();
    	return NULL;
    }	
    
	  /* Now specify the Host */
    curl_easy_setopt(curl, CURLOPT_URL, host);
    
    /* set the default protocol (scheme) for schemeless URLs : bcz host not include https://*/
    /* this option Added in curl v7.45.0*/
    curl_easy_setopt(curl, CURLOPT_DEFAULT_PROTOCOL, "https");

    /* set custom HTTP headers */
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, httpHead);

    /* Now specify the POST data */ 
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body);
    
    /* send all data to this function  */ 
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, httpDataCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)httpUserData);
    
    /*for debug*/
    curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
    
    /*set post*/
    curl_easy_setopt(curl, CURLOPT_POST, 1);

    /*for Error : * Malformed encoding found in chunked-encoding*/
    curl_easy_setopt(curl, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_0);

    /* set maximum time the request is allowed to take
      use c-ares to avoid dns socket leak*/
    curl_easy_setopt(curl,CURLOPT_TIMEOUT,6L);

    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);

    /* Perform the request, res will get the return code */ 
    res = curl_easy_perform(curl);
    /* Check for errors , always error 56, why?*/ 
    if(res != CURLE_OK){
    	fprintf(stderr, "curl_easy_perform() failed: %s res=%d\n",curl_easy_strerror(res), res);
    	free(httpUserData->data);
      httpUserData->data = NULL; //for return NULL
    }
    
    /* always cleanup */ 
    curl_easy_cleanup(curl);
    
    return httpUserData->data;
}

char *cloudApiHttpPost(struct cloudApi *cloud)
{  
	  char *response = NULL;
	  int rc = ERR_INVAL;
	  
  	if(!cloud) return NULL;
  	
  	if(cloud->httpBodyCb && cloud->httpBodyParam)
  	  rc = cloud->httpBodyCb(cloud, cloud->httpBodyParam);
  	
  	if(rc != ERR_SUCCESS) 
  		return NULL;
  		
  	if(cloud->httpSignatureHeadCb && cloud->httpHeadParam)
  		rc = cloud->httpSignatureHeadCb(cloud, cloud->httpHeadParam);
  		
  	if(rc != ERR_SUCCESS){
  	  return NULL;
  	}
    
  	response = httpPost(cloud);
  	
  	return response;
}
