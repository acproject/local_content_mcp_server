cmake_minimum_required(VERSION 3.20)

# 项目配置
project(local_content_mcp
    LANGUAGES CXX
    VERSION 1.0.0
    DESCRIPTION "Local Content Management MCP Server"
)

# C++标准
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 编译选项
if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# 构建选项
option(BUILD_TESTS "Build unit tests" ON)
option(BUILD_CLIENT "Build client application" ON)

# vcpkg工具链
if(NOT DEFINED CMAKE_TOOLCHAIN_FILE AND EXISTS "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake")
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake")
endif()

# 使用FetchContent获取第三方库
include(FetchContent)

# nlohmann/json
FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
)
FetchContent_MakeAvailable(nlohmann_json)

# spdlog
FetchContent_Declare(
    spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG v1.12.0
)
FetchContent_MakeAvailable(spdlog)

# httplib
FetchContent_Declare(
    httplib
    GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
    GIT_TAG v0.14.3
)
FetchContent_MakeAvailable(httplib)

# SQLite3
find_package(SQLite3 REQUIRED)
if(NOT SQLite3_FOUND)
    message(FATAL_ERROR "SQLite3 not found")
endif()

if(BUILD_TESTS)
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.14.0
    )
    FetchContent_MakeAvailable(googletest)
    enable_testing()
endif()

# 子目录
add_subdirectory(server)
if(BUILD_CLIENT)
    add_subdirectory(client)
endif()

# 设置包含目录
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/server/include
    ${CMAKE_CURRENT_SOURCE_DIR}/client/include
    ${CMAKE_CURRENT_SOURCE_DIR}/shared/include
)

# 创建必要的目录
file(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/shared/include)
file(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/config)
file(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/scripts)
file(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/data)
file(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/logs)

# 安装配置
install(DIRECTORY config/ DESTINATION etc/local_content_mcp_server
        FILES_MATCHING PATTERN "*.json")

install(DIRECTORY scripts/ DESTINATION bin
        FILES_MATCHING PATTERN "*.sh"
        PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
                   GROUP_READ GROUP_EXECUTE
                   WORLD_READ WORLD_EXECUTE)

# 打包配置
set(CPACK_PACKAGE_NAME "local-content-mcp-server")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Local Content Management MCP Server")
set(CPACK_PACKAGE_VENDOR "Local Content MCP")
set(CPACK_PACKAGE_CONTACT "admin@localhost")
set(CPACK_GENERATOR "TGZ;ZIP")

if(APPLE)
    set(CPACK_GENERATOR "${CPACK_GENERATOR};DragNDrop")
elseif(UNIX)
    set(CPACK_GENERATOR "${CPACK_GENERATOR};DEB;RPM")
elseif(WIN32)
    set(CPACK_GENERATOR "${CPACK_GENERATOR};NSIS")
endif()

include(CPack)

# 显示构建信息
message(STATUS "")
message(STATUS "Local Content MCP Server Configuration:")
message(STATUS "  Version: ${PROJECT_VERSION}")
message(STATUS "  Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  C++ standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "  Install prefix: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "  SQLite3 found: ${SQLite3_FOUND}")
message(STATUS "")