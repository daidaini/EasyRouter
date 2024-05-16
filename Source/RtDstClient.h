#pragma once

#include "RtCommDataDefine.h"

using namespace muduo::net;
using namespace muduo;

class DstClient final
{
public:
    explicit DstClient(int srcConnId);
    ~DstClient() = default;

    bool Create(ModuleGroupType type);

    void Connect();

    void OnConnect(const TcpConnectionPtr &tcpConn);
    void OnResponse(const TcpConnectionPtr &conn, Buffer *buf, muduo::Timestamp time);

    // 设置源对端Ip
    void SetSrcConnPeerIp(std::string peerIp);
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
    void SetLastInfo(std::string lastInfo);
    void UpdateLoginRspAndSendBack(Buffer *buff, const muduo::net::TcpConnectionPtr &srcConn);

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
    std::string m_SrcPeerIp;

    bool m_IsAuthed{false};

    std::string m_LastIp;
    std::string m_LastMac;

    bool m_IsLoginRsped{false};
};