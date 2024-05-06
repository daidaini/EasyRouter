#include "RouterServer.h"

#include "RtGlobalResource.h"

bool g_IsIPV6 = false;

void MainOnTimer()
{
    int nowDate, nowTime = 0;
    STD::GetDateTime(nowDate, nowTime);

    if (nowDate != SpdLogger::Instance().CurrentDate())
    {
        SpdLogger::Instance().Init(std::vector<LogType>{LogType::System});
    }
}

int main(int argc, char *argv[])
{
    g_Global.Init();

    muduo::net::InetAddress listenAddr(g_Global.Configer().m_ServerPort, false, g_IsIPV6);
    muduo::net::EventLoop serverLoop;

    serverLoop.runEvery(10.0, MainOnTimer); // 定时10秒

    RouterServer rtServer(&serverLoop, listenAddr);
    rtServer.Start();

    serverLoop.loop();

    return 0;
}