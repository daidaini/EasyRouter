#include "RtDstClient.h"
#include "RtGlobalResource.h"

using namespace muduo::net;

DstClient::DstClient(int srcConnId)
{
    m_SrcConnId = srcConnId;
}

void DstClient::Create(GwModuleTypeEnum moduleType)
{
    if (m_Client != nullptr)
    {
        fmt::print("Create client repeatly\n");
        return;
    }

    static std::atomic<size_t> s_ClientIndex = {1};
    m_Name = fmt::format("{}_client_{}", s_ClientIndex.fetch_add(1), m_SrcConnId);

    InetAddress *addr = g_Global.Configer().DstAddr(GwModuleTypeEnum::HST2, s_ClientIndex.load());
    EventLoop *loop = g_Global.EvnetLoop(GwModuleTypeEnum::HST2, s_ClientIndex.load());

    m_Client = std::unique_ptr<TcpClient>(new TcpClient(loop, *addr, m_Name));

    m_Client->setConnectionCallback([this](const TcpConnectionPtr &conn)
                                    { this->OnConnect(conn); });

    m_Client->setMessageCallback([this](const TcpConnectionPtr &conn, Buffer *buf, Timestamp time)
                                 { this->OnResponse(conn, buf, time); });
}

void DstClient::Connect()
{
    m_Client->connect();
}

void DstClient::OnConnect(const TcpConnectionPtr &tcpConn)
{
    auto connName = tcpConn->name();
    if (tcpConn->connected())
    {
        fmt::print(fg(fmt::color::sea_green), "New connection : {}[{}]\n", connName, m_SrcConnId);
        m_IsConnected.store(true);

        PushKeys();

        SendMsg(m_EncrypedLoginReq);
    }
    else
    {
        fmt::print(fg(fmt::color::orange_red), "Remove connection : {}[{}]\n", connName, m_SrcConnId);
        m_IsConnected.store(false);
        // 通知src断开

        auto connPtr = g_Global.UserSessions().GetTcpConn(m_SrcConnId);
        if (connPtr != nullptr)
        {
            connPtr->forceCloseWithDelay(1.0);
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
        return m_Client->connection()->send(msg.data(), msg.size());
    }

    if (!m_IsAuthed)
    {
        for (int i = 0; i < 10; ++i) // 等1秒
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            if (m_IsConnected)
            {
                return m_Client->connection()->send(msg.data(), msg.size());
            }
        }
    }
    fmt::print(fg(fmt::color::pale_violet_red), "[{}] msg send failed..\n", m_SrcConnId);
}

void DstClient::SendMsg(muduo::net::Buffer *buff)
{
    if (m_IsConnected)
    {
        m_Client->connection()->send(buff);
    }
}

void DstClient::Close()
{
    m_Client->disconnect();
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

bool DstClient::IsAuthed() const
{
    return m_IsAuthed;
}