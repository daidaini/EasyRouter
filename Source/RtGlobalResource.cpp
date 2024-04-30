#include "RtGlobalResource.h"

GlobalResource g_Global;

GlobalResource::GlobalResource()
    : m_AskingThreadPool("asking_thread_pool")

{
}

void GlobalResource::Init()
{
    m_Config.LoadConfig();

    InitEventLoop();

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