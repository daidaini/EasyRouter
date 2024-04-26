#pragma once

#include "RtCommDataDefine.h"

class RtConfig
{
public:
    RtConfig() = default;
    ~RtConfig() = default;

    void LoadConfig();

    muduo::net::InetAddress *DstAddr(GwModuleTypeEnum type, size_t index);

public:
    int m_ServerThreadCnt{4};

    // 路由目标地址可以有多个分组，从配置中获取
    std::map<GwModuleTypeEnum, std::vector<muduo::net::InetAddress>> m_DstAddrGroups;
};