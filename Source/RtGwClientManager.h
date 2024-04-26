#pragma once
/**
 * @brief 管理跟网关之间的client
 *
 */

#include "RtDstClient.h"

class RtGwClientManager
{
public:
    RtGwClientManager() = default;
    ~RtGwClientManager() = default;

    void AddClient(TcpConnIDType srcConnId, std::unique_ptr<DstClient> client)
    {
        std::lock_guard<std::mutex> gurard(m_DstMtx);

        m_DstClientMapper.emplace(srcConnId, std::move(client));
    }

    void EraseClient(TcpConnIDType srcConnId)
    {
        std::lock_guard<std::mutex> gurard(m_DstMtx);

        m_DstClientMapper.erase(srcConnId);
    }

    DstClient *GetClient(TcpConnIDType srcConnId)
    {
        std::lock_guard<std::mutex> gurard(m_DstMtx);

        auto it = m_DstClientMapper.find(srcConnId);
        if (it != m_DstClientMapper.end())
        {
            return it->second.get();
        }

        return nullptr;
    }

private:
    //<srcConn, dstClient>
    std::unordered_map<TcpConnIDType, std::unique_ptr<DstClient>>
        m_DstClientMapper;

    std::mutex m_DstMtx;
};