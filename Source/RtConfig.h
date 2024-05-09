#pragma once

#include "RtCommDataDefine.h"

class RtConfig
{
public:
    RtConfig() = default;
    ~RtConfig() = default;

    void LoadConfig();

    muduo::net::InetAddress *DstAddr(ModuleGroupType type, size_t index);

    void CheckBackCutFileOnTimer();

public:
    uint16_t m_ServerPort{0};
    int m_ServerThreadCnt{4};

    int m_RouterAuthThreadCnt{4};

    RouterAuthType m_RouterAuthType;

    // 路由认证所需要的参数字段
    std::map<std::string, std::string> m_RouterAuthParams;

    // 路由目标地址可以有多个分组，从配置中获取
    std::map<ModuleGroupType, std::vector<muduo::net::InetAddress>> m_DstAddrGroups;

    std::atomic_bool m_IsNeedBackcut{false};
};