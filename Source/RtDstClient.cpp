#include "RtDstClient.h"
#include "RtGlobalResource.h"

#include "RtTool.h"
#include "StringFunc.h"

using namespace muduo::net;

DstClient::DstClient(int srcConnId)
{
    m_SrcConnId = srcConnId;

    if (g_Global.Configer().m_RouterAuthType == RouterAuthType::ThirdSysAuth)
    {
        m_IsNeedUpdateLoginRsp = true; // 第三方认证时，走认证获取一些上次登录信息，然后缓存并更新
    }
    else
    {
        m_IsNeedUpdateLoginRsp = false;
    }
}

bool DstClient::Create(ModuleGroupType type)
{
    if (m_TcpClient != nullptr)
    {
        SpdLogger::Instance().WriteLog(LogType::System, LogLevel::Warn, "Create client repeatly");
        return false;
    }

    static std::atomic<size_t> s_ClientIndex = {1};
    InetAddress *addr = g_Global.Configer().DstAddr(type, s_ClientIndex.fetch_add(1));
    EventLoop *loop = g_Global.EvnetLoop(type, s_ClientIndex.load());

    if (loop == nullptr || addr == nullptr)
    {
        SpdLogger::Instance().WriteLog(LogType::System, LogLevel::Warn, "获取配置对应信息失败，检查配置是否准确");
        return false;
    }

    m_Name = fmt::format("S{}_Client_D{}", m_SrcConnId, addr->toIp());
    m_TcpClient = std::unique_ptr<TcpClient>(new TcpClient(loop, *addr, m_Name));

    SpdLogger::Instance().WriteLog(LogType::System, LogLevel::Info, "Creating client for dst addr[{}] to connect", addr->toIpPort());
    if (m_TcpClient == nullptr)
    {
        SpdLogger::Instance().WriteLog(LogType::System, LogLevel::Error, "Create client failed", addr->toIpPort());
        return false;
    }

    m_TcpClient->setConnectionCallback([this](const TcpConnectionPtr &conn)
                                       { this->OnConnect(conn); });

    m_TcpClient->setMessageCallback([this](const TcpConnectionPtr &conn, Buffer *buf, Timestamp time)
                                    { this->OnResponse(conn, buf, time); });

    return true;
}

void DstClient::Connect()
{
    m_TcpClient->disableRetry();

    m_TcpClient->connect();
}

void DstClient::OnConnect(const TcpConnectionPtr &tcpConn)
{
    if (tcpConn->connected())
    {
        SpdLogger::Instance().WriteLog(LogType::System, LogLevel::Info, "New connection : {}", m_Name);
        m_IsConnected.store(true);

        PushKeys();

        SendMsg(m_EncrypedLoginReq);
    }
    else
    {
        SpdLogger::Instance().WriteLog(LogType::System, LogLevel::Info, "Remove connection : {}", m_Name);
        m_IsConnected.store(false);

        g_Global.GwClientManager().EraseClient(m_SrcConnId);

        auto connPtr = g_Global.UserSessions().GetTcpConn(m_SrcConnId);
        if (connPtr != nullptr)
        {
            SpdLogger::Instance().WriteLog(LogType::System, LogLevel::Warn, "Active disconnect user connection[{}]", m_SrcConnId);

            // connPtr->forceCloseWithDelay(0.2);
            connPtr->shutdown();
        }
    }
}

void DstClient::OnResponse(const TcpConnectionPtr &conn, Buffer *buf, muduo::Timestamp time)
{
    auto connPtr = g_Global.UserSessions().GetTcpConn(m_SrcConnId);
    if (connPtr != nullptr)
    {
        if (m_IsNeedUpdateLoginRsp && (!m_LastIp.empty() || !m_LastMac.empty()))
        {
            if (UpdateLoginRspAndSendBack(buf, connPtr))
            {
                m_IsNeedUpdateLoginRsp = false; // 表示已经更新
            }
        }
        else
        {
            connPtr->send(buf);
            buf->retrieveAll();
        }
    }
    else
    {
        buf->retrieveAll();
    }
}

void DstClient::SendMsg(const std::string &msg)
{
    if (m_IsConnected)
    {
        return m_TcpClient->connection()->send(msg.data(), msg.size());
    }

    if (!m_IsAuthed)
    {
        for (int i = 0; i < 10; ++i) // 等1秒
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            if (m_IsConnected)
            {
                return m_TcpClient->connection()->send(msg.data(), msg.size());
            }
        }
    }
    SpdLogger::Instance().WriteLog(LogType::System, LogLevel::Error,
                                   "[{}] Message send failed, dst connection status = {}", m_SrcConnId, m_IsConnected.load());

    // 主动断开连接
    m_TcpClient->disconnect();
}

