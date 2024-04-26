#include "RtConfig.h"

void RtConfig::LoadConfig()
{
    m_DstAddrGroups[GwModuleTypeEnum::HST2].push_back(muduo::net::InetAddress("127.0.0.1", 20002));
}

muduo::net::InetAddress *RtConfig::DstAddr(GwModuleTypeEnum type, size_t index)
{
    auto it = m_DstAddrGroups.find(type);

    if (it != m_DstAddrGroups.end())
    {
        if (it->second.size() > index)
        {
            return &(it->second.at(index));
        }
    }

    return nullptr;
}