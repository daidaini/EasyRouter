#include "RtTool.h"

using namespace pobo;
namespace EasyRouter
{
    std::string UnLoadPackageDefault(const pobo::CommMessage &msg, AES_KEY &decryptKey)
    {
        unsigned char cachedMsgBuf[s_MaxPackgeSize]{};
        pobo::PoboPkgHandle::DecryptPackage(cachedMsgBuf, (const unsigned char *)msg.Body.data(), msg.Head.PackageSize, decryptKey);

        u_long msglen = s_MaxPackgeSize;
        char uploadedMsg[s_MaxPackgeSize]{};
        auto result = pobo::PoboPkgHandle::UncompressFromZip(uploadedMsg, msglen, cachedMsgBuf, strlen((const char *)cachedMsgBuf + sizeof(PB_ZipHead)));
        if (!result.first)
        {
            SpdLogger::Instance().WriteLog(LogType::System, LogLevel::Warn, "UnLoadPackageDefault failed[{}]", result.second);
            return {};
        }

        return std::string(uploadedMsg + 2, msglen - 2);
    }

    int DecrpytStaticKey(char *outBuf, const pobo::CommMessage &msg)
    {
        const auto &prikeys = GetPriKeys();

        if (prikeys.empty())
        {
            return PoboPkgHandle::DecryptPackgeRSA((u_char *)outBuf, (const unsigned char *)msg.Body.data(), msg.Head.PackageSize);
        }

        for (const u_char *prikey : prikeys)
        {
            int ret = PoboPkgHandle::DecryptPackgeRSA((u_char *)outBuf, (const unsigned char *)msg.Body.data(), msg.Head.PackageSize, prikey);
            if (ret >= 0)
            {
                return ret;
            }

            SpdLogger::Instance().WriteLog(LogType::System, LogLevel::Debug, "DecryptPackgeRSA this key failed[{}], continue..", ret);
            continue;
        }

        SpdLogger::Instance().WriteLog(LogType::System, LogLevel::Error, "DecryptPackgeRSA all private keys failed.");
        return -1;
    }

    std::string UnLoadPackageStatic(const pobo::CommMessage &msg)
    {
        u_char tmpBuf[s_MaxPackgeSize]{};
        int tmpSize = DecrpytStaticKey((char *)tmpBuf, msg);
        if (tmpSize < 0)
        {
            return "";
        }

        u_long msglen = s_MaxPackgeSize;
        char uploadedMsg[s_MaxPackgeSize]{};
        auto result = PoboPkgHandle::UncompressFromZip(uploadedMsg, msglen, tmpBuf, tmpSize - sizeof(PB_ZipHead));
        if (!result.first)
        {
            return "";
        }
        // 2位位移，前两位是校验码
        return std::string(uploadedMsg + 2, msglen - 2);
    }
}
