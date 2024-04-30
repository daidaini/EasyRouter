#pragma once

#include "muduo/net/EventLoopThread.h"
#include "muduo/net/EventLoop.h"
#include "muduo/net/InetAddress.h"
#include "muduo/net/TcpClient.h"
#include "muduo/net/TcpServer.h"

#include <syscall.h>

#include "SpdLogger.h"

#include "PbStaticData.h"

using TcpConnIDType = int;

enum class RouterAuthType
{
    ThirdSys,
    None,
};

inline RouterAuthType TransRouterAuthType(int type)
{
    switch (type)
    {
    case 1:
        return RouterAuthType::ThirdSys;
    }

    return RouterAuthType::None;
}

// 网关模块类型定义
enum class GwModuleTypeEnum
{
    HST2, // 恒生T2
    HST3, // 恒生T3
    DD20, // 顶点A2
    DDA5, // 顶点A5
    HTS_Option,
    HTS_Stock,
    JSD, // 金仕达期权
    JSD_Stock,
    JSD_Gold,
    AMS,
    CTP,
    UT,
};

inline GwModuleTypeEnum GwModuleTypeFromStr(std::string name)
{
    static std::map<std::string, GwModuleTypeEnum> s_NameMapperType{
        {"HST2", GwModuleTypeEnum::HST2},
        {"HST3", GwModuleTypeEnum::HST3},
        {"DD20", GwModuleTypeEnum::DD20},
        {"DDA5", GwModuleTypeEnum::DDA5},
        {"HTS_Option", GwModuleTypeEnum::HTS_Option},
        {"HTS_Stock", GwModuleTypeEnum::HTS_Stock},
        {"JSD", GwModuleTypeEnum::JSD},
        {"JSD_Stock", GwModuleTypeEnum::JSD_Stock},
        {"JSD_Gold", GwModuleTypeEnum::JSD_Gold},
        {"AMS", GwModuleTypeEnum::AMS},
        {"CTP", GwModuleTypeEnum::CTP},
        {"UT", GwModuleTypeEnum::UT},
    };

    return s_NameMapperType[name];
}

struct AuthRequestParam
{
    std::string AccountId;
    std::string Password;
    LoginTypeEnum LoginType;
    AccountTypeEnum AccountType;

    AuthRequestParam(
        std::string acc, std::string pwd, LoginTypeEnum loginType, AccountTypeEnum accType)
        : AccountId(std::move(acc)), Password(std::move(pwd)), LoginType(loginType), AccountType(accType)
    {
    }
};