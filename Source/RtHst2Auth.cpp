#include "RtHst2Auth.h"
#include "RtGlobalResource.h"
#include "StringFunc.h"
#include "STD.h"
#include "PoboTool.h"

namespace HST2
{
    CConnPool<Connection> g_HsConnPool;
}

using namespace HST2;

void RtHst2Auth::AskForModuleType(const AuthRequestParam &params, RtDstCallbackFuncType cb)
{
    try
    {
        GwModuleTypeEnum moduleType = GetCachedRsp(params.AccountId);
        if (moduleType != GwModuleTypeEnum::NONE)
        {
            return cb(moduleType, "");
        }

        Connection *hsConn = HST2::g_HsConnPool.GetConnectWithCheckConnectFlag();
        if (hsConn == nullptr)
        {
            SpdLogger::Instance().WriteLog(LogType::System, LogLevel::Warn, "Get HST2 connection failed..");
            return cb(GwModuleTypeEnum::NONE, "连接恒生认证系统失败");
        }

        DoAuthentication(params, hsConn, std::move(cb));

        if (hsConn != nullptr)
        {
            HST2::g_HsConnPool.PutConnect(hsConn); // 返还连接
        }
    }
    catch (const std::exception &e)
    {
        SpdLogger::Instance().WriteLog(LogType::System, LogLevel::Error, "RtHst2Auth::AskForModuleType happend exception, [{}]", e.what());
    }
}

void RtHst2Auth::DoAuthentication(const AuthRequestParam &params, Connection *&hsConn, RtDstCallbackFuncType cb)
{
    hsConn->PrepareRequest(331100, "客户登录", 0);

    hsConn->MID_AddString("op_branch_no", "");
    hsConn->MID_AddString("op_entrust_way", g_Global.Configer().m_RouterAuthParams["op_entrust_way"]);
    hsConn->MID_AddString("op_station", "RouterServer:Auth");
    hsConn->MID_AddString("branch_no", "");
    hsConn->MID_AddChar("password_type", '2'); // 1表示资金密码，2表示交易密码 3表示查询密码
    hsConn->MID_AddString("password", params.Password);

    hsConn->MID_AddString("account_content", params.AccountId);
    hsConn->MID_AddString("content_type", "0");

    if (params.AccountType == AccountTypeEnum::AccountID)
    {
        hsConn->MID_AddChar("asset_prop", 'B'); // B-衍生品账户
        hsConn->MID_AddChar("input_content", '1');
    }
    else
    {
        hsConn->MID_AddChar("input_content", '6'); // 1-资金账号 6-客户号
    }

    auto result = hsConn->MID_SendToData();
    if (::Failed(result.ErrCode))
    {
        std::string errmsg = fmt::format("[{}]{}", result.ErrCode, result.ErrMsg);
        SpdLogger::Instance().WriteLog(LogType::System, LogLevel::Warn, "Do HST2 request failed.. {}\n", errmsg);
        if (result.ErrCode == GateError::NET_ERROR)
        {
            HST2::g_HsConnPool.ReleaseConnect(hsConn);
            hsConn = nullptr;
            return cb(GwModuleTypeEnum::NONE, std::move(errmsg));
        }

        return cb(GwModuleTypeEnum::NONE, g_PASSWORD_ERRORMSG_TIPS);
    }

    if (!hsConn->MID_MoveNextRec())
    {
        SpdLogger::Instance().WriteLog(LogType::System, LogLevel::Info, "Get HST2 empty record");
        return cb(GwModuleTypeEnum::NONE, "获取认证返回为空");
    }

    std::string statusStr = hsConn->MID_GetString("asset_prop_status_str");

    std::string last_op_station = hsConn->MID_GetString("last_op_station");
    auto lastIpMac = ParseLastLoginInfo(last_op_station);

    GwModuleTypeEnum moduleType = ParseModuleTypeByData(statusStr, params.LoginType);

    // 返回标识
    if (moduleType != GwModuleTypeEnum::NONE)
    {
        cb(moduleType, std::move(lastIpMac));
        AddCachedRsp(params.AccountId, moduleType);
    }
    else
    {
        cb(moduleType, "未配置正确的模块");
    }
}

// 示例：
// 0,1;7,1;B:0
// 股票-已转系统  两融-已转系统  期权-未转系统
GwModuleTypeEnum RtHst2Auth::ParseModuleTypeByData(const std::string &statusStr, LoginTypeEnum loginType)
{
    auto TransType = [](char type) -> LoginTypeEnum
    {
        switch (type)
        {
        case '0':
            return LoginTypeEnum::Stock;
        case 'B':
            return LoginTypeEnum::Option;
        case '7':
            return LoginTypeEnum::Margin;
        }
        return LoginTypeEnum::None;
    };

    // 柜台没有迁移标识，认为还没切，则返回恒生标识
    if (statusStr.empty())
    {
        return GwModuleTypeEnum::HST2;
    }

    GwModuleTypeEnum dstType = GwModuleTypeEnum::NONE;

    auto items = str::Split(statusStr, ';');
    for (const auto &item : items)
    {
        char type = STD::GetValueChar(item.data(), 1, ',');
        if (TransType(type) == loginType)
        {
            int flag = STD::GetValueInt(item.data(), 2, ',');
            if (flag == 1)
            {
                dstType = GwModuleTypeEnum::DDA5;
            }
            else
            {
                dstType = GwModuleTypeEnum::HST2;
            }
            break;
        }
    }

    return dstType;
}

namespace
{
    std::string ParseIpFromOpStationByRegex(const std::string &opStation)
    {
        auto ip = pobo::SubStringByRegex(opStation, ".*IIP=(.*?);.*");
        if (ip.empty() || ip == "NA")
            return "";

        if (ip.front() == '[' && ip.back() == ']')
            return "";

        return ip;
    }

    std::string ParseMacFromOpStationByRegex(const std::string &opStation)
    {
        auto mac = pobo::SubStringByRegex(opStation, ".*MAC=(.*?);.*");
        if (mac.empty() || mac == "NA")
            return "";

        if (mac.front() == '[' && mac.back() == ']')
            return "";

        return mac;
    }
}

std::string RtHst2Auth::ParseLastLoginInfo(const std::string &last_op_station)
{
    if (last_op_station.empty())
    {
        return {};
    }

    // 空格 分隔 两个数据
    return fmt::format("{} {}",
                       ParseIpFromOpStationByRegex(last_op_station),
                       ParseMacFromOpStationByRegex(last_op_station));
}

GwModuleTypeEnum RtHst2Auth::GetCachedRsp(const std::string &accountId)
{
    std::lock_guard<std::mutex> guard(m_Mtx);

    auto it = m_AccountWithAskingTime.find(accountId);
    if (it == m_AccountWithAskingTime.end())
    {
        return GwModuleTypeEnum::NONE;
    }

    if (time(nullptr) - it->second.first > s_AccountAuthValidSeconds) // 超过有效时间
    {
        return GwModuleTypeEnum::NONE;
    }

    return it->second.second;
}

void RtHst2Auth::AddCachedRsp(const std::string &accountId, GwModuleTypeEnum type)
{
    std::lock_guard<std::mutex> guard(m_Mtx);

    m_AccountWithAskingTime[accountId] = std::make_pair(time(nullptr), type);
}