#pragma once

#include "RtCommDataDefine.h"

#include "T2SyncConnection.h"

/**
 * @brief 该类用以通过恒生系统获取标识来确定路由目标的模块类别
 *
 */

class RtHst2Auth
{
    const int s_AccountAuthValidSeconds = 10; // 账号认证有效时间

public:
    RtHst2Auth() = default;

    ~RtHst2Auth() = default;

    // 请求获取模块类别
    void AskForModuleType(const AuthRequestParam &params, RtDstCallbackFuncType cb);

private:
    GwModuleTypeEnum GetCachedRsp(const std::string &accountId);

    void AddCachedRsp(const std::string &accountId, GwModuleTypeEnum type);

    void DoAuthentication(const AuthRequestParam &params, HST2::Connection *&hsConn, RtDstCallbackFuncType cb);

    GwModuleTypeEnum ParseModuleTypeByData(const std::string &statusStr, LoginTypeEnum loginType);

    std::pair<std::string, std::string> ParseLastLoginInfo(const std::string &last_op_station);

private:
    //<account, <认证时间, dsttype>
    std::map<std::string, std::pair<time_t, GwModuleTypeEnum>> m_AccountWithAskingTime;

    std::mutex m_Mtx;
};