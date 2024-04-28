#pragma once

#include "RtAuthUser.h"
#include "RtDstClient.h"

class RouterServer
{
public:
    explicit RouterServer(muduo::net::EventLoop *loop, const muduo::net::InetAddress &addr);
    ~RouterServer();

    void Start();

private:
    void OnConnection(const muduo::net::TcpConnectionPtr &conn);

    void OnMessage(const muduo::net::TcpConnectionPtr &conn, muduo::net::Buffer *buf, muduo::Timestamp);

    void ProcessAuthMessage(const muduo::net::TcpConnectionPtr &conn, muduo::net::Buffer *buf, DstClient *client);

    void AddAuthUser(const muduo::net::TcpConnectionPtr &conn);

    RtAuthUser *GetAuthUser(TcpConnIDType connId);

private:
    std::unique_ptr<muduo::net::TcpServer> m_TcpServer;

    std::map<int, std::unique_ptr<RtAuthUser>> m_AuthUsers;
    std::mutex m_AuthMtx;
};