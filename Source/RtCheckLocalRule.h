#pragma once

/***
 * 使用python3脚本，检测账号规则来获取路由目标
 * python脚本定义的方法返回对应的路由目标（使用整型返回）
 */

#include <Python.h>
#include "RtCommDataDefine.h"

class RtCheckLocalRule final
{
public:
    RtCheckLocalRule();
    ~RtCheckLocalRule();

    void CheckRtByAccount(const std::string &accountId, RtDstCallbackFuncType cb);

private:
    GwModuleTypeEnum GetCachedRsp(const std::string &accountId);

    void AddCachedRsp(const std::string &accountId, GwModuleTypeEnum type);

private:
    PyObject *m_PyModule = nullptr;
    PyObject *m_CheckAccountFunc = nullptr;
    std::mutex m_PyModuleMtx;

    //<account,  dsttype>
    std::map<std::string, GwModuleTypeEnum> m_AccountWithDstType;

    std::mutex m_Mtx;
};
