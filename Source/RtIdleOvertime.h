#pragma once

#include "RtCommDataDefine.h"

namespace EasyRouter
{
    class IdleConnOverTime final
    {
    public:
        IdleConnOverTime() = default;
        ~IdleConnOverTime() = default;

        void Add(TcpConnIDType connId);

        void Erase(TcpConnIDType connId);

        void UpdateTime(TcpConnIDType connId);

        void CheckOverTimer();

    private:
        std::unordered_map<TcpConnIDType, time_t> m_ActiveTime;
        std::unordered_map<TcpConnIDType, time_t> m_TmpActiveTime;
        std::mutex m_TimeMtx;
    };
}