cmake_minimum_required(VERSION 3.20)

# ---- 项目 & 编译选项 -----------------------------------------------------------
project(mcp
    VERSION 1.0
    LANGUAGE CXX
)

set(CMAKE_CXX_STANDARD 20)               # 代码使用 C++20
set(CMAKE_CXX_STANDARD REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Position-Independent Code (让插件能被链接为动态库)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# 开关： 可以根据需要关闭
option(BUILD_WITH_PROTOBUF   "Build with Protocol Buffers support" ON)
option(BUILD_WITH_PROMETHEUS "Build with prometheus-cpp support" OFF)
option(BUILD_TESTS           "Build unit tests"                  ON)

# ---- 查找第三方依赖  -----------------------------------------------------------
find_package(Boost REQUIRED COMPONENTS system filesystem thread)
find_package(hiredis REQUIRED)
find_package(nlohmann_json 3.2.0 REQUIRED)                  # 》= v3.2.0
find_package(spdlog REQUIRED)

if(BUILD_WITH_PROTOBUF)
    find_package(Protobuf REQUIRED)
endif()

if(BUILD_TESTS)
    enable_testing()
    find_package(GTest REQUIRED)
endif()

# ---- 子目录  -----------------------------------------------------------
add_subdirectory(src)                       # 核心代码 + 主程序
add_subdirectory(plugins)                   # 共享库 （插件）

if(BUILD_TESTS)
    add_subdirectory(tests)
endif()
