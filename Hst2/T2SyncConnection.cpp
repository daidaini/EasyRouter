#include "T2SyncConnection.h"
#include "STD.h"
#include "SpdLogger.h"
#include "PoboTool.h"

namespace HST2
{
    CConfigInterface *GenerateConfiger()
    {
        static CConfigInterface *lpConfig = NewConfig();
        if (lpConfig == nullptr)
            return nullptr;

        lpConfig->AddRef();
        if (lpConfig->Load(s_T2ConfigPath) != 0) // 装载配置文件
        {
            throw std::runtime_error(
                fmt::format("Load config({}) failed", s_T2ConfigPath));
        }

        return lpConfig;
    }

    Connection::Connection()
    {
        m_HsConfiger = GenerateConfiger();
        if (m_HsConfiger == nullptr)
        {
            throw std::runtime_error("GenerateConfiger 失败.");
        }

        m_HsConnectionPtr = NewConnection(m_HsConfiger);
        if (m_HsConnectionPtr == nullptr)
        {
            return;
            throw std::runtime_error("构造Connection时 NewConnection 失败.");
        }
        m_HsConnectionPtr->AddRef();
    }

    Connection::~Connection()
    {
        // 释放打包器
        if (m_IF2Packer != nullptr)
        {
            m_IF2Packer->Release();
        }

        // 释放连接对象
        if (m_HsConnectionPtr != nullptr)
        {
            m_HsConnectionPtr->Release();
        }

        // 解包器由SDK内部管理，外部切勿释放
        m_IF2UnPacker = nullptr;
    }

    std::pair<std::string, int16_t> Connection::GetAddrFromConfig()
    {
        std::string servers = m_HsConfiger->GetString("t2sdk", "servers", "");
        auto addrs = str::SplitWithStrip(servers, ';');
        if (addrs.empty())
        {
            throw std::runtime_error(fmt::format("{} [t2sdk]未配置 servers", s_T2ConfigPath));
        }

        int index = STD::rand(addrs.size());
        assert(index < (int)addrs.size());

        auto param = str::Split(addrs[index], ':');
        return std::make_pair(
            std::move(param.front()), std::stoi(param.back()));
    }

    bool Connection::Create()
    {
        if (m_HsConnectionPtr == nullptr)
        {
            return false;
        }
        // 初始化连接
        int result = m_HsConnectionPtr->Create(NULL);
        if (result != 0)
        {
            SpdLogger::Instance().WriteLog(LogType::System, LogLevel::Warn, "初始t2连接对象(Create)失败:[{}]", m_HsConnectionPtr->GetErrorMsg(result));
            return false;
        }

        result = m_HsConnectionPtr->Connect(s_ConnectTimeout);
        if (result != 0)
        {
            SpdLogger::Instance().WriteLog(LogType::System, LogLevel::Warn, "t2尝试连接(Connect)失败:[{}]", m_HsConnectionPtr->GetErrorMsg(result));
            return false;
        }

        // 准备打包器
        NewIF2Packer();

        m_IsConnected = true;
        return true;
    }

    void Connection::NewIF2Packer()
    {
        if (m_IF2Packer != nullptr)
            return;

        m_IF2Packer = NewPacker(2);
        if (m_IF2Packer == nullptr)
        {
            throw std::runtime_error("NewPacker 失败.");
        }

        m_IF2Packer->AddRef();
        /*设置打包器缓冲，缓冲由用户自己去管理，避免重复的建立和释放
         * 如果使用SetBuffer自己管理打包器的缓存空间，就不需要调用 FreeMem
         */
        m_IF2Packer->SetBuffer(m_HsPackerMemory, sizeof(m_HsPackerMemory));
    }

    bool Connection::GetSocketValid() const
    {
        if (!m_IsConnected)
            return false;

        int status = m_HsConnectionPtr->GetStatus();
        if ((status & CConnectionInterface::Registered) == CConnectionInterface::Registered)
        {
            return true;
        }
        return false;
    }

