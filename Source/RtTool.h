#pragma once

#include "PbPkgHandle.h"

#include "RtCommDataDefine.h"

namespace HDGwRouter
{
    int DecrpytStaticKey(char *outBuf, const pobo::CommMessage &msg);

    std::string UnLoadPackageStatic(const pobo::CommMessage &msg);

    std::string UnLoadPackageDefault(const pobo::CommMessage &msg, AES_KEY &decryptKey);
}