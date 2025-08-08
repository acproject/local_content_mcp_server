# 客户端CMakeLists.txt
cmake_minimum_required(VERSION 3.16)

# 设置客户端库
add_library(mcp_client_lib STATIC
    src/mcp_client.cpp
    src/http_client.cpp
    src/content_client.cpp
    ${CMAKE_SOURCE_DIR}/server/src/database.cpp
)

# 设置头文件目录
target_include_directories(mcp_client_lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/server/include  # 共享一些数据结构
)

# 链接依赖
target_link_libraries(mcp_client_lib
    nlohmann_json::nlohmann_json
    spdlog::spdlog
    httplib::httplib
    SQLite::SQLite3
)

# 设置编译选项
target_compile_features(mcp_client_lib PUBLIC cxx_std_20)
target_compile_options(mcp_client_lib PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall -Wextra -Wpedantic>
    $<$<CXX_COMPILER_ID:MSVC>:/W4>
)

# 客户端可执行文件
add_executable(mcp_client
    src/main.cpp
)

target_link_libraries(mcp_client
    mcp_client_lib
)

# 安装目标
install(TARGETS mcp_client
    RUNTIME DESTINATION bin
)

install(TARGETS mcp_client_lib
    ARCHIVE DESTINATION lib
)

install(DIRECTORY include/
    DESTINATION include/mcp_client
    FILES_MATCHING PATTERN "*.hpp"
)