    void Connection::PrepareRequest(int hsFunID, const char *hsFuncName, int hsSysnodeId)
    {
        m_RequestParams.clear();
        m_CurrReqInfo.Assign(hsFunID, hsFuncName, hsSysnodeId);
    }

    void Connection::MID_AddString(std::string &&field, const std::string &value)
    {
        m_RequestParams.emplace(std::move(field), value);
    }

    void Connection::MID_SetString(std::string &&field, const std::string &value)
    {
        m_RequestParams[field] = value;
    }

    void Connection::MID_AddChar(std::string &&field, char value)
    {
        std::string tmp;
        if (value != '\0')
            tmp = std::string(1, value);

        m_RequestParams.emplace(std::move(field), std::move(tmp));
    }

    void Connection::MID_AddInt(std::string &&field, int value)
    {
        m_RequestParams.emplace(std::move(field), std::to_string(value));
    }

    GateErrorStruct Connection::MID_SendToData()
    {
        m_IF2Packer->BeginPack();
        for (const auto &item : m_RequestParams)
            m_IF2Packer->AddField(item.first.c_str(), 'S', item.second.length());

        for (const auto &item : m_RequestParams)
            m_IF2Packer->AddStr(item.second.c_str());

        // 打包器打包完毕
        m_IF2Packer->EndPack();

        // 请求日志
        LogRequest();

        // 发送业务数据，采用同步方式
        int ret = m_HsConnectionPtr->SendBiz(m_CurrReqInfo.FunID, m_IF2Packer, 0, m_CurrReqInfo.SysNode, 1);
        if (ret < 0)
        {
            return {GateError::NET_ERROR, m_HsConnectionPtr->GetErrorMsg(ret)};
        }

        m_IF2UnPacker = nullptr;
        m_CurrUnpackerRecIndex = 0;
        m_CurrUnpackerRecCnt = 0; // 清空当前记录数
        ret = m_HsConnectionPtr->RecvBiz(ret, (void **)&m_IF2UnPacker, s_ConnectTimeout, 0);
        if (ret < 0)
        {
            std::string tmp = m_HsConnectionPtr->GetErrorMsg(ret);
            char errmsg[256]{};
            pobo::GB2312ToUTF8((char *)tmp.data(), tmp.size(), errmsg, sizeof(errmsg));
            return {GateError::BIZ_ERROR, errmsg};
        }

        auto result = ParseRspPkg(ret);
        if (::Failed(result.ErrCode))
        {
            SpdLogger::Instance().WriteLog(LogType::System, LogLevel::Info, "[HST2][Response] Error:({})", result.ErrMsg);
        }
        else
        {
            m_CurrUnpackerRecCnt = m_IF2UnPacker->GetRowCount();
        }
        return result;
    }

    /*
     * 如果返回值等于0，表示业务数据接收成功，并且业务操作成功，*lppUnPackerOrStr指向一个解包器，此时应首先将该指针转换为IF2UnPacker *
     * 如果返回值等于1，表示业务数据接收成功，但业务操作失败了，*lppUnPackerOrStr指向一个解包器，此时应首先将该指针转换为IF2UnPacker *。
     * 如果返回值等于2，表示收到非业务错误信息，*lppUnPackerOrStr指向一个可读的字符串错误信息。
     * 如果返回值等于3，表示业务包解包失败。*lppUnPackerOrStr原先所指向的内容不会被改变。
     */
    GateErrorStruct Connection::ParseRspPkg(int code)
    {
        GateErrorStruct result{GateError::BIZ_ERROR, ""};

        switch (code)
        {
        case 0:
            return {GateError::SUCCESS, ""};
        case 1:
        {
            std::string tmp = GetPackerErrorInfo();
            char errmsg[256]{};
            pobo::GB2312ToUTF8((char *)tmp.data(), tmp.size(), errmsg, sizeof(errmsg));
            return {GateError::BIZ_ERROR, errmsg};
        }
        case 2:
            return {GateError::BIZ_ERROR, (const char *)m_IF2UnPacker};
        case 3:
            return {GateError::BIZ_ERROR, "解包失败"};
        default:
            return {GateError::BIZ_ERROR, "未知错误"};
        }
    }

