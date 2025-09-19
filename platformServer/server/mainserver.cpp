#include"serverOP.h"
#include<cstdio>
int main()
{
    ServerOP server("server.json");
    server.startServer();
    return 0;
}