#include "RtDstClient.h"
#include "RtGlobalResource.h"

using namespace muduo::net;

DstClient::DstClient(int srcConnId)
{
    m_SrcConnId = srcConnId;
}

void DstClient::Create(GwModuleTypeEnum moduleType)
{
    if (m_TcpClient != nullptr)
    {
        fmt::print("Create client repeatly\n");
        return;
    }

    static std::atomic<size_t> s_ClientIndex = {1};
    m_Name = fmt::format("{}_client_{}", s_ClientIndex.fetch_add(1), m_SrcConnId);

    InetAddress *addr = g_Global.Configer().DstAddr(moduleType, s_ClientIndex.load());
    EventLoop *loop = g_Global.EvnetLoop(moduleType, s_ClientIndex.load());

    if (loop == nullptr || addr == nullptr)
    {
        SpdLogger::Instance().WriteLog(LogType::System, LogLevel::Warn, "获取配置对应信息失败，检查配置是否准确");
        return;
    }

    m_TcpClient = std::unique_ptr<TcpClient>(new TcpClient(loop, *addr, m_Name));

    SpdLogger::Instance().WriteLog(LogType::System, LogLevel::Info, "Creating client for dst addr[{}] to connect", addr->toIpPort());

    m_TcpClient->setConnectionCallback([this](const TcpConnectionPtr &conn)
                                       { this->OnConnect(conn); });

    m_TcpClient->setMessageCallback([this](const TcpConnectionPtr &conn, Buffer *buf, Timestamp time)
                                    { this->OnResponse(conn, buf, time); });
}

void DstClient::Connect()
{
    m_TcpClient->disableRetry();

    m_TcpClient->connect();
}

void DstClient::OnConnect(const TcpConnectionPtr &tcpConn)
{
    auto connName = tcpConn->name();
    if (tcpConn->connected())
    {
        SpdLogger::Instance().WriteLog(LogType::System, LogLevel::Info, "New connection : {}[{}]", connName, m_SrcConnId);
        m_IsConnected.store(true);

        PushKeys();

        SendMsg(m_EncrypedLoginReq);
    }
    else
    {
        SpdLogger::Instance().WriteLog(LogType::System, LogLevel::Info, "Remove connection : {}[{}]", connName, m_SrcConnId);
        m_IsConnected.store(false);
        // 通知src断开

        auto connPtr = g_Global.UserSessions().GetTcpConn(m_SrcConnId);
        if (connPtr != nullptr)
        {
            connPtr->forceCloseWithDelay(0.2);
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
    // fmt::print(fg(fmt::color::pale_violet_red), "[{}] msg send failed..\n", m_SrcConnId);
    SpdLogger::Instance().WriteLog(LogType::System, LogLevel::Info, "[{}] msg send failed..", m_SrcConnId);
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
    SendMsg(fmt::format("Router,{},{},", m_Commkey, m_Pwdkey));
}

void DstClient::ConfirmAuthed()
{
    m_IsAuthed = true;
}

bool DstClient::IsValid() const
{
    return m_IsAuthed && m_IsConnected;
}