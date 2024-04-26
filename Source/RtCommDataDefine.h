#pragma once

#include "muduo/net/EventLoopThread.h"
#include "muduo/net/EventLoop.h"
#include "muduo/net/InetAddress.h"
#include "muduo/net/TcpClient.h"
#include "muduo/net/TcpServer.h"

#include <fmt/format.h>
#include <fmt/color.h>

#include <map>
#include <unordered_map>
#include <memory>
#include <vector>
#include <mutex>

#include <syscall.h>

#include "SpdLogger.h"

using TcpConnIDType = int;

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

inline GwModuleTypeEnum TGwModuleTypeFromStr(std::string name)
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