#include "RtAuthUser.h"
#include "PoboTool.h"
#include "STD.h"

using namespace pobo;

RtAuthUser::RtAuthUser(const muduo::net::TcpConnectionPtr &tcpConn)
{
    m_TcpConn = tcpConn;
    m_UploadedMsg = new char[MAX_STEP_PACKAGE_BUFFER_SIZE]{};
    m_CachedMsgBuf = new u_char[MAX_STEP_PACKAGE_BUFFER_SIZE]{};
}

RtAuthUser::~RtAuthUser()
{
    delete m_UploadedMsg;
    delete m_CachedMsgBuf;
}

void RtAuthUser::ProcessMsg(const pobo::CommMessage &msg)
{
    auto type = PoboPkgHandle::CheckPackage(msg.Head);
    if (type == RawPackageType::Unknown)
    {
        // to do. // 此处可以埋点检测下未知的包的多少，可以考虑对连接的IP做管理
        SpdLogger::Instance().WriteLog(LogType::System, LogLevel::Error, "Unnknown package type[{}] ..", (int)msg.Head.Sign);
        return;
    }

    if (!CheckPkgAccurate(type))
    {
        // log
        return;
    }

    u_short checkcode = pobo::GetCheckCode(msg.Body.data(), msg.Head.PackageSize);
    if (checkcode != msg.Head.CheckCode)
    {
        return;
    }

    if (type == RawPackageType::Static)
    {
        auto commMsg = UnLoadPackageStatic(msg);
        if (!commMsg.empty())
        {
            m_ReqStep.SetPackage(commMsg);
        }
        if (m_ReqStep.FunctionId() == 1)
        {
            return SwapCommunicationKey();
        }
    }
    else
    {
        auto reqMsg = UnLoadPackageDefault(msg);
        if (!reqMsg.empty())
        {
            m_ReqStep.SetPackage(reqMsg);
        }
        if (m_ReqStep.FunctionId() == 3)
        {
            SwapPasswordKey();
        }
        else if (m_ReqStep.FunctionId() == 3)
        {
        }
    }
}

bool RtAuthUser::CheckPkgAccurate(pobo::RawPackageType type) const
{
    if (m_CurrAuthStatus == AuthStatus::None && type != RawPackageType::Static)
    {
        return false;
    }

    if (m_CurrAuthStatus != AuthStatus::None && type != RawPackageType::Dynamic)
    {
        return false;
    }

    return true;
}

std::string RtAuthUser::UnLoadPackageStatic(const pobo::CommMessage &msg)
{
    u_char tmpBuf[MAX_STEP_PACKAGE_BUFFER_SIZE]{};
    int tmpSize = DecrpytStaticKey((char *)tmpBuf, msg);
    if (tmpSize < 0)
    {
        return "";
    }

    u_long msglen = MAX_STEP_PACKAGE_BUFFER_SIZE;
    auto result = PoboPkgHandle::UncompressFromZip(m_UploadedMsg, msglen, tmpBuf, tmpSize - sizeof(PB_ZipHead));
    if (!result.first)
    {
        return "";
    }
    // 2位位移，前两位是校验码
    return std::string(m_UploadedMsg + 2, msglen - 2);
}

void RtAuthUser::SwapCommunicationKey()
{
    if (m_ReqStep.FunctionId() != 1)
    {
        // log error
        return;
    }

    char txmy[33]{};
    m_ReqStep.GetFieldValueString(STEP_TXMY, txmy, sizeof(txmy));
    AES_set_encrypt_key((u_char *)txmy, 256, &m_CommEncryptKey);

    char randkey[128]{};
    STD::GenerateDesKey((u_char *)randkey, sizeof(randkey), 32);
    m_RspStep.SetBaseFieldValueInt(STEP_CODE, 0);
    m_RspStep.SetBaseFieldValueString(STEP_MSG, "");
    m_RspStep.SetBaseFieldValueInt(STEP_FUNC, 1);
    m_RspStep.SetBaseFieldValueInt(STEP_REQUESTNO, m_ReqStep.Requestno());
    m_RspStep.SetBaseFieldValueInt(STEP_SESSION, m_TcpConn->getConnId());
    m_RspStep.SetBaseFieldValueInt(STEP_RETURNNUM, 1);
    m_RspStep.SetBaseFieldValueInt(STEP_TOTALNUM, 1);

    m_RspStep.AppendRecord();
    m_RspStep.AddFieldValue(STEP_TXMY, randkey);

    m_RspStep.EndAppendRecord();

    auto rspBuf = m_RspStep.ToString();

    char zipedmsg[MAX_STEP_PACKAGE_BUFFER_SIZE];
    int zipedlen = 0;
    if (!PoboPkgHandle::CompressToZip(zipedmsg, zipedlen, rspBuf.data(), rspBuf.size(), false))
    {
        return;
    }

    u_char pkgedmsg[MAX_STEP_PACKAGE_BUFFER_SIZE];
    int len = PoboPkgHandle::EncryptPackage(pkgedmsg, (u_char *)zipedmsg, (u_long)zipedlen, m_CommEncryptKey);

    if (m_TcpConn->connected())
    {
        m_TcpConn->send(pkgedmsg, len);
    }

    ::memset(&m_CommEncryptKey, 0, sizeof(m_CommEncryptKey));
    ::memset(&m_CommDecryptKey, 0, sizeof(m_CommDecryptKey));
    AES_set_encrypt_key((u_char *)randkey, 256, &m_CommEncryptKey); // 更新
    AES_set_decrypt_key((u_char *)randkey, 256, &m_CommDecryptKey);

    m_Commkey = randkey;
}

