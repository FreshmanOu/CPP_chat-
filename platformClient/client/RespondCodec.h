#pragma once
#include"Codec.h"
#include"message.pb.h"
class RespondCodec : public Codec{
public:
    RespondCodec();
    RespondCodec(std::string str);
    RespondCodec(RespondInfo*req);
    ~RespondCodec();

    void* decodeMsg()override;
    std::string encodeMsg()override;

    void initMessage(std::string str); //初始化消息
    void initMessage(RespondInfo *res) ;//初始化消息

private:
    RespondMsg Res_msg;
    std::string Res_str;
};