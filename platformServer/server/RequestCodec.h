#pragma once
#include"Codec.h"
#include"message.pb.h"
class RequestCodec:public Codec
{
public:
    //构造函数,初始化信息
    RequestCodec(std::string str);
    RequestCodec(RequestInfo *info);;
    ~RequestCodec();

    std::string encodeMsg()override;
    void*decodeMsg()override;

    void initMessage(std::string str);
    void initMessage(RequestInfo *info);

private:
    RequestMsg Req_Info;
    //yong lai 编码 

    //yong lai 解码
    std::string encodec_str;
};
