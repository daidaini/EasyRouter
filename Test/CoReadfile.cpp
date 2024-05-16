#include <iostream>
#include <boost/asio.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/bind/bind.hpp>
#include <fstream>
#include <string>

using namespace boost::asio;

void readFile(const std::string &filename, std::string &content, io_context &io_ctx, yield_context yield)
{
    std::ifstream file(filename);
    if (!file.is_open())
    {
        std::cerr << "Failed to open file: " << filename << std::endl;
        return;
    }

    std::string line;
    content.clear(); // Clear content before reading
    while (std::getline(file, line))
    {
        content += line + "\n"; // Append line to content
    }
}

int main()
{
    const std::string filename = "example.txt";
    std::string content;
    io_context io_ctx;
    spawn(io_ctx, [&](yield_context yield)
          {
        try {
            readFile(filename, content, io_ctx, yield);
        } catch (std::exception& e) {
            std::cerr << "Exception: " << e.what() << std::endl;
        } });

    io_ctx.run();

    // Output the content to verify
    std::cout << "Content read from file:\n"
              << content << std::endl;

    return 0;
}
