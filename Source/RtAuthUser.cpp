#include "RtAuthUser.h"
#include "PoboTool.h"
#include "STD.h"
#include "RtGlobalResource.h"
#include "RtTool.h"

using namespace pobo;

RtAuthUser::RtAuthUser(const muduo::net::TcpConnectionPtr &tcpConn)
{
    m_TcpConn = tcpConn;
    m_UploadedMsg = new char[s_MaxPackgeSize]{};
    m_CachedMsgBuf = new u_char[s_MaxPackgeSize]{};
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
        auto commMsg = HDGwRouter::UnLoadPackageStatic(msg);
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
        auto reqMsg = HDGwRouter::UnLoadPackageDefault(msg, m_CommDecryptKey);
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
    char zipedmsg[s_MaxPackgeSize];
    int zipedlen = 0;
    if (!PoboPkgHandle::CompressToZip(zipedmsg, zipedlen, src, srclen))
    {
        return;
    }

    // 加密
    u_char pkgedmsg[s_MaxPackgeSize];
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
                    [this, client, loginType = params.LoginType](GwModuleTypeEnum moduleType, std::string returnMsg)
                    {
                        SpdLogger::Instance().WriteLog(LogType::System, LogLevel::Info, "Confirm module type[{}] from HST2..", GwModuleTypeToStr(moduleType));
                        if (moduleType == GwModuleTypeEnum::NONE)
                        {
                            return this->DoErrorRsp(GateErrorStruct{GateError::BIZ_ERROR, std::move(returnMsg)});
                        }
                        // 成功，开始创建练级
                        if (!client->Create(std::make_pair(moduleType, loginType)))
                        {
                            return this->DoErrorRsp(GateErrorStruct{GateError::BIZ_ERROR, "[汇点]无法成功创建到网关的连接"});
                        }

                        if (moduleType == GwModuleTypeEnum::HST2)
                        {
                            // 确认模块成功，msg返回的是"lastip lastmac lastlogindate lastlogintime"
                            client->SetLastInfo(std::move(returnMsg));
                        }

                        client->Connect();
                        client->ConfirmAuthed();
                    });
            });
    }
    else
    {
        // 回切，则需要恒生认证，直接连接恒生网关
        SpdLogger::Instance().WriteLog(LogType::System, LogLevel::Info, "Back cutted, route to HST2..");

        if (!client->Create(std::make_pair(GwModuleTypeEnum::HST2, params.LoginType)))
        {
            return this->DoErrorRsp(GateErrorStruct{GateError::BIZ_ERROR, "[汇点]无法成功创建到网关的连接"});
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
                [this, client, loginType = params.LoginType](GwModuleTypeEnum moduleType, std::string errmsg)
                {
                    SpdLogger::Instance().WriteLog(LogType::System, LogLevel::Info, "Confirm module type[{}] from local rule..", GwModuleTypeToStr(moduleType));
                    if (moduleType == GwModuleTypeEnum::NONE)
                    {
                        return this->DoErrorRsp(GateErrorStruct{GateError::BIZ_ERROR, std::move(errmsg)});
                    }
                    if (!client->Create(std::make_pair(moduleType, loginType)))
                    {
                        return this->DoErrorRsp(GateErrorStruct{GateError::BIZ_ERROR, "[汇点]无法成功创建到网关的连接"});
                    }
                    client->Connect();
                    client->ConfirmAuthed();
                });
        });
}