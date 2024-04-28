#pragma once

#include "RtCommDataDefine.h"

#include "PbPkgHandle.h"

#include "CachedGatePBStep.h"

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

    void ProcessMsg(const pobo::CommMessage &msg);

private:
    bool CheckPkgAccurate(pobo::RawPackageType type) const;

    std::string UnLoadPackageStatic(const pobo::CommMessage &msg);

    int DecrpytStaticKey(char *outBuf, const pobo::CommMessage &msg);

    std::string UnLoadPackageDefault(const pobo::CommMessage &msg);

    void SwapCommunicationKey();

    void SwapPasswordKey();

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