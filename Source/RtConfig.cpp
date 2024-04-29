#include "RtConfig.h"
#include "json/json.h"
#include "PoboTool.h"
#include "StringFunc.h"

void RtConfig::LoadConfig()
{
    const std::string configFile = "./config.json";
    auto contents = pobo::ReadContentsFromFile(configFile);
    if (contents.empty())
    {
        fmt::print(fg(fmt::color::pale_violet_red), "缺少配置文件[{}]\n", configFile);
        exit(-1);
    }

    Json::Value jval;
    Json::Reader jReader;
    if (!jReader.parse(contents, jval))
    {
        fmt::print(fg(fmt::color::pale_violet_red), "[{}]数据格式错误\n", configFile);
        exit(-1);
    }

    Json::Value::Members mems = jval.getMemberNames();
    m_ServerPort = jval["server_port"].asInt();
    m_ServerThreadCnt = jval["server_thread_cnt"].asInt();

    auto &dstAddrArr = jval["dst_addrs"];
    for (auto i = 0U; i < dstAddrArr.size(); ++i)
    {
        auto moduleType = GwModuleTypeFromStr(str::Upper(dstAddrArr[i]["dsp"].asString()));

        m_DstAddrGroups[moduleType].push_back(
            muduo::net::InetAddress(dstAddrArr[i]["ip"].asCString(), dstAddrArr[i]["port"].asInt()));
    }

    m_RouterAuthThreadCnt = jval["router_auth"]["thread_cnt"].asInt();
    m_RouterAuthAddr = jval["router_auth"]["address"].asString();
    m_RouterAuthType = TransRouterAuthType(jval["router_auth"]["type"].asInt());
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