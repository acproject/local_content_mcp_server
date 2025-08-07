# -----------------------------------------------------------------
# 1) 生成.pb 代码 （如果使用 protobuf）
# -----------------------------------------------------------------
if(BUILD_WITH_PROTOBUF)
    protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS proto/message.proto)
    list(APPEND CORE_SRCS ${PROTO_SRCS} ${PROTO_HDRS})
endif()

# -----------------------------------------------------------------
# 2) 核心静态库（core） -- 可以把需要复用的类放在这里
# -----------------------------------------------------------------

add_library(core STATIC
    config.cpp
    server.cpp
    handler.cpp
    storage.cpp
    plugin.cpp
    ${CORE_SRCS}              # 可能为空 （没有protobuf）
)

# 头文件路径
target_include_directories(core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

# 链接依赖
target_link_libraries(core PUBLIC
    Boost::system
    Boost::filesystem
    Boost::thread
    hiredis::hiredis
    nlohmann_json::nlohmann_json
    spdlog::spdlog
)

# 如果要使用 protobuf
if(BUILD_WITH_PROTOBUF)
    target_link_libraries(core PUBLIC protobuf::libprotobuf)
endif()

# 如果使用 Prometheus-cpp
if(BUILD_WITH_PROMETHEUS)
    target_link_libraries(core PUBLIC prometheus-cpp::pullgateway)
endif()

# 编译选项
target_compile_options(core PRIVATE
    -Wall
    -Wextra
    -Werror
)

# -----------------------------------------------------------------
# 3) 主程序
# -----------------------------------------------------------------
add_executable(mcp main.cpp)
target_link_libraries(mcp PRIVATE core)


# -----------------------------------------------------------------
# 4) 安装规则（可以按需要修改）
# -----------------------------------------------------------------
install(TARGETS mcp
    RUNTIME DESTINATION bin
)

install(DIRECTORY resources DESTINATION share/mcp)