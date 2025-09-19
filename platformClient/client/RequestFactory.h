#pragma once
#include "CodecFactory.h"
#include "Codec.h"
#include "RequestCodec.h"
#include <iostream>

class RequestFactory:public CodecFactory{
public:
    RequestFactory(std::string str);
    RequestFactory(RequestInfo * requestInfo);
    ~RequestFactory();
    Codec* createCodec()override;
private:
    RequestInfo * req_info;
    std::string encodc_str;
    //
    bool isEncodc;
};

  