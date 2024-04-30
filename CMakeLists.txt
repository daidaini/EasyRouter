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
add_package(Define ../Common/CommonDefine)
add_package(PBStep ../Common/PBStep)

include_directories(../pb-trade-server/PBFrame/DataDefine)
add_match_package(StaticData ../pb-trade-server/PBFrame/Source/ PbStaticData.*)

include_directories(../pb-trade-server/ServiceHST2/T2SDK/Include)
add_package(HST2Sync ./Sync/)

find_library(BOOST_FILESYS_LIB libboost_filesystem.a)
message("BOOST_FILESYS_LIB : " ${BOOST_FILESYS_LIB})

set(OPENSSL_ROOT_DIR /usr/local/lib64)
message(STATUS "OPENSSL_ROOT_DIR = " ${OPENSSL_ROOT_DIR})
find_package(OpenSSL REQUIRED)

if(OPENSSL_FOUND)
    message(STATUS "OPENSSL_LIBRARIES: " ${OPENSSL_LIBRARIES})
else()
    message(FATAL_ERROR "OpenSSL NOT FOUND...")
endif()

find_library(Z_LIB libz.a)
if(Z_LIB STREQUAL "Z_LIB-NOTFOUND")
    message(FATAL_ERROR "Not find zlib...")
else()
    message(STATUS "ZLIB: " ${Z_LIB})
endif()

SET(THIRD_LIBRARY_PATH ${THIRD_LIBRARY_PATH} ../pb-trade-server/ServiceHST2/T2SDK/linux.x64/lib)

# 添加预定义
add_definitions(-DFMT_HEADER_ONLY)
add_definitions(-DSPDLOG_FMT_EXTERNAL)

add_executable(${PRJ} ${all_file})

target_link_libraries(${PRJ} PRIVATE ${BOOST_FILESYS_LIB} ${OPENSSL_LIBRARIES} ${Z_LIB} libt2sdk.so)

target_link_directories(${PRJ} PUBLIC ${THIRD_LIBRARY_PATH})

