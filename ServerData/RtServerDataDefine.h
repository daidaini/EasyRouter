#pragma once

#include "PbCommunicateData.h"
#include "GatewayDataType.h"
#include "Timestamp.h"
#include "ThreadPool.h"
#include "CachedGatePBStep.h"
#include "stepdef.h"
#include "Callbacks.h"

#include <sys/types.h>
#include <syscall.h>

#include <map>
#include <vector>
#include <assert.h>
#include <memory>
#include <functional>
#include <string>
#include <thread>
#include <array>
#include <unordered_map>
#include <cassert>
#include <atomic>
#include <mutex>
#include <utility>
#include <queue>
#include <list>
#include <exception>

#include <fmt/format.h>
#include <fmt/color.h>

namespace pobo
{
    extern muduo::ThreadPool g_ThreadPool;

    class ConnectionUser;

    using TcpConnIDType = int;
    using ConnectionUserPtr = std::shared_ptr<ConnectionUser>;

    using ReqPkgPtr = GatePBStep *;
    using RspPkgPtr = GatePBStep *;
    using PushPkgPtr = GatePBStep *;
    struct MsgPackType
    {
        ReqPkgPtr Request;
        RspPkgPtr Response;
        muduo::Timestamp RequestTime;

        MsgPackType(ReqPkgPtr req, RspPkgPtr rsp, muduo::Timestamp t)
            : Request(req), Response(rsp), RequestTime(t)
        {
        }

        MsgPackType()
            : Request(nullptr), Response(nullptr)
        {
        }

        void Release()
        {
            if (Request != nullptr)
            {
                delete Request;
                Request = nullptr;
            }

            if (Response != nullptr)
            {
                delete Response;
                Response = nullptr;
            }
        }
    };

    using UserCacheType = std::vector<MsgPackType>;

    using OnResponseFuncType = std::function<void(MsgPackType)>;
}
