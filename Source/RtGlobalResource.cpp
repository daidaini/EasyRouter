#include "RtGlobalResource.h"
#include "muduo/base/Logging.h"

GlobalResource g_Global;

GlobalResource::GlobalResource()
    : m_AskingThreadPool("AskingThread")

{
}

void GlobalResource::Init()
{
    muduo::Logger::setLogLevel(Logger::LogLevel::WARN);

    SpdLogger::Instance().Init(std::vector<LogType>{LogType::System});

    m_Config.LoadConfig();

    SpdLogger::Instance().SetLoggerLvl(SpdLogger::TransLogLevel(Configer().m_LoggerLvl.c_str()));

    InitEventLoop();

    if (m_Config.m_RouterAuthType == RouterAuthType::ThirdSysAuth)
    {
        m_Hst2Auth = std::unique_ptr<RtHst2Auth>(new RtHst2Auth);
    }
    else if (m_Config.m_RouterAuthType == RouterAuthType::LocalRule)
    {
        m_LocalRuler = std::unique_ptr<RtCheckLocalRule>(new RtCheckLocalRule);
    }

    m_AskingThreadPool.start(m_Config.m_RouterAuthThreadCnt);
}

void GlobalResource::InitEventLoop()
{
    // 创建eventloopthrea，并开启线程
    for (auto &item : m_Config.m_DstAddrGroups)
    {
        for (size_t i = 0; i < item.second.size(); ++i)
        {
            auto it = m_DstEventloopGroup.find(item.first);
            if (it == m_DstEventloopGroup.end())
            {
                it = m_DstEventloopGroup.insert({item.first, std::vector<muduo::net::EventLoop *>{}}).first;
            }

            std::unique_ptr<muduo::net::EventLoopThread> loopTh =
                std::unique_ptr<muduo::net::EventLoopThread>(new muduo::net::EventLoopThread(
                    [](EventLoop *loop)
                    {
                        long tid = ::syscall(SYS_gettid);
                        fmt::print(fg(fmt::color::sea_green), "LoopThread[{}] init sucucess..\n", tid);
                        SpdLogger::Instance().WriteLog(LogType::System, LogLevel::Warn, "LoopThread[{}] init sucucess..", tid);
                    },
                    "LoopThread"));

            muduo::net::EventLoop *loop = loopTh->startLoop();
            it->second.emplace_back(loop);
            m_EventLoopThreads.emplace_back(std::move(loopTh));
        }
    }
}

RtConfig &GlobalResource::Configer()
{
    return m_Config;
}

std::unique_ptr<RtHst2Auth> &GlobalResource::Hst2Auther()
{
    return m_Hst2Auth;
}

std::unique_ptr<RtCheckLocalRule> &GlobalResource::LocalRuler()
{
    return m_LocalRuler;
}

RtUserSessions &GlobalResource::UserSessions()
{
    return m_UserSessions;
}

muduo::ThreadPool &GlobalResource::AskingThreadPool()
{
    return m_AskingThreadPool;
}

RtGwClientManager &GlobalResource::GwClientManager()
{
    return m_GwClientManager;
}

muduo::net::EventLoop *GlobalResource::EvnetLoop(ModuleGroupType type, size_t index)
{
    auto it = m_DstEventloopGroup.find(type);
    if (it != m_DstEventloopGroup.end())
    {
        index = index % it->second.size();
        return it->second.at(index);
    }

    return nullptr;
}

EasyRouter::IdleConnOverTime &GlobalResource::IdleOvertimer()
{
    return m_IdleOverTimer;
}