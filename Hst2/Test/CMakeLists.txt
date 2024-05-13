cmake_minimum_required(VERSION 3.5)

set(CMAKE_CXX_STANDARD 14)

set(curr_path "../../..")

include(${curr_path}/Common.cmake)

project(test_hst2)

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread -ldl -O0 -ggdb")

include_directories(../)
include_directories(${curr_path}/GeneralThirdPart/)
add_package(jsoncpp ${curr_path}/GeneralThirdPart/jsoncpp)

add_package(Tools ${curr_path}/Common/Tools)
add_package(Logger ${curr_path}/Common/Logger)
include_directories(${curr_path}/Common/CommonDefine)
add_package(PBStep ${curr_path}/Common/PBStep)

include_directories(${curr_path}/pb-trade-server/ServiceHST2/T2SDK/Include)

find_library(BOOST_FILESYS_LIB libboost_filesystem.a)
message("BOOST_FILESYS_LIB : " ${BOOST_FILESYS_LIB})

# 添加预定义
add_definitions(-DFMT_HEADER_ONLY)
add_definitions(-DSPDLOG_FMT_EXTERNAL)

add_executable(test_hst2 test.cpp ../T2SyncConnection.cpp ${all_file})
target_link_libraries(test_hst2 PRIVATE libt2sdk.so ${BOOST_FILESYS_LIB})
target_link_directories(test_hst2 PUBLIC ${curr_path}/pb-trade-server/ServiceHST2/T2SDK/linux.x64/lib)
