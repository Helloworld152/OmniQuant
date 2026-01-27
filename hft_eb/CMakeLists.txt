cmake_minimum_required(VERSION 3.10)
project(hft_eb)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 1. 编译插件 A: CTP
add_library(mod_ctp SHARED modules/ctp/ctp_module.cpp)
target_include_directories(mod_ctp PRIVATE include)
# 强制链接 pthread，防止 --as-needed 把它优化掉
target_link_libraries(mod_ctp PRIVATE -Wl,--no-as-needed pthread -Wl,--as-needed)

# 2. 编译插件 B: Strategy
add_library(mod_strategy SHARED modules/strategy/simple_strategy.cpp)
target_include_directories(mod_strategy PRIVATE include)

# 3. 编译插件 C: SimpleTrade
add_library(mod_trade SHARED modules/trade/simple_trade.cpp)
target_include_directories(mod_trade PRIVATE include)

# 4. 编译主程序
add_executable(hft_engine src/main.cpp src/engine.cpp)
target_include_directories(hft_engine PRIVATE include)
# 引入项目现有的 rapidjson
target_include_directories(hft_engine PRIVATE "${CMAKE_SOURCE_DIR}/../gateway_ctp/include") 
target_link_libraries(hft_engine PRIVATE dl pthread)
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -rdynamic")

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
