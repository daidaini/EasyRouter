#pragma once

#include "RtCommDataDefine.h"

#include "T2SyncConnection.h"

/**
 * @brief 该类用以通过恒生系统获取标识来确定路由目标的模块类别
 *
 */

using AuthRspCallbackFuncType = std::function<void(GwModuleTypeEnum)>;

class RtHst2Auth
{
    const int s_AccountAuthValidSeconds = 10; // 账号认证有效时间

public:
    RtHst2Auth() = default;

    ~RtHst2Auth() = default;

    // void SetModuleTypeCb(AuthRspCallbackFuncType cb);

    // 请求获取模块类别
    void AskForModuleType(const AuthRequestParam &params, AuthRspCallbackFuncType cb);

private:
    GwModuleTypeEnum GetCachedRsp(const std::string &accountId);

    void DoAuthentication(const AuthRequestParam &params, HST2::Connection *&hsConn, AuthRspCallbackFuncType cb);

    GwModuleTypeEnum ParseModuleTypeByData(const std::string &statusStr, LoginTypeEnum loginType);

private:
    //<account, 认证时间>
    std::map<std::string, std::pair<time_t, GwModuleTypeEnum>> m_AccountWithAskingTime;

    std::mutex m_Mtx;

    // AuthRspCallbackFuncType m_RspCb;
};