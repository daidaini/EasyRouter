#include "RtStaticData.h"
#include "PbPkgHandle.h"

const std::map<std::string, std::string> &GetEthNameWithMac()
{
    static std::map<std::string, std::string> macs;
    if (!macs.empty())
    {
        return macs;
    }

    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    if (sock == -1)
    {
        return macs;
    }

    char buf[2048]{};
    ifconf ifc;
    ifc.ifc_len = sizeof(buf);
    ifc.ifc_buf = buf;
    if (ioctl(sock, SIOCGIFCONF, &ifc) == -1)
    {
        return macs;
    }

    ifreq *it = ifc.ifc_req;
    const ifreq *end = it + (ifc.ifc_len / sizeof(ifreq));

    for (; it != end; ++it)
    {
        ifreq ifr;
        strcpy(ifr.ifr_name, it->ifr_name);
        if (ioctl(sock, SIOCGIFFLAGS, &ifr) == 0)
        {
            if (strstr(ifr.ifr_name, "docker") != NULL)
            {
                // 过滤 docker的mac
                continue;
            }
            if (!(ifr.ifr_flags & IFF_LOOPBACK))
            {
                if (ioctl(sock, SIOCGIFHWADDR, &ifr) == 0)
                {
                    unsigned char *ptr;
                    ptr = (unsigned char *)&ifr.ifr_ifru.ifru_hwaddr.sa_data[0];
                    char szMac[64]{};
                    snprintf(szMac, 64, "%02X%02X%02X%02X%02X%02X", *ptr, *(ptr + 1), *(ptr + 2), *(ptr + 3), *(ptr + 4), *(ptr + 5));
                    // printf("Interface name : %s , Mac address : %s \n", ifr.ifr_name, szMac);

                    macs.emplace(std::string(ifr.ifr_name), szMac);
                }
            }
        }
        else
        {
            continue;
        }
    }

    return macs;
}

std::vector<std::string> GetMacAddrs()
{
    std::vector<std::string> result;

    const auto &macs = GetEthNameWithMac();
    for (const auto &item : macs)
    {
        result.emplace_back(item.second);
    }

    return result;
}

auto IsPhysical = [](struct ifaddrs *ifa) -> bool
{
    return (ifa->ifa_flags & IFF_LOOPBACK) == 0 &&
           strstr(ifa->ifa_name, "veth") == NULL &&
           strstr(ifa->ifa_name, "vir") == NULL &&
           strstr(ifa->ifa_name, "tap") == NULL &&
           strstr(ifa->ifa_name, "docker") == NULL;
};

const std::map<std::string, std::pair<std::string, std::string>> &GetPhysicalIpAddrs()
{
    // key: eth name
    static std::map<std::string, std::pair<std::string, std::string>> ipAddrs;
    if (!ipAddrs.empty())
    {
        return ipAddrs;
    }

    auto UpdateIpAddr = [](std::string ethName, const char *addr, bool isIpv6)
    {
        auto it = ipAddrs.find(ethName);
        if (it == ipAddrs.end())
        {
            auto ret = ipAddrs.insert(std::make_pair(std::move(ethName), std::pair<std::string, std::string>{}));
            it = ret.first;
        }
        if (isIpv6)
        {
            it->second.second = addr;
        }
        else
        {
            it->second.first = addr;
        }
    };

    struct ifaddrs *ifAddrStruct = NULL;
    struct ifaddrs *ifa = NULL;

    if (getifaddrs(&ifAddrStruct) == -1)
    {
        return ipAddrs;
    }

    for (ifa = ifAddrStruct; ifa != NULL; ifa = ifa->ifa_next)
    {
        if (!ifa->ifa_addr)
        {
            continue;
        }
        if (ifa->ifa_addr->sa_family == AF_INET) // check it is IP4
        {
            if (!IsPhysical(ifa))
            {
                continue;
            }
            // is a valid IP4 Address
            void *tmpAddrPtr = &((sockaddr_in *)ifa->ifa_addr)->sin_addr;
            char addressBuffer[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, tmpAddrPtr, addressBuffer, INET_ADDRSTRLEN);

            UpdateIpAddr(std::string(ifa->ifa_name), addressBuffer, false);
            // printf("%s IP Address %s\n", ifa->ifa_name, addressBuffer);
        }
        else if (ifa->ifa_addr->sa_family == AF_INET6) // check it is IP6
        {
            if (!IsPhysical(ifa))
            {
                continue;
            }
            // is a valid IP6 Address
            void *tmpAddrPtr = &((sockaddr_in6 *)ifa->ifa_addr)->sin6_addr;
            char addressBuffer[INET6_ADDRSTRLEN];
            inet_ntop(AF_INET6, tmpAddrPtr, addressBuffer, INET6_ADDRSTRLEN);

            UpdateIpAddr(std::string(ifa->ifa_name), addressBuffer, true);
            // printf("%s IP Address %s\n", ifa->ifa_name, addressBuffer);
        }
    }
    if (ifAddrStruct != NULL)
    {
        freeifaddrs(ifAddrStruct);
    }

    return ipAddrs;
}

std::vector<std::string> LocalIp(bool isIpv6)
{
    std::vector<std::string> result;

    const auto &ipaddrs = GetPhysicalIpAddrs();

    if (isIpv6)
    {
        for (const auto &item : ipaddrs)
        {
            result.emplace_back(item.second.second);
        }
    }
    else
    {
        for (const auto &item : ipaddrs)
        {
            result.emplace_back(item.second.first);
        }
    }
    return result;
}

const std::vector<u_char *> &GetPriKeys()
{
    static std::vector<u_char *> prikeys;
    if (!prikeys.empty())
    {
        return prikeys;
    }

    static std::vector<std::string> prikeyFiles{
        "pri.key", "pri_1.key", "pri_2.key", "pri_3.key"};

    for (const auto &filename : prikeyFiles)
    {
        FILE *fp = fopen(filename.c_str(), "rb");
        if (fp != nullptr)
        {
            VerHead verhaed{};
            fread(&verhaed, sizeof(VerHead), 1, fp);
            if (strcmp(verhaed.Head, "pengbo") != 0)
            {
                continue;
            }

            char decData[MAX_RSA_KEY_LEN]{};
            u_char *keyData = new u_char[MAX_RSA_KEY_LEN]{};
            fread(decData, verhaed.FileLen, 1, fp);
            // decrpyt
            static const char encrytKey[]{"pobo1234"};
            auto result = pobo::PoboPkgHandle::DecryptKeyData(decData, verhaed.FileLen, encrytKey, (char *)keyData, MAX_RSA_KEY_LEN);
            if (result.first < 0)
            {
                fmt::print("Read pri key[{}] failed. [{}]. \nCheck the config of pri keys..\n", filename, result.second);
            }
            else
            {
                prikeys.emplace_back(keyData);
            }
        }
    }

    return prikeys;
}