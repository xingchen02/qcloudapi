#include <string.h>
#include <stdio.h>
#include <time.h>
#include <errno.h>

#include "qcloudapi.h"

struct smsBodyParam{
	  char *phoneNum;
    char *templId;
    char *templVCode;
    char *templTime;
    char *templServicer;
    char *appId;
    char *sign;	
};

int smsHttpBody(struct cloudApi *cloud, void *bodyParam)
{
	  struct smsBodyParam *smsBody = NULL; 
	  char *smsBuffer = NULL;
	  
	  if(!cloud || !bodyParam) return ERR_INVAL;
	  
		smsBuffer = malloc(512);
		if(!smsBuffer) return ERR_NOMEM;
			
		smsBody = (struct smsBodyParam *) bodyParam;
		
		//just a demo, you can use cjson here, maybe you should encode smsBuffer to utf-8 
		snprintf(smsBuffer, 512, "{\"PhoneNumberSet\":[\"%s\"],\"TemplateID\":\"%s\"," \
                     "\"Sign\":\"%s\",\"TemplateParamSet\":[\"%s\",\"%s\",\"%s\"],\"SmsSdkAppid\":\"%s\"}", 
                     smsBody->phoneNum,           
                     smsBody->templId,
                     smsBody->sign,           
                     smsBody->templVCode,          
                     smsBody->templTime,                
                     smsBody->templServicer,
                     smsBody->appId
                     );
                     
    if(cloud->httpBody) free(cloud->httpBody);
    
    cloud->httpBody = smsBuffer;
    
    printf("%s\n", "body-----------------------");
    printf("%s\n\n", cloud->httpBody);
    
    return ERR_SUCCESS;
}

struct defaultHeadParam *smsHeadParamNew(void)
{
	  // sms head parameter: "sms", "sms.tencentcloudapi.com", "SendSms", "2019-07-11"
    const char *service = "sms";                   //cloud provider define
    const char *host = "sms.tencentcloudapi.com";  //cloud provider define
    const char *action ="SendSms";                 //cloud provider define
    const char *version ="2019-07-11";             //cloud provider define
    
    struct defaultHeadParam *smsHead = (struct defaultHeadParam *)malloc(sizeof(struct defaultHeadParam));
    if(! smsHead) return NULL;
    
    smsHead->service = strdup(service);
    smsHead->host = strdup(host);
    smsHead->action = strdup(action);
    smsHead->version = strdup(version);
    
    return smsHead;
}

void smsHeadParamFree(struct defaultHeadParam *smsHead)
{
	 if(smsHead){
	     	free(smsHead->service);
        free(smsHead->host);
        free(smsHead->action);
        free(smsHead->version);
        
        free(smsHead);
	 }
}

struct smsBodyParam *smsBodyParamNew(void)
{
	  // sms body parameter, the templ* and sign must same with your sms templ
    const char *phoneNum = "+86133*******1"; //user define: test phone num 1
    const char *templId = "306328";          //cloud provider define
    const char *templVCode = "521521";       //user define: test vcode 1
    const char *templTime = "5";             //user define: vcode timeout
    const char *templServicer = "0789-678*****";        //user define: customer service phone number
    const char *appId = "14001*****";                   //cloud provider define
    const char *sign = "测试签名";                      //cloud provider define
    
    struct smsBodyParam *smsBody = (struct smsBodyParam *)malloc(sizeof(struct smsBodyParam));
    if(! smsBody) return NULL;
    
    smsBody->phoneNum = strdup(phoneNum);           
    smsBody->templId = strdup(templId);
    smsBody->templVCode = strdup(templVCode);          
    smsBody->templTime = strdup(templTime);                
    smsBody->templServicer = strdup(templServicer);
    smsBody->appId = strdup(appId);           
    smsBody->sign = strdup(sign);    
    
    return smsBody;
}

//modify parameter demo
int smsBodyPhoneVcodeRenew(struct smsBodyParam *smsBody, const char *phone, const char *vcode)
{
	if(!smsBody || !phone || !vcode) return ERR_INVAL;
		
	if(smsBody->phoneNum)
		free(smsBody->phoneNum);
		
	if(smsBody->templVCode)
		free(smsBody->templVCode);
		
	smsBody->phoneNum = strdup(phone);           
  smsBody->templVCode = strdup(vcode); 
  
  return ERR_SUCCESS;
}

void smsBodyParamFree(struct smsBodyParam *smsBody)
{
	  if(smsBody){
	      free(smsBody->phoneNum);           
        free(smsBody->templId);
        free(smsBody->templVCode);          
        free(smsBody->templTime);                
        free(smsBody->templServicer);
        free(smsBody->appId);           
        free(smsBody->sign);
        
        free(smsBody); 	
	  }
}

int main(void)
{
    // 密钥参数
    const char* SECRET_ID = "AKID*******"; //cloud provider define
    const char* SECRET_KEY = "oNg*******";    //cloud provider define
    
    const char* testPhone2 = "+86133*******2";    //test value 2
    const char* testVcode2 = "520520";            //test value 2
    
    char *resp = NULL;
    
    //1)new private parameter, private parameter are used by private callback func
    struct defaultHeadParam *smsHead = smsHeadParamNew();
    struct smsBodyParam *smsBody = smsBodyParamNew();
    
    //2)new cloud api
    struct cloudApi *smsApi = cloudApiNew();
    
    if(!smsHead || !smsBody || !smsApi) return ERR_NOMEM;
    
    //3)set cloud api id and key
    cloudApiSecretSet(smsApi, SECRET_ID, SECRET_KEY);
    
    //4)set private parameter and callback func
    cloudApiHttpBodyInit(smsApi, smsBody, smsHttpBody);
    cloudApiHttpHeadInit(smsApi, smsHead, defaultHttpSignatureHead);
    
    //5)http post and return reply
    resp = cloudApiHttpPost(smsApi);
    
    //6)use reply value, do not need free it in you code
    //just a demo, you can use cjson to handle resp here
    if(resp){
       printf("%s\n", "resp1-----------------------");        
       printf("%s\n\n", resp);
    }
 #if 0   //for test 2
    //7)renew http body parameter parameter demo
    smsBodyPhoneVcodeRenew(smsBody, testPhone2, testVcode2);
    
    //8)http post and return reply
    resp = cloudApiHttpPost(smsApi);
    
    if(resp){
       printf("%s\n", "resp2-----------------------");        
       printf("%s\n\n", resp);
    }
 #endif   
    //9)release
    smsHeadParamFree(smsHead);
    smsBodyParamFree(smsBody);
    cloudApiFree(smsApi);
    
    return ERR_SUCCESS;
};
