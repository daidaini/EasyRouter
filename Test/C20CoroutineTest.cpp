#include <iostream>
#include <fstream>
#include <boost/asio.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/use_awaitable.hpp>

using boost::asio::ip::tcp;

boost::asio::io_context io_context;

boost::asio::awaitable<std::string> readFromFile(const std::string &filename)
{
    std::ifstream file(filename);
    if (!file.is_open())
    {
        throw std::runtime_error("Failed to open file");
    }

    co_return std::string((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
}

boost::asio::awaitable<void> session(tcp::socket socket)
{
    try
    {
        char data[1024];
        // 从文件中读取内容
        std::string fileContent = co_await readFromFile("example.txt");

        // 将文件内容发送回客户端
        co_await boost::asio::async_write(socket, boost::asio::buffer(fileContent), boost::asio::use_awaitable);

        for (;;)
        {
            std::size_t length = co_await socket.async_read_some(boost::asio::buffer(data), boost::asio::use_awaitable);
            co_await boost::asio::async_write(socket, boost::asio::buffer(data, length), boost::asio::use_awaitable);
        }
    }
    catch (std::exception &e)
    {
        std::cerr << "Exception in session: " << e.what() << std::endl;
    }
}

boost::asio::awaitable<void> listener(tcp::acceptor &acceptor)
{
    for (;;)
    {
        tcp::socket socket = co_await acceptor.async_accept(boost::asio::use_awaitable);
        boost::asio::co_spawn(
            socket.get_executor(), [&socket]()
            { return session(std::move(socket)); },
            boost::asio::detached);
    }
}

int main()
{
    try
    {
        tcp::acceptor acceptor(io_context, tcp::endpoint(tcp::v4(), 5555));
        boost::asio::co_spawn(
            io_context, [&acceptor]()
            { return listener(acceptor); },
            boost::asio::detached);
        io_context.run();
    }
    catch (std::exception &e)
    {
        std::cerr << "Exception: " << e.what() << std::endl;
    }
    return 0;
}
