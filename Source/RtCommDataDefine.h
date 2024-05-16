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

constexpr int s_MaxPackgeSize = 4096;

enum class RouterAuthType
{
    // 三方认证
    ThirdSysAuth,
    // 本地规则
    LocalRule,
    // 空
    None,
};

inline RouterAuthType TransRouterAuthType(int type)
{
    switch (type)
    {
    case 1:
        return RouterAuthType::ThirdSysAuth;
    case 2:
        return RouterAuthType::LocalRule;
    }

    return RouterAuthType::None;
}

// 网关模块类型定义
enum class GwModuleTypeEnum
{
    NONE = -1,
    HST2 = 1, // 恒生T2
    HST3 = 2, // 恒生T3
    DD20 = 3, // 顶点A2
    DDA5 = 4, // 顶点A5
    HTS_Option = 5,
    HTS_Stock = 6,
    JSD = 7, // 金仕达期权
    JSD_Stock = 8,
    JSD_Gold = 9,
    AMS = 10,
    CTP = 11,
    UT = 12,
};

using ModuleGroupType = std::pair<GwModuleTypeEnum, LoginTypeEnum>;

using RtDstCallbackFuncType = std::function<void(GwModuleTypeEnum, std::string)>;

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

    auto it = s_NameMapperType.find(name);
    if (it == s_NameMapperType.end())
    {
        return GwModuleTypeEnum::NONE;
    }

    return it->second;
}

inline std::string GwModuleTypeToStr(GwModuleTypeEnum type)
{
    static std::map<GwModuleTypeEnum, std::string> s_TypeMapperName{
        {GwModuleTypeEnum::HST2, "HST2"},
        {GwModuleTypeEnum::HST3, "HST3"},
        {GwModuleTypeEnum::DD20, "DD20"},
        {GwModuleTypeEnum::DDA5, "DDA5"},
        {GwModuleTypeEnum::HTS_Option, "HTS_Option"},
        {GwModuleTypeEnum::HTS_Stock, "HTS_Stock"},
        {GwModuleTypeEnum::JSD, "JSD"},
        {GwModuleTypeEnum::JSD_Stock, "JSD_Stock"},
        {GwModuleTypeEnum::JSD_Gold, "JSD_Gold"},
        {GwModuleTypeEnum::AMS, "AMS"},
        {GwModuleTypeEnum::CTP, "CTP"},
        {GwModuleTypeEnum::UT, "UT"},
        {GwModuleTypeEnum::NONE, "NONE"},
    };

    return s_TypeMapperName[type];
}

inline LoginTypeEnum LoginTypeFromStr(std::string name)
{
    static std::map<std::string, LoginTypeEnum> s_LoginTypeMapperNames{
        {"option", LoginTypeEnum::Option},
        {"stock", LoginTypeEnum::Stock},
        {"margin", LoginTypeEnum::Margin}};

    auto it = s_LoginTypeMapperNames.find(name);
    if (it == s_LoginTypeMapperNames.end())
    {
        return LoginTypeEnum::None;
    }
    return it->second;
}

struct AuthRequestParam
{
    std::string AccountId;
    std::string Password;
    LoginTypeEnum LoginType;
    AccountTypeEnum AccountType;

    AuthRequestParam() = default;
    AuthRequestParam(
        std::string acc, std::string pwd, LoginTypeEnum loginType, AccountTypeEnum accType)
        : AccountId(std::move(acc)), Password(std::move(pwd)), LoginType(loginType), AccountType(accType)
    {
    }
};