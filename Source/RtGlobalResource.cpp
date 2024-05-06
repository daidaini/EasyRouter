#include "RtGlobalResource.h"

GlobalResource g_Global;

GlobalResource::GlobalResource()
    : m_AskingThreadPool("asking_thread_pool")

{
}

void GlobalResource::Init()
{
    SpdLogger::Instance().Init(std::vector<LogType>{LogType::System});

    m_Config.LoadConfig();

    InitEventLoop();

    if (m_Config.m_RouterAuthType == RouterAuthType::ThirdSysAuth)
    {
        m_Hst2Auth = std::unique_ptr<RtHst2Auth>(new RtHst2Auth);
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
                        fmt::print("thread[{}] init sucucess..\n", ::syscall(SYS_gettid));
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

muduo::net::EventLoop *GlobalResource::EvnetLoop(GwModuleTypeEnum type, size_t index)
{
    auto it = m_DstEventloopGroup.find(type);
    if (it != m_DstEventloopGroup.end())
    {
        index = index % it->second.size();
        return it->second.at(index);
    }

    return nullptr;
}