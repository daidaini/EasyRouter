#include "RtDstClient.h"
#include "RtGlobalResource.h"

using namespace muduo::net;

DstClient::DstClient(int srcConnId)
{
    m_SrcConnId = srcConnId;
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
        connPtr->send(buf);
    }
    buf->retrieveAll();
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
}

bool DstClient::IsValid() const
{
    return m_IsAuthed && m_IsConnected;
}