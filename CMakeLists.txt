cmake_minimum_required(VERSION 3.5)

set(CMAKE_CXX_STANDARD 14)

include(./Common.cmake)
set(PRJ "EasyRouter")

project(${PRJ})

if(CMAKE_BUILD_TYPE)
else()
    SET(CMAKE_BUILD_TYPE debug) # 默认debug
endif()

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread -ldl")
SET(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O0 -ggdb -pipe")
SET(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -O3 -w")

SET(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../run/EasyRouter)

include_directories(../GeneralThirdPart/)
add_package(jsoncpp ../GeneralThirdPart/jsoncpp)
add_package(MuduoBase ../GeneralThirdPart/muduo/base)
add_package(MuduoNet ../GeneralThirdPart/muduo/net)

add_package(Source ./Source)
add_package(Tools ../Common/Tools)
add_package(Logger ../Common/Logger)
add_package(Define ../Common/CommonDefine)
add_package(PBStep ../Common/PBStep)

add_package(ServerData ./ServerData)
add_package(HST2Sync ./Hst2/)

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

find_package(Python COMPONENTS Interpreter Development)

if(Python_FOUND)
    message("PYTHON_INCLUDE_DIR = " ${Python_INCLUDE_DIRS})
    message("Python_LIBRARIES = " ${Python_LIBRARIES})
    include_directories(${Python_INCLUDE_DIRS})
else()
    message(STATUS "CPython not found")
endif()

SET(THIRD_LIBRARY_PATH ${THIRD_LIBRARY_PATH} ./Hst2)

# 添加预定义
add_definitions(-DFMT_HEADER_ONLY)
add_definitions(-DSPDLOG_FMT_EXTERNAL)

if(${CMAKE_BUILD_TYPE} STREQUAL "release")
	add_definitions(-DNDEBUG)
endif()

add_executable(${PRJ} ${all_file})

target_link_libraries(${PRJ} PRIVATE ${BOOST_FILESYS_LIB} ${OPENSSL_LIBRARIES} ${Z_LIB} ${Python_LIBRARIES} libt2sdk.so)

target_link_directories(${PRJ} PUBLIC ${THIRD_LIBRARY_PATH})
