#include "RouterServer.h"
#include "RtGlobalResource.h"
#include "PbPkgHandle.h"
#include "t2sdk_interface.h"

using namespace muduo::net;
using namespace std::placeholders;
using namespace pobo;

RouterServer::RouterServer(muduo::net::EventLoop *loop, const muduo::net::InetAddress &addr)
{
    m_TcpServer = std::unique_ptr<TcpServer>(
        new TcpServer(loop, addr, "RouterServer"));
}

RouterServer::~RouterServer()
{
}

void RouterServer::Start()
{
    m_TcpServer->setConnectionCallback(std::bind(&RouterServer::OnConnection, this, ::_1));
    m_TcpServer->setMessageCallback(std::bind(&RouterServer::OnMessage, this, ::_1, ::_2, ::_3));

    // 设置Server的线程池大小
    m_TcpServer->setThreadNum(g_Global.Configer().m_ServerThreadCnt);

    m_TcpServer->start();
}

void RouterServer::OnConnection(const muduo::net::TcpConnectionPtr &conn)
{
    int connId = conn->getConnId();

    if (conn->connected())
    {
        // 创建client
        std::unique_ptr<DstClient> client = std::unique_ptr<DstClient>(new DstClient(connId));
        g_Global.GwClientManager().AddClient(connId, std::move(client));

        g_Global.UserSessions().AddSession(conn);

        AddAuthUser(conn);
    }
    else
    {
        // 先删除缓存
        g_Global.UserSessions().EraseSession(connId);
        // 通知 client断开
        DstClient *client = g_Global.GwClientManager().GetClient(connId);
        if (client != nullptr)
        {
            client->Close();
        }
    }
}

void RouterServer::OnMessage(const muduo::net::TcpConnectionPtr &conn, muduo::net::Buffer *buf, muduo::Timestamp)
{
    DstClient *client = g_Global.GwClientManager().GetClient(conn->getConnId());
    if (client == nullptr)
    {
        return;
    }

    if (!client->IsAuthed())
    {
        return ProcessAuthMessage(conn, buf, client);
    }
    else
    {
        // to do. //测试，什么时候删除需要再研究考虑
        EraseAuthUser(conn->getConnId());
    }

    // to do . test
    //  直接转发到网关

    if (client != nullptr)
    {
        client->SendMsg(buf);
    }
}

void RouterServer::ProcessAuthMessage(const muduo::net::TcpConnectionPtr &conn, muduo::net::Buffer *buf, DstClient *client)
{
    std::vector<pobo::CommMessage> cachedMsgs{};
    // 解析请求
    int buflen = static_cast<int>(buf->readableBytes());
    int leftLen = PoboPkgHandle::SplitPackage(buf->peek(), buflen, cachedMsgs);

    auto connId = conn->getConnId();

    if (leftLen < 0 && leftLen >= MAX_STEP_PACKAGE_SIZE)
    {
        buf->retrieveAll();
        // 存在非法包，不继续处理
        SpdLogger::Instance().WriteLog(LogType::System, LogLevel::Warn, "Unsupported request package",
                                       connId, conn->peerAddress().toIp());
        return;
    }

    if (leftLen > 0)
    {
        SpdLogger::Instance().WriteLog(LogType::System, LogLevel::Debug, "[{}]Package left size[{}:{}]", connId, leftLen, buflen);
    }

    RtAuthUser *authUser = GetAuthUser(connId);
    if (authUser == nullptr)
    {
        return;
    }

    if (cachedMsgs.empty())
    {
        return;
    }

    auto &msg = cachedMsgs.front();

    int result = authUser->ProcessMsg(msg);
    if (result <= 0)
    {
        fmt::print("AuthUser ProcessMsg failed..\n");
    }

    if (result == 1)
    {
        client->SetCommKey(authUser->GetCommKey());
    }
    else if (result == 3)
    {
        client->SetPwdKey(authUser->GetPwdKey());
    }
    else if (result == 6011)
    {
        // to do. test
        client->Create(GwModuleTypeEnum::HST2);

        client->SetLoginRequest(std::string(buf->peek(), buflen - leftLen));

        client->Connect();

        client->ConfirmAuthed();
    }

    buf->retrieve(buflen - leftLen);
}

void RouterServer::AddAuthUser(const muduo::net::TcpConnectionPtr &conn)
{
    std::lock_guard<std::mutex> guard(m_AuthMtx);

    m_AuthUsers.insert(std::make_pair(conn->getConnId(),
                                      std::unique_ptr<RtAuthUser>(new RtAuthUser(conn))));
}

RtAuthUser *RouterServer::GetAuthUser(TcpConnIDType connId)
{
    std::lock_guard<std::mutex> guard(m_AuthMtx);

    auto it = m_AuthUsers.find(connId);
    if (it != m_AuthUsers.end())
    {
        return it->second.get();
    }

    return nullptr;
}

void RouterServer::EraseAuthUser(TcpConnIDType connId)
{
    std::lock_guard<std::mutex> guard(m_AuthMtx);
    m_AuthUsers.erase(connId);
}