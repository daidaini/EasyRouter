#pragma once

#include "RtCommDataDefine.h"

#include "PbPkgHandle.h"

#include "CachedGatePBStep.h"

#include "RtDstClient.h"

class RtAuthUser
{
    enum AuthStatus
    {
        // 初始
        None,
        // 交换密钥
        ConfirmCommKey,
        // 交换密码密钥
        ConrirmPwdKey,
        // 登录请求
        ConfirmLogin,
    };

public:
    explicit RtAuthUser(const muduo::net::TcpConnectionPtr &tcpConn);
    ~RtAuthUser();

    // 返回功能号 (小于0为失败)
    int ProcessMsg(const pobo::CommMessage &msg, DstClient *client);

    const std::string &GetCommKey() const;

    const std::string &GetPwdKey() const;

    void AskingRouterFlagTh(const AuthRequestParam &params, DstClient *client);
    void CheckLocalRuleTh(const AuthRequestParam &params, DstClient *client);

private:
    bool CheckPkgAccurate(pobo::RawPackageType type) const;

    void SwapCommunicationKey();

    void SwapPasswordKey();
    void ProcessLoginRequest(DstClient *client);

    void DoErrorRsp(GateErrorStruct err);
    void SendMsgBack(const std::string &rsp);

private:
    muduo::net::TcpConnectionPtr m_TcpConn;

    char *m_UploadedMsg = nullptr;
    u_char *m_CachedMsgBuf = nullptr;

    AuthStatus m_CurrAuthStatus{AuthStatus::None};

    std::string m_Pwdkey;
    std::string m_Commkey;
    std::string m_EncrypedLoginReq;

    GatePBStep m_ReqStep;
    GatePBStep m_RspStep;

    AES_KEY m_CommEncryptKey{};
    AES_KEY m_CommDecryptKey{};
};