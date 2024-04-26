#include "RouterServer.h"
#include "RtGlobalResource.h"
#include "RtDstClient.h"

using namespace muduo::net;
using namespace std::placeholders;

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
        InetAddress *addr = g_Global.Configer().DstAddr(GwModuleTypeEnum::HST2, 0);
        EventLoop *loop = g_Global.EvnetLoop(GwModuleTypeEnum::HST2, 0);

        std::unique_ptr<DstClient> client = std::unique_ptr<DstClient>(new DstClient(loop, addr, "client_hst2_0", connId));
        client->Start();

        g_Global.GwClientManager().AddClient(conn->getConnId(), std::move(client));
        g_Global.UserSessions().AddSession(conn);
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
    // to do . test
    //  直接转发到网关

    DstClient *client = g_Global.GwClientManager().GetClient(conn->getConnId());

    if (client != nullptr)
    {
        client->SendMsg(buf);
    }
}
