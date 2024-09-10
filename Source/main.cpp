#include "RouterServer.h"

#include "RtGlobalResource.h"

#include "args.hxx"
#include "RtVersion.h"

bool g_IsIPV6 = false;

// 返回true表示程序需要继续运行，false则结束
bool ParseArgs(int argCnt, char *argVal[])
{
    if (argCnt < 2)
    {
        return true;
    }

    args::ArgumentParser parser("This is a router program to trans data to different HuiDian's gateways.",
                                "Use command \'./EasyRouter -h\' to know the usage.");

    args::HelpFlag help(parser, "help", "Show usage", {'h', "help"});
    args::ValueFlag<int> serverPort(parser, "", "Set port for server", {'p', "port"});
    args::ValueFlag<std::string> ipPatternOption(parser, "", "Set if use ipv6 or ipv4", {"ip"});
    args::ValueFlag<int> thSizeOption(parser, "", "Set the size of server's thread pool", {'t', "th_size"});
    args::Flag version(parser, "version", "show version info", {'v', "version"});

    try
    {
        parser.ParseCLI(argCnt, argVal);
    }
    catch (args::Help)
    {
        std::cout << parser;
        return false;
    }
    catch (args::ParseError e)
    {
        std::cerr << e.what() << std::endl;
        std::cerr << parser;
        return false;
    }
    catch (args::ValidationError e)
    {
        std::cerr << e.what() << std::endl;
        std::cerr << parser;
        return false;
    }

    if (version)
    {
        fmt::print(fg(fmt::color::green), "Version : {}\n", EasyRouter::VersionInfo);
        return false;
    }

    if (serverPort)
    {
        g_Global.Configer().m_ServerPort = args::get(serverPort);
    }

    if (ipPatternOption)
    {
        g_IsIPV6 = (args::get(ipPatternOption) == "ipv6");
    }

    if (thSizeOption)
    {
        g_Global.Configer().m_ServerThreadCnt = args::get(thSizeOption);
    }

    return true;
}

void MainOnTimer()
{
    int nowDate, nowTime = 0;
    STD::GetDateTime(nowDate, nowTime);

    if (nowDate != SpdLogger::Instance().CurrentDate())
    {
        // 日志重置
        SpdLogger::Instance().Init(std::vector<LogType>{LogType::System});

        SpdLogger::Instance().SetLoggerLvl(SpdLogger::TransLogLevel(g_Global.Configer().m_LoggerLvl.c_str()));
    }

    g_Global.Configer().CheckBackCutFileOnTimer();

    g_Global.IdleOvertimer().CheckOverTimer();
}

int main(int argc, char *argv[])
{
    if (!ParseArgs(argc, argv))
    {
        exit(EXIT_SUCCESS);
    }

    g_Global.Init();

    SpdLogger::Instance().WriteLog(LogType::System, LogLevel::Warn, "Global resouces init finished.");

    muduo::net::InetAddress listenAddr(g_Global.Configer().m_ServerPort, false, g_IsIPV6);
    muduo::net::EventLoop serverLoop;

    RouterServer rtServer(&serverLoop, listenAddr);
    rtServer.Start();

    serverLoop.runEvery(30.0, MainOnTimer); // 定时30秒

    SpdLogger::Instance().WriteLog(LogType::System, LogLevel::Warn, "Server started, begin to loop..");

    serverLoop.loop();

    exit(EXIT_SUCCESS);
}