void DstClient::SendMsg(muduo::net::Buffer *buff)
{
    if (m_IsConnected)
    {
        m_TcpClient->connection()->send(buff);
    }
}

void DstClient::Close()
{
    if (m_IsConnected)
    {
        m_TcpClient->disconnect();
    }
}

void DstClient::SetSrcConnPeerIp(std::string peerIp)
{
    m_SrcPeerIp = std::move(peerIp);
}

void DstClient::SetCommKey(std::string commkey)
{
    m_Commkey = std::move(commkey);
}

void DstClient::SetPwdKey(std::string pwdkey)
{
    m_Pwdkey = std::move(pwdkey);
}

void DstClient::SetLoginRequest(std::string loginReq)
{
    m_EncrypedLoginReq = std::move(loginReq);
}

void DstClient::PushKeys()
{
    std::string pushmsg = fmt::format("Router,{},{},{},", m_Commkey, m_Pwdkey, m_SrcPeerIp);
    SpdLogger::Instance().WriteLog(LogType::System, LogLevel::Debug, "[Pushmsg]{}", pushmsg);
    SendMsg(pushmsg);
}

void DstClient::ConfirmAuthed()
{
    m_IsAuthed = true;

    g_Global.IdleOvertimer().Erase(m_SrcConnId);
}

bool DstClient::IsValid() const
{
    return m_IsAuthed && m_IsConnected;
}

void DstClient::SetLastInfo(std::string lastInfo)
{
    auto data = str::Split(lastInfo, ' ');
    if (data.size() < 2)
    {
        return;
    }
    m_LastIp = data.at(0);
    m_LastMac = data.at(1);
}

bool DstClient::UpdateLoginRspAndSendBack(Buffer *buff, const muduo::net::TcpConnectionPtr &srcConn)
{
    if (buff == nullptr)
    {
        return false;
    }

    // 1. 根据commkey生产aes key
    AES_KEY encryptKey{};
    AES_KEY decryptKey{};
    AES_set_encrypt_key((u_char *)m_Commkey.data(), 256, &encryptKey);
    AES_set_decrypt_key((u_char *)m_Commkey.data(), 256, &decryptKey);

    // 2. 解密
    std::vector<pobo::CommMessage> cachedMsgs{};
    int leftLen = pobo::PoboPkgHandle::SplitPackage(buff->peek(), buff->readableBytes(), cachedMsgs);
    bool isSuccess = false;
    do
    {
        if (cachedMsgs.empty())
        {
            SpdLogger::Instance().WriteLog(LogType::System, LogLevel::Warn, "网关返回的应答包解析为空");
            break;
        }

        auto rspMsg = HDGwRouter::UnLoadPackageDefault(cachedMsgs.front(), decryptKey);
        if (rspMsg.empty())
        {
            SpdLogger::Instance().WriteLog(LogType::System, LogLevel::Warn, "卸载网关返回的应答包失败");
            break;
        }

        // 3. 更新包数据
        GatePBStep stepLoginRsp;
        stepLoginRsp.SetPackage(rspMsg);
        if (stepLoginRsp.FunctionId() == 6011)
        {
            assert(stepLoginRsp.SetFieldValue(STEP_LAST_LOGIN_IP, m_LastIp.data()));
            assert(stepLoginRsp.SetFieldValue(STEP_LAST_LOGIN_MAC, m_LastMac.data()));
            m_LastIp.clear();
            m_LastMac.clear();
        }

        std::string loginRsp = stepLoginRsp.ToString();
        // 4. 重新加载
        const char *src = loginRsp.data();
        int srclen = loginRsp.size();

        char zipedmsg[s_MaxPackgeSize];
        int zipedlen = 0;
        if (!pobo::PoboPkgHandle::CompressToZip(zipedmsg, zipedlen, src, srclen))
        {
            break;
        }

        // 加密
        u_char pkgedmsg[s_MaxPackgeSize];
        int len = pobo::PoboPkgHandle::EncryptPackage(pkgedmsg, (u_char *)zipedmsg, zipedlen, encryptKey);

        srcConn->send(pkgedmsg, len);
        isSuccess = true;
        break;
    } while (false);

    buff->retrieve(buff->readableBytes() - leftLen);
    return isSuccess;
}