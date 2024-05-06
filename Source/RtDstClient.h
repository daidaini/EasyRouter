#pragma once

#include "RtCommDataDefine.h"

using namespace muduo::net;
using namespace muduo;

class DstClient final
{
public:
    explicit DstClient(int srcConnId);
    ~DstClient() = default;

    void Create(GwModuleTypeEnum moduleType);

    void Connect();

    void OnConnect(const TcpConnectionPtr &tcpConn);
    void OnResponse(const TcpConnectionPtr &conn, Buffer *buf, muduo::Timestamp time);

    // 设置通讯密钥
    void SetCommKey(std::string commkey);
    // 设置密码密钥
    void SetPwdKey(std::string pwdkey);
    // 设置卸载前的登录请求
    void SetLoginRequest(std::string loginReq);

    void ConfirmAuthed();
    bool IsValid() const;

    void SendMsg(const std::string &msg);
    void SendMsg(muduo::net::Buffer *buff);

    void Close();

private:
    void PushKeys();

private:
    std::unique_ptr<TcpClient> m_TcpClient;

    std::atomic_bool m_IsConnected{false};

    std::string m_Name;

    // 客户端的连接标识
    TcpConnIDType m_SrcConnId{-1};

    std::string m_Pwdkey;
    std::string m_Commkey;

    std::string m_EncrypedLoginReq;

    bool m_IsAuthed{false};
};