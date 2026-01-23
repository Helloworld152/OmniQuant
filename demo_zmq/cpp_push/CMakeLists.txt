cmake_minimum_required(VERSION 3.10)
project(cpp_push_demo)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# 1. Find Dependencies
find_package(PkgConfig REQUIRED)
pkg_check_modules(ZMQ REQUIRED libzmq)

# 2. Find Protobuf (System Version)
# Explicitly force the system compiler to avoid version mismatch with Anaconda in PATH
set(Protobuf_PROTOC_EXECUTABLE "/usr/bin/protoc")
find_package(Protobuf REQUIRED)

message(STATUS "Using Protobuf Compiler: ${Protobuf_PROTOC_EXECUTABLE}")
message(STATUS "Using Protobuf Library: ${Protobuf_LIBRARIES}")

# 3. Find cppzmq
find_package(cppzmq QUIET)

# 4. Include Directories
include_directories(${ZMQ_INCLUDE_DIRS})
include_directories(${Protobuf_INCLUDE_DIRS})
include_directories(${CMAKE_CURRENT_BINARY_DIR})

# 5. Generate Protobuf Sources
set(PROTO_DIR ${CMAKE_SOURCE_DIR}/../../protocol)
set(PROTO_FILE ${PROTO_DIR}/omni.proto)
set(PROTO_SRC ${CMAKE_CURRENT_BINARY_DIR}/omni.pb.cc)
set(PROTO_HDR ${CMAKE_CURRENT_BINARY_DIR}/omni.pb.h)

add_custom_command(
    OUTPUT ${PROTO_SRC} ${PROTO_HDR}
    COMMAND ${Protobuf_PROTOC_EXECUTABLE}
    ARGS --cpp_out=${CMAKE_CURRENT_BINARY_DIR}
         -I ${PROTO_DIR}
         ${PROTO_FILE}
    DEPENDS ${PROTO_FILE}
    COMMENT "Generating C++ code from protocol buffer"
    VERBATIM
)

# 6. Build Target
add_executable(cpp_push src/main.cpp ${PROTO_SRC} ${PROTO_HDR})

# Explicitly add binary dir to include path
target_include_directories(cpp_push PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

target_link_libraries(cpp_push ${ZMQ_LIBRARIES} ${Protobuf_LIBRARIES})

if(cppzmq_FOUND)
    target_link_libraries(cpp_push cppzmq)
endif()