cmake_minimum_required(VERSION 3.5)

set(CMAKE_CXX_STANDARD 14)

include(../Common.cmake)
set(PRJ "HDGwRouter")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread -ggdb") 

include_directories(../GeneralThirdPart/)
add_package(jsoncpp ../GeneralThirdPart/jsoncpp)
add_package(MuduoBase ../GeneralThirdPart/muduo/base)
add_package(MuduoNet ../GeneralThirdPart/muduo/net)

add_package(Source ./Source)
add_package(Tools ../Common/Tools)
add_package(Logger ../Common/Logger)

find_library(BOOST_FILESYS_LIB libboost_filesystem.a)
message("BOOST_FILESYS_LIB : " ${BOOST_FILESYS_LIB})

# 添加预定义
add_definitions(-DFMT_HEADER_ONLY)
add_definitions(-DSPDLOG_FMT_EXTERNAL)

add_executable(${PRJ} ${all_file})

target_link_libraries(${PRJ} PRIVATE ${BOOST_FILESYS_LIB})
