#include "RtGlobalResource.h"

GlobalResource g_Global;

void GlobalResource::Init()
{
    m_Config.LoadConfig();

    InitEventLoop();
}

void GlobalResource::InitEventLoop()
{
    // 创建eventloopthrea，并开启线程
    for (auto &item : m_Config.m_DstAddrGroups)
    {
        auto it = m_DstEventloopGroup.find(item.first);
        if (it == m_DstEventloopGroup.end())
        {
            it = m_DstEventloopGroup.insert({item.first, std::vector<muduo::net::EventLoop *>{}}).first;
        }

        std::unique_ptr<muduo::net::EventLoopThread> loopTh = std::unique_ptr<muduo::net::EventLoopThread>(new muduo::net::EventLoopThread);

        muduo::net::EventLoop *loop = loopTh->startLoop();

        it->second.emplace_back(loop);

        m_EventLoopThreads.emplace_back(std::move(loopTh));
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
    return m_DstEventloopGroup[type].at(index);
}