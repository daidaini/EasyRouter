#pragma once

#include <fmt/format.h>

namespace HDGwRouter
{
    constexpr const int mainVer = 1;
    constexpr const int subVer = 0;
    constexpr const int pubVer = 0;
    constexpr const int buildVer = 0;
    constexpr const int updateDate = 20240507;
#ifdef NDEBUG
    static const std::string dsp = "汇点网关路由服务";
#else
    static const std::string dsp = "汇点网关路由服务(DEBUG)";
#endif
    static const std::string VersionInfo = fmt::format("V{}.{}.{}.{}({})({})", mainVer, subVer, pubVer, buildVer, updateDate, dsp);
    static const std::string VersionNumber = fmt::format("V{}.{}.{}.{}({})", mainVer, subVer, pubVer, buildVer, updateDate);
}