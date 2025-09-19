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
    if(isencode){
        return new RespondCodec(Res_info);
    }else{
        return new RespondCodec(enc_str)  ;
    }
}