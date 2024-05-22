#pragma once
/**
 * @brief 管理客户端用户的连接
 *
 */

#include "RtCommDataDefine.h"

class RtUserSessions final
{
public:
    RtUserSessions() = default;
    ~RtUserSessions() = default;

    void AddSession(const muduo::net::TcpConnectionPtr &tcpConn)
    {
        TcpConnIDType connId = tcpConn->getConnId();
        std::unique_lock<std::mutex> lock(m_ConnMtx);

        m_ConnMapper.emplace(connId, tcpConn);
        std::string logstr = fmt::format("用户[{}]连接成功, 当前总用户数 = {}", connId, m_ConnMapper.size());

        lock.unlock();
        fmt::print(fg(fmt::color::sea_green), "{}\n", logstr);
        SpdLogger::Instance().WriteLog(LogType::System, LogLevel::Info, logstr);

        g_Global.IdleOvertimer().Add(connId);
    }

    void EraseSession(TcpConnIDType connId)
    {
        std::unique_lock<std::mutex> lock(m_ConnMtx);

        int userCnt = m_ConnMapper.size();
        auto it = m_ConnMapper.find(connId);
        if (it != m_ConnMapper.end())
        {
            // 删除用户
            m_ConnMapper.erase(it);

            userCnt = m_ConnMapper.size();
            lock.unlock();
        }

        std::string logstr = fmt::format("用户[{}]断开连接成功, 当前总用户数 = {}", connId, userCnt);
        fmt::print(fg(fmt::color::orange_red), "{}\n", logstr);
        SpdLogger::Instance().WriteLog(LogType::System, LogLevel::Info, std::move(logstr));

        g_Global.IdleOvertimer().Erase(connId);
    }

    muduo::net::TcpConnectionPtr GetTcpConn(TcpConnIDType connId)
    {
        std::lock_guard<std::mutex> guard(m_ConnMtx);

        auto it = m_ConnMapper.find(connId);
        if (it == m_ConnMapper.end())
        {
            return nullptr;
        }

        return it->second;
    }

private:
    std::unordered_map<TcpConnIDType, muduo::net::TcpConnectionPtr> m_ConnMapper;

    std::mutex m_ConnMtx;
};