#pragma once

#include <fmt/format.h>

namespace EasyRouter
{
    constexpr const int mainVer = 1;
    constexpr const int subVer = 0;
    constexpr const int pubVer = 0;
    constexpr const int buildVer = 0;
    constexpr const int updateDate = 20240523;
#ifdef NDEBUG
    static const std::string dsp = "Easy ROuter";
#else
    static const std::string dsp = "Easy ROuter(DEBUG)";
#endif
    static const std::string VersionInfo = fmt::format("V{}.{}.{}.{}({})({})", mainVer, subVer, pubVer, buildVer, updateDate, dsp);
    static const std::string VersionNumber = fmt::format("V{}.{}.{}.{}({})", mainVer, subVer, pubVer, buildVer, updateDate);
}