int RtAuthUser::DecrpytStaticKey(char *outBuf, const pobo::CommMessage &msg)
{
    const auto &prikeys = GetPriKeys();

    if (prikeys.empty())
    {
        return PoboPkgHandle::DecryptPackgeRSA((u_char *)outBuf, (const unsigned char *)msg.Body.data(), msg.Head.PackageSize);
    }

    for (const u_char *prikey : prikeys)
    {
        int ret = PoboPkgHandle::DecryptPackgeRSA((u_char *)outBuf, (const unsigned char *)msg.Body.data(), msg.Head.PackageSize, prikey);
        if (ret >= 0)
        {
            return ret;
        }

        SpdLogger::Instance().WriteLog(LogType::System, LogLevel::Debug, "DecryptPackgeRSA this key failed[{}], continue..", ret);
        continue;
    }

    SpdLogger::Instance().WriteLog(LogType::System, LogLevel::Error, "DecryptPackgeRSA all private keys failed.");
    return -1;
}

std::string RtAuthUser::UnLoadPackageDefault(const pobo::CommMessage &msg)
{
    PoboPkgHandle::DecryptPackage(m_CachedMsgBuf, (const unsigned char *)msg.Body.data(), msg.Head.PackageSize, m_CommDecryptKey);

    u_long msglen = MAX_STEP_PACKAGE_BUFFER_SIZE;
    auto result = PoboPkgHandle::UncompressFromZip(m_UploadedMsg, msglen, m_CachedMsgBuf, strlen((const char *)m_CachedMsgBuf + sizeof(PB_ZipHead)));
    if (!result.first)
    {
        SpdLogger::Instance().WriteLog(LogType::System, LogLevel::Warn, "UnLoadPackageDefault failed[{}]", result.second);
        return {};
    }

    return std::string(m_UploadedMsg + 2, msglen - 2);
}

void RtAuthUser::SwapPasswordKey()
{
    char randkey[128]{};
    STD::GenerateDesKey((u_char *)randkey, sizeof(randkey), 32);
    // AES_set_decrypt_key((u_char *)randkey, 256, &m_PwdDecodeKey);
    // AES_set_encrypt_key((u_char *)randkey, 256, &m_PwdEncodeKey);

    m_RspStep.Init();

    m_RspStep.SetBaseFieldValueInt(STEP_CODE, 0);
    m_RspStep.SetBaseFieldValueString(STEP_MSG, "");
    m_RspStep.SetBaseFieldValueInt(STEP_FUNC, 3); // 功能
    m_RspStep.SetBaseFieldValueInt(STEP_REQUESTNO, m_ReqStep.Requestno());
    m_RspStep.SetBaseFieldValueInt(STEP_SESSION, m_TcpConn->getConnId());
    m_RspStep.SetBaseFieldValueInt(STEP_RETURNNUM, 1);
    m_RspStep.SetBaseFieldValueInt(STEP_TOTALNUM, 1);

    m_RspStep.AppendRecord();
    m_RspStep.AddFieldValue(STEP_TXMY, randkey);
    m_RspStep.EndAppendRecord();

    m_Pwdkey = randkey;

    SendMsgBack(m_RspStep.ToString());
}

void RtAuthUser::SendMsgBack(const std::string &rsp)
{
    const char *src = rsp.data();
    int srclen = rsp.size();

    // 压缩
    char zipedmsg[MAX_STEP_PACKAGE_BUFFER_SIZE];
    int zipedlen = 0;
    if (!PoboPkgHandle::CompressToZip(zipedmsg, zipedlen, src, srclen))
    {
        return;
    }

    // 加密
    u_char pkgedmsg[MAX_STEP_PACKAGE_BUFFER_SIZE];
    int len = PoboPkgHandle::EncryptPackage(pkgedmsg, (u_char *)zipedmsg, zipedlen, m_CommEncryptKey);

    PB_FrameHead *head = (PB_FrameHead *)pkgedmsg;
    head->PackageNo = 0;
    head->PackageNum = 1;

    if (m_TcpConn->connected())
    {
        m_TcpConn->send(pkgedmsg, len);
    }
}