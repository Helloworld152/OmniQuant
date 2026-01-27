cmake_minimum_required(VERSION 3.10)
project(omni_gateway_ctp)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_FLAGS_RELEASE "-O3") # 强制开启最高优化

# 1. 查找依赖
find_package(PkgConfig REQUIRED)
pkg_check_modules(ZMQ REQUIRED libzmq)
find_package(cppzmq QUIET)

# 2. 强制使用系统 Protobuf
set(Protobuf_PROTOC_EXECUTABLE "/usr/bin/protoc")
find_package(Protobuf REQUIRED)

# 3. 包含目录
include_directories(${ZMQ_INCLUDE_DIRS})
include_directories(${Protobuf_INCLUDE_DIRS})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# === CTP SDK 配置 ===
set(CTP_SDK_DIR ${CMAKE_CURRENT_SOURCE_DIR}/ctp_sdk)
include_directories(${CTP_SDK_DIR}/include)
link_directories(${CTP_SDK_DIR}/lib)
# ===================

# 4. 生成 Protobuf 源码
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/generated)
set(PROTO_SRC ${CMAKE_CURRENT_BINARY_DIR}/generated/omni.pb.cc)
set(PROTO_HDR ${CMAKE_CURRENT_BINARY_DIR}/generated/omni.pb.h)

if(CMAKE_BUILD_TYPE MATCHES "Release|RelWithDebInfo")
    # 开启本机架构优化 (AVX/AVX2/AVX512)
    add_compile_options(-march=native)
endif()

add_custom_command(
    OUTPUT ${PROTO_SRC} ${PROTO_HDR}
    COMMAND ${Protobuf_PROTOC_EXECUTABLE}
    ARGS --cpp_out=${CMAKE_CURRENT_BINARY_DIR}/generated
         -I ${CMAKE_SOURCE_DIR}/../protocol
         ${CMAKE_SOURCE_DIR}/../protocol/omni.proto
    DEPENDS ${CMAKE_SOURCE_DIR}/../protocol/omni.proto
    COMMENT "Generating C++ code from protocol buffer"
    VERBATIM
)

# 5. 构建 Gateway
# 添加 MdHandler.cpp 和 TraderHandler.cpp 到源文件列表
add_executable(gateway_ctp 
    src/main.cpp 
    src/MdHandler.cpp 
    src/TraderHandler.cpp
    ${PROTO_SRC} 
    ${PROTO_HDR}
)

# 链接 Protobuf, ZMQ 和 CTP 库
# 注意：libthostmduserapi_se 是 Linux 下的标准名
target_link_libraries(gateway_ctp 
    ${Protobuf_LIBRARIES} 
    ${ZMQ_LIBRARIES}
    thostmduserapi_se
    thosttraderapi_se
)

if(cppzmq_FOUND)
    target_link_libraries(gateway_ctp cppzmq)
endif()

target_include_directories(gateway_ctp PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/generated)
