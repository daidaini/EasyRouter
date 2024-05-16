cmake_minimum_required(VERSION 3.5)

set(CMAKE_CXX_STANDARD 20)

include(../../Common.cmake)
set(PRJ "CoroutienTest")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread -ggdb")

find_package(Boost REQUIRED COMPONENTS coroutine context)
if(BOOST_FOUND)
    message(STATUS "Boost version: ${Boost_VERSION}")
    include_directories(${Boost_INCLUDE_DIRS})
    message("coroutine: " Boost::coroutine)
else()
    message(STATUS "Boost coroutine not found")
endif()

#include_directories(../../GeneralThirdPart/)
#add_package(jsoncpp ../../GeneralThirdPart/jsoncpp)

#add_package(Tools ../../Common/Tools)
#add_package(Logger ../../Common/Logger)

#添加预定义
add_definitions(-DFMT_HEADER_ONLY)
add_definitions(-DSPDLOG_FMT_EXTERNAL)
#add_definitions(-DBOOST_ASIO_HAS_CO_AWAIT)
#add_definitions(-DBOOST_ASIO_HAS_STD_COROUTINE)


add_executable(${PRJ} CoroutineTest.cpp)
target_link_libraries(${PRJ} PRIVATE Boost::coroutine)

#add_executable(C20CoroutineTest C20CoroutineTest.cpp)
#target_link_libraries(C20CoroutineTest PRIVATE Boost::coroutine)

#add_executable(MultiCoroutineTest MultiCoroutineTest.cpp)
#target_link_libraries(MultiCoroutineTest PRIVATE Boost::coroutine Boost::context)

#add_executable(SimpleCoTest SimpleCoTest.cpp)
#target_link_libraries(SimpleCoTest PRIVATE Boost::coroutine)

add_executable(CoReadfile CoReadfile.cpp)
target_link_libraries(CoReadfile PRIVATE Boost::coroutine)

