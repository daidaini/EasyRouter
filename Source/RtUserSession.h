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

    void UpdateSession(const muduo::net::TcpConnectionPtr &tcpConn)
    {
        TcpConnIDType connId = tcpConn->getConnId();
        std::lock_guard<std::mutex> guard(m_ConnMtx);
        auto it = m_ConnMapper.find(connId);
        if (it != m_ConnMapper.end())
        {
            // 删除用户
            // to do .notice?
            m_ConnMapper.erase(it);
            return;
        }

        m_ConnMapper.emplace(connId, tcpConn);
        std::string logstr = fmt::format("用户[{}]连接成功, 当前总用户数 = {}", connId, m_ConnMapper.size());
        fmt::print(fg(fmt::color::sea_green), "{}\n", logstr);
        SpdLogger::Instance().WriteLog(LogType::System, LogLevel::Warn, std::move(logstr));
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