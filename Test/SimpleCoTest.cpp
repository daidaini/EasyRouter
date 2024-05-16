#include <iostream>
#include <boost/coroutine2/all.hpp>
#include <string>

using namespace boost::coroutines2;

void producer(coroutine<std::string>::push_type &yield)
{
    for (int i = 0; i < 5; ++i)
    {
        std::string str_i = std::to_string(i) + "haha";
        std::cout
            << "Producing: " << str_i << std::endl;

        yield(str_i); // Yield the current value
        printf("yielding...\n");
    }
}

void consumer(coroutine<std::string>::pull_type &source)
{
    for (const std::string &i : source)
    {
        std::cout << "Consuming: " << i << std::endl;
    }
}

int main()
{
    coroutine<std::string>::pull_type source(producer);
    printf("prepare...\n");
    consumer(source);
    return 0;
}
