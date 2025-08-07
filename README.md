# 本地内容管理MCP服务

一个基于C++20开发的本地知识库内容管理MCP（Model Context Protocol）服务，支持插件化架构和Redis存储。

## 功能特性

- 🚀 基于Boost.Asio的高性能异步网络服务
- 🔌 插件化架构，支持动态加载插件
- 💾 Redis存储支持
- 📡 Protocol Buffers消息序列化
- 🧪 完整的单元测试覆盖
- 🔧 跨平台支持（Windows/Linux）

## 系统要求

- C++20兼容编译器
- CMake 3.20+
- vcpkg包管理器
- Redis服务器

## 依赖库

- Boost (system, filesystem, thread)
- nlohmann/json
- spdlog
- hiredis
- Protocol Buffers (可选)
- Google Test (测试)

## 构建说明

### 1. 安装依赖

使用vcpkg安装依赖：
```bash
vcpkg install boost-system boost-filesystem boost-thread nlohmann-json spdlog hiredis protobuf gtest
```

### 2. 配置环境变量

设置VCPKG_ROOT环境变量指向vcpkg安装目录。

### 3. 构建项目

```bash
mkdir build
cd build
cmake ..
cmake --build .
```

### 4. 运行测试

```bash
ctest
```

## 配置

编辑 `resources/config.json` 文件：

```json
{
    "host": "0.0.0.0",
    "port": 5555,
    "redis_host": "localhost",
    "redis_port": 6379
}
```

## 使用方法

1. 启动Redis服务器
2. 运行MCP服务：
   ```bash
   ./build/src/mcp
   ```
3. 服务将在配置的端口上监听连接

## 插件开发

参考 `plugins/echo_plugin.cpp` 示例创建新插件：

```cpp
class MyPlugin : public Plugin {
public:
    void init(Server& server) override {
        server.add_handler("my_command", 
            [](Connection& conn, const std::string& payload) {
                // 处理逻辑
                conn.send("response");
            });
    }
};

extern "C" PluginPtr create_plugin() {
    return std::make_unique<MyPlugin>();
}
```

## 许可证

MIT License - 详见 [LICENSE](LICENSE) 文件。