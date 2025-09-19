#include"RequestCodec.h"
RequestCodec::RequestCodec(std::string str){
    initMessage(str);
}
RequestCodec::RequestCodec(RequestInfo *info){
    
    initMessage(info);
}
RequestCodec::~RequestCodec(){

}

void RequestCodec::initMessage(std::string str){
    encodec_str = str;
}
/*
    int cmd;
	std::string clientID;
	std::string serverID;
	std::string sign;
	std::string data;
*/
void RequestCodec::initMessage(RequestInfo *info){
    
    
    
    Req_Info.set_cmdtype(info->cmd);
    Req_Info.set_clientid(info->clientID);
    Req_Info.set_serverid(info->serverID);
    Req_Info.set_sign(info->sign);
    Req_Info.set_data(info->data);

    
}
//解码
void *RequestCodec::decodeMsg(){
    RequestMsg* msg = new RequestMsg();
     if(msg!=nullptr)
     {
        msg->ParseFromString(encodec_str);
     }
    return msg;
}
//编码
std::string RequestCodec::encodeMsg(){
    std::string str;
    Req_Info.SerializeToString(&str);
    return str;
}