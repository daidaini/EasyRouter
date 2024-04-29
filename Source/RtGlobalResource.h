#pragma once

/** 管理全局资源
 */

#include "RtConfig.h"
#include "RtUserSession.h"
#include "RtGwClientManager.h"
#include "muduo/base/ThreadPool.h"

class GlobalResource
{
public:
    RtConfig &Configer();

    RtUserSessions &UserSessions();

    muduo::ThreadPool &AskingThreadPool();

    RtGwClientManager &GwClientManager();

    muduo::net::EventLoop *EvnetLoop(GwModuleTypeEnum type, size_t index);

public:
    GlobalResource() = default;
    ~GlobalResource() = default;

    // 必须第一步使用
    void Init();

private:
    void InitEventLoop();

private:
    RtConfig m_Config;

    RtUserSessions m_UserSessions;

    muduo::ThreadPool m_AskingThreadPool;

    RtGwClientManager m_GwClientManager;

    std::vector<std::unique_ptr<muduo::net::EventLoopThread>> m_EventLoopThreads;
    std::map<GwModuleTypeEnum, std::vector<muduo::net::EventLoop *>> m_DstEventloopGroup;
};

extern GlobalResource g_Global;