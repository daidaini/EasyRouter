#include "T2SyncConnection.h"
#include <string>
#include <fmt/format.h>
#include <fmt/color.h>

#include "SpdLogger.h"

using namespace HST2;

std::string g_op_entrust_way = "7";

void DoAuthentication(Connection *hsConn, std::string account, std::string pwd)
{
    hsConn->PrepareRequest(331100, "客户登录", 0);

    hsConn->MID_AddString("op_branch_no", "");
    hsConn->MID_AddString("op_entrust_way", g_op_entrust_way);
    hsConn->MID_AddString("op_station", "RouterServer:Auth");
    hsConn->MID_AddString("branch_no", "");
    hsConn->MID_AddChar("password_type", '2'); // 1表示资金密码，2表示交易密码 3表示查询密码
    hsConn->MID_AddString("password", pwd);
    hsConn->MID_AddString("account_content", account);
    hsConn->MID_AddString("content_type", "0");
    hsConn->MID_AddChar("input_content", '6');

    auto result = hsConn->MID_SendToData();
    if (::Failed(result.ErrCode))
    {
        fmt::print(fg(fmt::color::red), "获取柜台应答失败:[{}][{}]\n", (int)result.ErrCode, result.ErrMsg);
        return;
    }

    if (!hsConn->MID_MoveNextRec())
    {
        fmt::print(fg(fmt::color::red), "柜台应答数据获取不到\n");
        return;
    }

    std::string statusStr = hsConn->MID_GetString("asset_prop_status_str");
    fmt::print(fg(fmt::color::green), "asset_prop_status_str : {}\n");
}

int main(int argc, char *argv[])
{
    if (argc < 3)
    {
        fmt::print(fg(fmt::color::indian_red), "usage: \n\t./test_hst2 account password\n");
        exit(1);
    }

    std::string accountId = argv[1];
    std::string password = argv[2];

    if (argc > 3)
    {
        g_op_entrust_way = argv[3];
    }

    SpdLogger::Instance().Init(std::vector<LogType>{LogType::System});
    CConnPool<Connection> g_HsConnPool;

    Connection *hsConn = g_HsConnPool.GetConnectWithCheckConnectFlag();
    if (hsConn == nullptr)
    {
        // SpdLogger::Instance().WriteLog(LogType::System, LogLevel::Warn, "创建恒生柜台连接失败");
        fmt::print(fg(fmt::color::red), "创建恒生柜台连接失败\n");
        exit(1);
    }

    DoAuthentication(hsConn, accountId, password);

    exit(0);
}