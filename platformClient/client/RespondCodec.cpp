#include"RespondCodec.h"
RespondCodec::RespondCodec(){

}
RespondCodec::RespondCodec(std::string str){
    initMessage(str);
}
RespondCodec::RespondCodec(RespondInfo*req){
    initMessage(req);
}
RespondCodec::~RespondCodec(){
    
}
void* RespondCodec::decodeMsg(){
    RespondMsg* msg = new RespondMsg();
    if (msg != nullptr)
    {
        msg->ParseFromString(Res_str);
    }
    return msg;
}
std::string RespondCodec::encodeMsg(){
    std::string str;
    Res_msg.SerializeToString(&str);
    return str; //编码
}


void RespondCodec::initMessage(std::string str){
    
    Res_str=str;
}

void RespondCodec::initMessage(RespondInfo *res){
    Res_msg.set_clientid(res->clientID);
    Res_msg.set_status(res->status);
    Res_msg.set_serverid(res->serverID);
    Res_msg.set_data(res->data);
    Res_msg.set_seckeyid(res->seckeyID);
}