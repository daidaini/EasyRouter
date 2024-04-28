#include "RouterServer.h"

#include "RtGlobalResource.h"

bool g_IsIPV6 = false;
double g_kOnTimerInterval = 10.0; // 秒

int main(int argc, char *argv[])
{
    g_Global.Init();

    muduo::net::InetAddress listenAddr(g_Global.Configer().m_ServerPort, false, g_IsIPV6);
    muduo::net::EventLoop serverLoop;

    // serverLoop->runEvery()

    RouterServer rtServer(&serverLoop, listenAddr);
    rtServer.Start();

    serverLoop.loop();

    return 0;
}