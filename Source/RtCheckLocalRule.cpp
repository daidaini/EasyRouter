#include "RtCheckLocalRule.h"

namespace
{
    const std::string s_LocalRulePyModule = "check_local_rule"; // check_local_rule.py

    const std::string s_CheckAccountFunc = "check_account";
}

RtCheckLocalRule::RtCheckLocalRule()
{
    Py_Initialize();

    m_PyModule = PyImport_ImportModule(s_LocalRulePyModule.data());
    if (m_PyModule == nullptr)
    {
        SpdLogger::Instance().WriteLog(LogType::System, LogLevel::Warn, "PyImport_ImportModule [{}] failed..", s_LocalRulePyModule);
    }
    else
    {
        std::lock_guard<std::mutex> guard(m_PyModuleMtx);
        m_CheckAccountFunc = PyObject_GetAttrString(m_PyModule, s_CheckAccountFunc.data());
        if (m_CheckAccountFunc == nullptr)
        {
            SpdLogger::Instance().WriteLog(LogType::System, LogLevel::Warn, "PyObject_GetAttrString [{}] failed..", s_CheckAccountFunc);
        }
    }
}

RtCheckLocalRule::~RtCheckLocalRule()
{
    if (m_CheckAccountFunc != nullptr)
    {
        Py_DECREF(m_CheckAccountFunc);
    }

    if (m_PyModule != nullptr)
    {
        Py_DECREF(m_PyModule);
    }

    Py_Finalize(); // Finalize the Python interpreter
}

void RtCheckLocalRule::CheckRtByAccount(const std::string &accountId, RtDstCallbackFuncType cb)
{
    GwModuleTypeEnum gwType = GetCachedRsp(accountId);
    if (gwType != GwModuleTypeEnum::NONE)
    {
        return cb(gwType);
    }

    if (m_PyModule == nullptr || m_CheckAccountFunc == nullptr)
    {
        return cb(gwType);
    }

    PyObject *pArg = PyUnicode_FromString(accountId.c_str());
    if (pArg == nullptr)
    {
        SpdLogger::Instance().WriteLog(LogType::System, LogLevel::Warn, "PyUnicode_FromString failed..");
        return cb(gwType);
    }

    PyObject *pArgs = PyTuple_New(1);
    PyTuple_SetItem(pArgs, 0, pArg);

    std::unique_lock<std::mutex> lock(m_PyModuleMtx);
    PyObject *pResult = PyObject_CallObject(m_CheckAccountFunc, pArgs);
    lock.unlock();
    if (pResult != nullptr)
    {
        if (PyLong_Check(pResult))
        {
            gwType = static_cast<GwModuleTypeEnum>(PyLong_AsLong(pResult));
        }
    }

    cb(gwType);

    if (gwType != GwModuleTypeEnum::NONE)
    {
        AddCachedRsp(accountId, gwType);
    }

    Py_DECREF(pResult);
    Py_DECREF(pArgs);
}

void RtCheckLocalRule::AddCachedRsp(const std::string &accountId, GwModuleTypeEnum type)
{
    std::lock_guard<std::mutex> guard(m_Mtx);

    m_AccountWithDstType[accountId] = type;
}

GwModuleTypeEnum RtCheckLocalRule::GetCachedRsp(const std::string &accountId)
{
    std::lock_guard<std::mutex> guard(m_Mtx);

    auto it = m_AccountWithDstType.find(accountId);
    if (it == m_AccountWithDstType.end())
    {
        return GwModuleTypeEnum::NONE;
    }
    return it->second;
}