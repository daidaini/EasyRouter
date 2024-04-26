#pragma once

#include "RtCommDataDefine.h"

class RouterServer
{
public:
    explicit RouterServer(muduo::net::EventLoop *loop, const muduo::net::InetAddress &addr);
    ~RouterServer();

    void Start();

private:
    void OnConnection(const muduo::net::TcpConnectionPtr &conn);

    void OnMessage(const muduo::net::TcpConnectionPtr &conn, muduo::net::Buffer *buf, muduo::Timestamp);

private:
    std::unique_ptr<muduo::net::TcpServer> m_TcpServer;
};