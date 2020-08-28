# qcloudapi
# 腾讯云服务器 API3.0 C语言版本

### 腾讯云服务器 API处理的流程：
##### 1）初始化变量，主要有三类：1 密钥参数，2 构成请求头（http head）的参数，3 构成请求数据（http body）的参数
##### 2）生成请求数据（http body），请求数据的参数和请求数据生成的函数都是私有自定义
##### 3）生成签名的请求头（http head），头部参数和签名头部处理函数可以使用API提供的也可以参考API定义私有的
##### 4）发送http post请求，并返回云服务器的响应数据

### 具体处理Demo：发送短信，参见mian.c：
##### {
    // 密钥参数
    const char* SECRET_ID = "AKID*******";    //cloud provider define
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
    //for test 2
    //7)renew http body parameter demo(your private define if needed)
    smsBodyPhoneVcodeRenew(smsBody, testPhone2, testVcode2);
    
    //8)http post and return reply
    resp = cloudApiHttpPost(smsApi);
    
    if(resp){
       printf("%s\n", "resp2-----------------------");        
       printf("%s\n\n", resp);
    }
    
    //9)release
    smsHeadParamFree(smsHead);
    smsBodyParamFree(smsBody);
    cloudApiFree(smsApi);
    
    return ERR_SUCCESS;
##### }

### 注意：
##### 1）私有定义的数据的内存分配，API代码不做资源维护处理，由你自己的代码处理：

###### struct defaultHeadParam *smsHead = smsHeadParamNew();
###### struct smsBodyParam *smsBody = smsBodyParamNew();

###### smsHeadParamFree(smsHead);
###### smsBodyParamFree(smsBody);

##### 2）cloudapi数据的内存分配，由API维护，外部仅使用即可，在程序结束时显式调用：
###### cloudApiFree(smsApi);

##### 3）对外部库的要求：
###### libcurl，版本不低于7.45.0：https://curl.haxx.se/download/curl-7.45.0.tar.bz2
###### 选项注意：
###### 1，--enable-ares dns解析使用异步库，避免网络异常curl_easy_perform超时，socket泄漏，参考：
###### gethostbyname阻塞分析 https://www.jianshu.com/p/4c21e1f58f6e
###### 2，当在你的主程序中使用了openssl时，在curl如果也选用openssl库会出现异常，因此编译curl库建议选用其选项中的：
PolarSSL
###### 3，可能因为实际需要，你还需要集成cjson库（序列化和反序列化http body），libiconv：将http body转成utf-8

##### 4）最常见的错误：
###### {"Response":{"Error":{"Code":"AuthFailure.SignatureFailure","Message":"The provided credentials could not be validated. Please check your signature is correct."},"RequestId":"9ee538f4-1814-458a-a811-22ac0269dacd"}} 
###### 可以使用腾讯云提供的“签名串生成”功能（登录你的云账号腾讯云提供了该调试功能）进行调试，对比网页生成的数据和代码打印的数据，找到不一致的地方进行修改

###### 另外从开发者角度一点拙见：腾讯云优于阿里云优于华为云

