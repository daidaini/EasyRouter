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
        exit(0);
    }
    try
    {
        Json::Value jval;
        Json::Reader jReader;
        if (!jReader.parse(contents, jval))
        {
            fmt::print(fg(fmt::color::pale_violet_red), "[{}]数据格式错误\n", configFile);
            exit(-1);
        }

        Json::Value::Members mems = jval.getMemberNames();
        int servPort = jval["server_port"].asInt();
        if (servPort != 0)
        {
            m_ServerPort = servPort;
        }

        int threadCnt = jval["server_thread_cnt"].asInt();
        if (threadCnt != 0)
        {
            m_ServerThreadCnt = threadCnt;
        }

        auto &dstAddrArr = jval["dst_addrs"];
        for (auto i = 0U; i < dstAddrArr.size(); ++i)
        {
            auto moduleType = GwModuleTypeFromStr(dstAddrArr[i]["group"].asString());

            m_DstAddrGroups[moduleType].push_back(
                muduo::net::InetAddress(dstAddrArr[i]["ip"].asCString(), dstAddrArr[i]["port"].asInt()));
        }

        m_RouterAuthThreadCnt = jval["router_auth"]["thread_cnt"].asInt();

        m_RouterAuthType = TransRouterAuthType(jval["router_auth"]["type"].asInt());

        auto &jSubVal = jval["router_auth"]["third_system_items"];
        if (jSubVal.type() != Json::ValueType::nullValue)
        {
            Json::Value::Members mems = jSubVal.getMemberNames();

            for (const auto &item : mems)
            {
                m_RouterAuthParams.emplace(item, jSubVal[item].asString());
            }
        }
    }
    catch (const std::runtime_error &err)
    {
        fmt::print("LoadConfig error . {}\n", err.what());
        exit(0);
    }
}

muduo::net::InetAddress *RtConfig::DstAddr(GwModuleTypeEnum type, size_t index)
{
    auto it = m_DstAddrGroups.find(type);

    if (it != m_DstAddrGroups.end())
    {
        index = index % it->second.size();

        return &(it->second.at(index));
    }

    return nullptr;
}