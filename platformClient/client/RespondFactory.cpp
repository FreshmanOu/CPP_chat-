#include "RespondFactory.h"

RespondFactory::~RespondFactory()
{
}
RespondFactory::RespondFactory(){
    
}
RespondFactory::RespondFactory(std::string str){
    enc_str=str;
    isencode = false;
}
RespondFactory::RespondFactory(RespondInfo* info){
    Res_info=  info;
    isencode = true;

}

Codec* RespondFactory::createCodec(){
    Codec* codec = nullptr;
    if(isencode){
        codec = new RespondCodec();
    }else{
        codec = new RespondCodec(enc_str)  ;
    }
    return codec;
}