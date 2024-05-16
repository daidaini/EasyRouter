#include <iostream>
#include <boost/asio.hpp>
#include <boost/coroutine2/all.hpp>
#include <boost/asio/spawn.hpp>
#include <thread>
#include <vector>
#include <fstream> // 引入文件 I/O 头文件

#include <syscall.h>

using boost::asio::ip::tcp;

boost::asio::io_service io_service;

// 从文件中读取内容的协程
void readFromFile(const std::string &filename, std::string &content, boost::asio::yield_context yield)
{
    std::ifstream file(filename);
    if (!file.is_open())
    {
        throw std::runtime_error("Failed to open file");
    }

    std::string line;
    content.clear(); // Clear content before reading
    while (std::getline(file, line))
    {
        content += line + "\n"; // Append line to content
    }
}

void session(tcp::socket socket, boost::asio::yield_context yield)
{
    try
    {
        char data[1024];
        for (;;)
        {
            std::size_t length = socket.async_read_some(boost::asio::buffer(data), yield);
            printf("[%ld]%ld\n", ::syscall(SYS_gettid), length);

            // 从文件中读取内容
            std::string fileContent;
            boost::asio::spawn(
                socket.get_executor(),
                [&](boost::asio::yield_context yield2)
                {
                    readFromFile("example.txt", fileContent, yield2);
                });

            // 将文件内容发送回客户端
            if (!fileContent.empty())
                boost::asio::async_write(socket, boost::asio::buffer(fileContent.data(), fileContent.size()), yield);

            // 再增加一个协程调用方法
            // boost::asio::async_write(socket, boost::asio::buffer(data, length), yield);
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

/*该方案可行
 */

int main()
{
    try
    {
        tcp::acceptor acceptor(io_service, tcp::endpoint(tcp::v4(), 5555));
        printf("Main thread id = %ld\n", ::syscall(SYS_gettid));
        // 创建多个线程来运行 io_context
        std::vector<std::thread> threads;

        boost::asio::spawn(io_service, [&](boost::asio::yield_context yield)
                           { listener(acceptor, yield); });

        for (int i = 0; i < 1; ++i)
        { // 创建 4 个线程
            threads.emplace_back(
                std::thread([]()
                            {
                                printf("curr thread id = %ld\n", ::syscall(SYS_gettid));
                                io_service.run(); }));
        }

        io_service.run();

        getchar();

        // 等待所有线程结束
        for (auto &thread : threads)
        {

            // thread.join();
        }
    }
    catch (std::exception &e)
    {
        std::cerr << "Exception: " << e.what() << "\n";
    }
    return 0;
}
