#pragma once

/** 管理全局资源
 */

#include "RtConfig.h"
#include "RtUserSession.h"
#include "RtGwClientManager.h"
#include "muduo/base/ThreadPool.h"
#include "RtHst2Auth.h"
#include "RtCheckLocalRule.h"
#include "RtIdleOvertime.h"

class GlobalResource
{
public:
    RtConfig &Configer();

    RtUserSessions &UserSessions();

    muduo::ThreadPool &AskingThreadPool();

    RtGwClientManager &GwClientManager();

    muduo::net::EventLoop *EvnetLoop(ModuleGroupType type, size_t index);

    std::unique_ptr<RtHst2Auth> &Hst2Auther();

    std::unique_ptr<RtCheckLocalRule> &LocalRuler();

    EasyRouter::IdleConnOverTime &IdleOvertimer();

public:
    GlobalResource();
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
    std::map<ModuleGroupType, std::vector<muduo::net::EventLoop *>> m_DstEventloopGroup;

    std::unique_ptr<RtHst2Auth> m_Hst2Auth;
    std::unique_ptr<RtCheckLocalRule> m_LocalRuler;

    EasyRouter::IdleConnOverTime m_IdleOverTimer;
};

extern GlobalResource g_Global;