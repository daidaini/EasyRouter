#include "RtAuthUser.h"
#include "PoboTool.h"
#include "STD.h"
#include "RtGlobalResource.h"

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

int RtAuthUser::ProcessMsg(const pobo::CommMessage &msg, DstClient *client)
{
    auto type = PoboPkgHandle::CheckPackage(msg.Head);
    if (type == RawPackageType::Unknown || !CheckPkgAccurate(type))
    {
        SpdLogger::Instance().WriteLog(LogType::System, LogLevel::Error, "Unnknown package type[{}] ..", (int)msg.Head.Sign);
        return -1;
    }

    u_short checkcode = pobo::GetCheckCode(msg.Body.data(), msg.Head.PackageSize);
    if (checkcode != msg.Head.CheckCode)
    {
        return -1;
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
            SwapCommunicationKey();
            return 1;
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
            return 3;
        }
        else if (m_ReqStep.FunctionId() == 6011)
        {
            ProcessLoginRequest(client);
            return 6011;
        }
    }

    return 0;
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
    char txmy[33]{};
    m_ReqStep.GetFieldValueString(STEP_TXMY, txmy, sizeof(txmy));
    AES_set_encrypt_key((u_char *)txmy, 256, &m_CommEncryptKey);

    char randkey[128]{};
    STD::GenerateDesKey((u_char *)randkey, sizeof(randkey), 32);
    m_Commkey = randkey;

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

    SendMsgBack(m_RspStep.ToString());

    ::memset(&m_CommEncryptKey, 0, sizeof(m_CommEncryptKey));
    ::memset(&m_CommDecryptKey, 0, sizeof(m_CommDecryptKey));
    AES_set_encrypt_key((u_char *)randkey, 256, &m_CommEncryptKey); // 更新
    AES_set_decrypt_key((u_char *)randkey, 256, &m_CommDecryptKey);

    m_CurrAuthStatus = AuthStatus::ConfirmCommKey;
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
    m_CurrAuthStatus = AuthStatus::ConrirmPwdKey;

    SendMsgBack(m_RspStep.ToString());
}

void RtAuthUser::DoErrorRsp(GateErrorStruct err)
{
    m_RspStep.Init();

    m_RspStep.SetBaseFieldValueInt(STEP_CODE, static_cast<int>(err.ErrCode));
    m_RspStep.SetBaseFieldValueString(STEP_MSG, err.ErrMsg);
    m_RspStep.SetBaseFieldValueInt(STEP_FUNC, m_ReqStep.FunctionId());
    m_RspStep.SetBaseFieldValueInt(STEP_REQUESTNO, m_ReqStep.Requestno());
    m_RspStep.SetBaseFieldValueInt(STEP_RETURNNUM, 1);
    m_RspStep.SetBaseFieldValueInt(STEP_TOTALNUM, 1);
    m_RspStep.SetBaseFieldValueInt(STEP_SESSION, m_TcpConn != nullptr ? m_TcpConn->getConnId() : 0);

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

    if (m_TcpConn->connected())
    {
        m_TcpConn->send(pkgedmsg, len);
    }
}

void RtAuthUser::ProcessLoginRequest(DstClient *client)
{
    AES_KEY decodeKey{};
    AES_set_decrypt_key((u_char *)m_Pwdkey.data(), 256, &decodeKey);

    AuthRequestParam params;

    params.Password = m_ReqStep.GetBaseFieldValueString_Encrypted(STEP_JYMM, decodeKey);
    params.AccountId = m_ReqStep.GetStepValueByID(STEP_DLZH);
    params.LoginType = m_ReqStep.GetLoginType();
    params.AccountType = static_cast<AccountTypeEnum>(m_ReqStep.GetFieldValueChar(STEP_ZHLB));

    if (params.AccountId.empty() || params.Password.empty() || params.LoginType == LoginTypeEnum::None)
    {
        return DoErrorRsp(GateErrorStruct{GateError::BIZ_ERROR, "登录请求入参有误，请检查"});
    }

    this->m_CurrAuthStatus = AuthStatus::ConfirmLogin;

    if (g_Global.Configer().m_RouterAuthType == RouterAuthType::ThirdSysAuth)
    {
        AskingRouterFlagTh(params, client);
    }
    else if (g_Global.Configer().m_RouterAuthType == RouterAuthType::LocalRule)
    {
        CheckLocalRuleTh(params, client);
    }
}

const std::string &RtAuthUser::GetCommKey() const
{
    return m_Commkey;
}

const std::string &RtAuthUser::GetPwdKey() const
{
    return m_Pwdkey;
}

void RtAuthUser::AskingRouterFlagTh(const AuthRequestParam &params, DstClient *client)
{
    if (!g_Global.Configer().m_IsNeedBackcut)
    {
        g_Global.AskingThreadPool().run(
            [params, client, this]()
            {
                g_Global.Hst2Auther()->AskForModuleType(
                    params,
                    [this, client, loginType = params.LoginType](GwModuleTypeEnum moduleType)
                    {
                        SpdLogger::Instance().WriteLog(LogType::System, LogLevel::Info, "Confirm module type[{}] from hst2..", GwModuleTypeToStr(moduleType));
                        if (moduleType == GwModuleTypeEnum::NONE)
                        {
                            return this->DoErrorRsp(GateErrorStruct{GateError::BIZ_ERROR, "获取目标网关失败"});
                        }
                        if (!client->Create(std::make_pair(moduleType, loginType)))
                        {
                            return this->DoErrorRsp(GateErrorStruct{GateError::BIZ_ERROR, "无法成功创建到网关的连接"});
                        }
                        client->Connect();
                        client->ConfirmAuthed();
                    });
            });
    }
    else
    {
        // 回切，则需要恒生认证，直接连接恒生网关
        SpdLogger::Instance().WriteLog(LogType::System, LogLevel::Info, "Back cutted, route to hst2..");

        if (!client->Create(std::make_pair(GwModuleTypeEnum::HST2, params.LoginType)))
        {
            return this->DoErrorRsp(GateErrorStruct{GateError::BIZ_ERROR, "无法成功创建到网关的连接"});
        }
        client->Connect();
        client->ConfirmAuthed();
    }
}

void RtAuthUser::CheckLocalRuleTh(const AuthRequestParam &params, DstClient *client)
{
    g_Global.AskingThreadPool().run(
        [params, client, this]()
        {
            g_Global.LocalRuler()->CheckRtByAccount(
                params.AccountId,
                [this, client, loginType = params.LoginType](GwModuleTypeEnum moduleType)
                {
                    SpdLogger::Instance().WriteLog(LogType::System, LogLevel::Info, "Confirm module type[{}] from local rule..", GwModuleTypeToStr(moduleType));
                    if (moduleType == GwModuleTypeEnum::NONE)
                    {
                        return this->DoErrorRsp(GateErrorStruct{GateError::BIZ_ERROR, "获取目标网关失败"});
                    }
                    if (!client->Create(std::make_pair(moduleType, loginType)))
                    {
                        return this->DoErrorRsp(GateErrorStruct{GateError::BIZ_ERROR, "无法成功创建到网关的连接"});
                    }
                    client->Connect();
                    client->ConfirmAuthed();
                });
        });
}