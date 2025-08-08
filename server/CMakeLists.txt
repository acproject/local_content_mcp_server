# 服务器端库
add_library(mcp_server_lib STATIC
    src/database.cpp
    src/content_manager.cpp
    src/mcp_server.cpp
    src/http_handler.cpp
    src/config.cpp
    src/file_upload.cpp
    src/llama_client.cpp
)

# 头文件目录
target_include_directories(mcp_server_lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# 链接依赖
target_link_libraries(mcp_server_lib PUBLIC
    nlohmann_json::nlohmann_json
    spdlog::spdlog
    SQLite::SQLite3
    httplib::httplib
)

# 服务器可执行文件
add_executable(mcp_server
    src/main.cpp
)

target_link_libraries(mcp_server PRIVATE
    mcp_server_lib
)

# 安装
install(TARGETS mcp_server
    RUNTIME DESTINATION bin
)