# 每一个插件都以SHARED库形式生成
# 这里以echo_plugin 为例： 如果有多个，只需要再增加一行add_library(...)

add_library(echo_plugin SHARED echo_plugin.cpp)

# 需要访问Plugin 接口的头文件
target_include_directories(echo_plugin PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

# 链接到core（如果插件直接使用到core的实现）
target_link_libraries(echo_plugin PRIVATE core)

# 设置共享库后缀（Linux 为.so ， Windows 为 .dll）
set_target_properties(echo_plugin PROPERTIES
    PREFIX ""
    if(WIN32)
        SUFFIX ".dll"
    elseif(APPLE)
        SUFFIX ".dylib"
    else()
        SUFFIX ".so"
    endif() 
)

# 安装插件
install(FILES $<TARGET_FILE:echo_plugin>
    DESTINATION lib/plugins
)