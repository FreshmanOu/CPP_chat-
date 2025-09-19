#include"RequestFactory.h"
RequestFactory::RequestFactory(std::string str){
    encodc_str=str;
    isEncodc=false;
}
//RequestFactory通过工厂模式调用CodecFactory的createCodec方法,创建RequestCodec对象
RequestFactory::RequestFactory(RequestInfo * requestInfo){
    req_info=requestInfo;
    isEncodc=true;
}
RequestFactory::~RequestFactory(){
    
}
Codec* RequestFactory::createCodec(){
    if(!isEncodc){
        //bian ma
        return new RequestCodec(encodc_str);
    }
    else{
        //jie ma
        return new RequestCodec(req_info);
    }
}