    bool Connection::MID_MoveNextRec()
    {
        if (m_IF2UnPacker == nullptr || m_IF2UnPacker->IsEmpty())
            return false;

        if (m_CurrUnpackerRecIndex != 0)
        {
            m_IF2UnPacker->Next();
        }

        // 判断记录是否已经取完, 当IsEOF返回true表示记录已经取完，无法再next
        if (m_IF2UnPacker->IsEOF())
        {
            assert(m_CurrUnpackerRecIndex <= m_CurrUnpackerRecCnt);
            // 每次查询的记录数等于当次查询返回的记录数时，如果字段名里含有position_str表示需要发起后续查询
            if (m_CurrUnpackerRecCnt == s_QueryRecordNum && IsNeedNextPack())
            {
                auto pstr = MID_GetString(s_PositionStr);
                if (!pstr.empty())
                {
                    m_RequestParams[s_PositionStr] = std::move(pstr);
                    auto ret = MID_SendToData();
                    return ::Successed(ret.ErrCode);
                }
            }
            return false;
        }

        LogOneRspRecord();

        ++m_CurrUnpackerRecIndex;
        return true;
    }

    bool Connection::IsNeedNextPack() const
    {
        return m_RequestParams.find(s_PositionStr) != m_RequestParams.end();
    }

    int Connection::MID_GetInt(const char *field) const
    {
        assert(m_IF2UnPacker != nullptr);
        return m_IF2UnPacker->GetInt(field);
    }

    char Connection::MID_GetChar(const char *field) const
    {
        assert(m_IF2UnPacker != nullptr);
        return m_IF2UnPacker->GetChar(field);
    }

    std::string Connection::MID_GetString(const char *field) const
    {
        assert(m_IF2UnPacker != nullptr);
        const char *result = m_IF2UnPacker->GetStr(field);
        if (result != nullptr)
            return result;
        return "";
    }

    double Connection::MID_GetDouble(const char *field) const
    {
        if (m_IF2UnPacker != nullptr)
        {
            return m_IF2UnPacker->GetDouble(field);
        }
        return 0.0;
    }

    std::string Connection::GetPackerErrorInfo() const
    {
        if (m_IF2UnPacker != nullptr)
        {
            const char *err = m_IF2UnPacker->GetStr("error_info");
            if (err != nullptr)
                return err;
        }

        return "";
    }

    void Connection::LogOneRspRecord() const
    {
        char buffer[10240];
        int len = 0;
        if (m_IF2UnPacker != nullptr && m_IF2UnPacker->GetRowCount() > 0 && m_IF2UnPacker->GetColCount() > 0)
        {
            const char *fieldName = nullptr;
            const char *fieldValue = nullptr;

            for (int j = 0; j < m_IF2UnPacker->GetColCount(); ++j)
            {
                fieldName = m_IF2UnPacker->GetColName(j);
                if (fieldName != nullptr)
                {
                    fieldValue = m_IF2UnPacker->GetStr(fieldName);

                    len += snprintf(buffer + len, sizeof(buffer) - len, "(%s:%s)", fieldName, fieldValue);
                }
            }
        }

        buffer[len] = '\0';
        SpdLogger::Instance().WriteLog(LogType::System, LogLevel::Info, "[HST2][Response][{}:{}]:{}", m_CurrUnpackerRecIndex, m_CurrUnpackerRecCnt, buffer);
    }

    void Connection::LogRequest() const
    {
        std::string logstr = fmt::format("[HST2][Request]:({}, {})", m_CurrReqInfo.FunID, m_CurrReqInfo.FuncName);

        for (const auto &item : m_RequestParams)
        {
            if (item.first == "password")
            {
                logstr.append(fmt::format("({}:****)", item.first));
            }
            else
            {
                logstr.append(fmt::format("({}:{})", item.first, item.second));
            }
        }

        SpdLogger::Instance().WriteLog(LogType::System, LogLevel::Info, logstr);
    }
}