#include <iostream>
#include <utility>
#include <functional>

#include <boost/asio.hpp>
#include <boost/asio/spawn.hpp>

#include <syscall.h>

using boost::asio::ip::tcp;

boost::asio::io_service io_service;

void session(tcp::socket socket, boost::asio::yield_context yield)
{
    try
    {
        char data[1024];
        for (;;)
        {
            std::size_t length = socket.async_read_some(boost::asio::buffer(data), yield);
            printf("thread id = %ld\n", ::syscall(SYS_gettid));
            boost::asio::async_write(socket, boost::asio::buffer(data, length), yield);
        }
    }
    catch (std::exception &e)
    {
        std::cerr << "Exception in session: " << e.what() << "\n";
    }
}

void listener(tcp::acceptor &acceptor, boost::asio::yield_context yield)
{
    for (;;)
    {
        tcp::socket socket(io_service);
        acceptor.async_accept(socket, yield);
        boost::asio::spawn(socket.get_executor(),
                           [&](boost::asio::yield_context yield)
                           {
                               session(std::move(socket), yield);
                           });
    }
}

int main()
{
    try
    {
        tcp::acceptor acceptor(io_service, tcp::endpoint(tcp::v4(), 5555));
        boost::asio::spawn(io_service, [&](boost::asio::yield_context yield)
                           { listener(acceptor, yield); });
        io_service.run();
    }
    catch (std::exception &e)
    {
        std::cerr << "Exception: " << e.what() << "\n";
    }
    return 0;
}
