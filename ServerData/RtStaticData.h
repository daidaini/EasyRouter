/*
 * @Descripttion: 一些静态数据，包括像mac地址等硬件信息
 * @Author: yubo
 * @Date: 2022-08-11 14:55:18
 * @LastEditTime: 2022-09-13 10:56:33
 */
#pragma once

#include "RtServerDataDefine.h"

#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <ifaddrs.h>
#include <netpacket/packet.h>

namespace pobo
{
    constexpr double s_CloseDelayTime = 0.5; // 秒

    const std::map<std::string, std::string> &GetEthNameWithMac();

    std::vector<std::string> GetMacAddrs();

    const std::map<std::string, std::pair<std::string, std::string>> &GetPhysicalIpAddrs();

    std::vector<std::string> LocalIp(bool isIpv6);

    const std::vector<u_char *> &GetPriKeys();
}