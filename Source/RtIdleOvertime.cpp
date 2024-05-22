#include "RtIdleOvertime.h"
#include "RtGlobalResource.h"

namespace HDGwRouter
{
    namespace
    {
        const int s_IdleOvertimeInterval = 120; // 秒
    }

    void IdleConnOverTime::Add(TcpConnIDType connId)
    {
        std::lock_guard<std::mutex> guard(m_TimeMtx);
        m_ActiveTime[connId] = time(nullptr);
    }

    void IdleConnOverTime::Erase(TcpConnIDType connId)
    {
        std::lock_guard<std::mutex> guard(m_TimeMtx);
        m_ActiveTime.erase(connId);
    }

    void IdleConnOverTime::UpdateTime(TcpConnIDType connId)
    {
        std::lock_guard<std::mutex> guard(m_TimeMtx);
        m_ActiveTime[connId] = time(nullptr);
    }

    void IdleConnOverTime::CheckOverTimer()
    {
        {
            std::lock_guard<std::mutex> guard(m_TimeMtx);
            m_TmpActiveTime = m_ActiveTime;
        }

        std::vector<TcpConnIDType> overTimeConnIds;

        time_t currTime = time(nullptr);
        for (auto &item : m_TmpActiveTime)
        {
            auto connId = item.first;
            auto lastTime = item.second;

            if (currTime - lastTime > s_IdleOvertimeInterval)
            {
                overTimeConnIds.emplace_back(connId);
            }
        }

        for (auto connId : overTimeConnIds)
        {
            auto tcpConn = g_Global.UserSessions().GetTcpConn(connId);
            SpdLogger::Instance().WriteLog(LogType::System, LogLevel::Warn, "用户[{}]闲置时间超时，断开连接", tcpConn->name());
            tcpConn->forceClose();
        }
    }
}