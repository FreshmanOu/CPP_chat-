#pragma once
#include "CodecFactory.h"
#include "Codec.h"
#include "RespondCodec.h"
#include <iostream>

class RespondFactory:public CodecFactory{
public:
    RespondFactory();
    RespondFactory(std::string str);
    RespondFactory(RespondInfo* info);
    virtual Codec* createCodec()override;
    ~RespondFactory();
private:
    bool isencode;
    std::string enc_str;//序列化之后的数据
    RespondInfo* Res_info;//反序列化之后的数据

};
