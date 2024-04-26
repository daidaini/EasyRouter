#pragma once

#include "RtCommDataDefine.h"

using namespace muduo::net;
using namespace muduo;

class DstClient final
{
public:
    DstClient(EventLoop *loop, const InetAddress *serverAddr, std::string name, int srcConnId);
    ~DstClient() = default;

    void
    Start();

    void OnConnect(const TcpConnectionPtr &tcpConn);
    void OnResponse(const TcpConnectionPtr &conn, Buffer *buf, muduo::Timestamp time);

    void SendMsg(std::string msg);
    void SendMsg(muduo::net::Buffer *buff);

    void Close();

private:
    TcpClient m_Client;

    std::string m_Name;

    // 客户端的连接标识
    TcpConnIDType m_SrcConnId{-1};
};