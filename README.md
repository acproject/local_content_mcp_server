# Local Content MCP Server

一个基于 MCP (Model Context Protocol) 的本地内容管理服务器，提供内容的创建、存储、搜索和管理功能。

## 特性

- 🚀 **高性能**: 使用 C++ 开发，支持多线程处理
- 💾 **SQLite 存储**: 使用嵌入式 SQLite 数据库，支持全文搜索
- 🔌 **MCP 协议**: 完整支持 MCP 协议规范
- 🌐 **REST API**: 同时提供 RESTful API 接口
- 🏷️ **标签系统**: 支持内容标签分类和过滤
- 🔍 **全文搜索**: 内置 FTS5 全文搜索引擎
- 📊 **统计分析**: 提供内容统计和分析功能
- 🛡️ **安全可靠**: 支持认证、限流等安全特性
- 📱 **客户端工具**: 提供命令行客户端和库

## 项目结构

```
local_content_mcp_server/
├── server/                 # 服务器端代码
│   ├── include/           # 头文件
│   ├── src/              # 源文件
│   └── CMakeLists.txt    # 服务器构建配置
├── client/                # 客户端代码
│   ├── include/          # 头文件
│   ├── src/              # 源文件
│   └── CMakeLists.txt    # 客户端构建配置
├── config/               # 配置文件
│   ├── server.json       # 服务器配置
│   └── client.json       # 客户端配置
├── scripts/              # 脚本文件
│   ├── start_server.sh   # 服务器启动脚本
│   └── test_client.sh    # 客户端测试脚本
├── data/                 # 数据目录
├── logs/                 # 日志目录
└── CMakeLists.txt        # 根构建配置
```

## 快速开始

### 系统要求

- C++17 或更高版本编译器
- CMake 3.16+
- SQLite3 开发库
- Git

### 安装依赖

**macOS:**
```bash
brew install cmake sqlite3
```

**Ubuntu/Debian:**
```bash
sudo apt update
sudo apt install cmake build-essential libsqlite3-dev
```

**CentOS/RHEL:**
```bash
sudo yum install cmake gcc-c++ sqlite-devel
```

### 构建和运行

1. **克隆项目**
```bash
git clone <repository-url>
cd local_content_mcp_server
```

2. **构建项目**
```bash
# 使用脚本构建（推荐）
./scripts/start_server.sh build

# 或手动构建
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```

3. **启动服务器**
```bash
# 使用脚本启动（推荐）
./scripts/start_server.sh start

# 或手动启动
./build/server/mcp_server --config config/server.json
```

4. **测试客户端**
```bash
# 运行基本测试
./scripts/test_client.sh basic

# 运行所有测试
./scripts/test_client.sh all

# 交互式模式
./scripts/test_client.sh interactive
```

## 使用指南

### 服务器管理

```bash
# 启动服务器
./scripts/start_server.sh start

# 停止服务器
./scripts/start_server.sh stop

# 重启服务器
./scripts/start_server.sh restart

# 查看状态
./scripts/start_server.sh status

# 查看日志
./scripts/start_server.sh logs
```

### 客户端使用

**基本命令:**
```bash
# 创建内容
./build/client/mcp_client create "标题" "内容" tag1 tag2

# 获取内容
./build/client/mcp_client get 123

# 搜索内容
./build/client/mcp_client search "关键词" tag1

# 列出内容
./build/client/mcp_client list 1 10

# 获取标签
./build/client/mcp_client tags

# 获取统计
./build/client/mcp_client stats
```

**使用 REST API:**
```bash
# 添加 --rest 参数使用 REST API
./build/client/mcp_client --rest create "标题" "内容"
```

### API 接口

**MCP 协议端点:**
- `POST /mcp` - MCP 协议入口

**REST API 端点:**
- `GET /api/content` - 列出内容
- `POST /api/content` - 创建内容
- `GET /api/content/{id}` - 获取内容
- `PUT /api/content/{id}` - 更新内容
- `DELETE /api/content/{id}` - 删除内容
- `GET /api/content/search` - 搜索内容
- `GET /api/tags` - 获取标签
- `GET /api/statistics` - 获取统计

**系统端点:**
- `GET /health` - 健康检查
- `GET /info` - 服务器信息

### 配置说明

**服务器配置 (config/server.json):**
```json
{
  "server": {
    "host": "localhost",
    "port": 8086,
    "threads": 4
  },
  "database": {
    "path": "./data/content.db",
    "enable_wal": true
  },
  "logging": {
    "level": "info",
    "file_path": "./logs/server.log"
  }
}
```

**客户端配置 (config/client.json):**
```json
{
  "mcp_client": {
    "server_url": "http://localhost:8086/mcp",
    "timeout_ms": 30000
  },
  "content_client": {
    "preferred_protocol": "mcp",
    "enable_cache": true
  }
}
```

## 开发指南

### 项目架构

**服务器端:**
- `MCPServer`: MCP 协议处理
- `HttpHandler`: HTTP 服务器和路由
- `ContentManager`: 内容管理业务逻辑
- `Database`: SQLite 数据库操作
- `Config`: 配置管理

**客户端:**
- `MCPClient`: MCP 协议客户端
- `HttpClient`: HTTP 客户端
- `ContentClient`: 内容管理客户端封装

### 添加新功能

1. **添加新的 MCP 工具:**
   - 在 `MCPServer::initialize_tools()` 中注册工具
   - 实现对应的处理函数
   - 更新客户端调用

2. **添加新的 REST API:**
   - 在 `HttpHandler::setup_routes()` 中添加路由
   - 实现处理函数
   - 更新客户端支持

3. **扩展数据模型:**
   - 更新 `ContentItem` 结构
   - 修改数据库表结构
   - 更新相关的序列化/反序列化代码

### 测试

```bash
# 运行单元测试（如果有）
cd build && ctest

# 运行集成测试
./scripts/test_client.sh all

# 性能测试
./scripts/test_client.sh performance
```

### 调试

```bash
# 构建调试版本
cmake .. -DCMAKE_BUILD_TYPE=Debug
make

# 使用 GDB 调试
gdb ./build/server/mcp_server

# 启用详细日志
# 修改配置文件中的 logging.level 为 "debug"
```

## 部署

### Docker 部署

```dockerfile
# Dockerfile 示例
FROM ubuntu:22.04

RUN apt-get update && apt-get install -y \
    cmake build-essential libsqlite3-dev \
    && rm -rf /var/lib/apt/lists/*

COPY . /app
WORKDIR /app

RUN mkdir build && cd build && \
    cmake .. -DCMAKE_BUILD_TYPE=Release && \
    make -j$(nproc)

EXPOSE 8086
CMD ["./build/server/mcp_server", "--config", "config/server.json"]
```

### 系统服务

```ini
# /etc/systemd/system/local-content-mcp.service
[Unit]
Description=Local Content MCP Server
After=network.target

[Service]
Type=simple
User=mcp
WorkingDirectory=/opt/local-content-mcp
ExecStart=/opt/local-content-mcp/build/server/mcp_server --config config/server.json
Restart=always
RestartSec=5

[Install]
WantedBy=multi-user.target
```

## 故障排除

### 常见问题

1. **编译错误:**
   - 检查 C++ 编译器版本
   - 确保安装了所有依赖
   - 清理构建目录重新编译

2. **服务器启动失败:**
   - 检查端口是否被占用
   - 验证配置文件格式
   - 查看日志文件

3. **数据库错误:**
   - 检查数据目录权限
   - 确保 SQLite3 正确安装
   - 删除数据库文件重新初始化

4. **客户端连接失败:**
   - 确认服务器正在运行
   - 检查网络连接
   - 验证服务器地址和端口

### 日志分析

```bash
# 查看服务器日志
tail -f logs/server.log

# 查看错误日志
grep ERROR logs/server.log

# 查看性能日志
grep "response_time" logs/server.log
```

## 贡献

1. Fork 项目
2. 创建特性分支 (`git checkout -b feature/amazing-feature`)
3. 提交更改 (`git commit -m 'Add amazing feature'`)
4. 推送到分支 (`git push origin feature/amazing-feature`)
5. 创建 Pull Request

## 许可证

本项目采用 MIT 许可证 - 查看 [LICENSE](LICENSE) 文件了解详情。

## 更新日志

### v1.0.0
- 初始版本发布
- 支持 MCP 协议
- 提供 REST API
- SQLite 存储后端
- 全文搜索功能
- 命令行客户端

## 联系方式

- 项目主页: [GitHub Repository]
- 问题反馈: [GitHub Issues]
- 文档: [Wiki]

---

**注意**: 这是一个本地内容管理系统，主要用于开发和测试环境。在生产环境中使用时，请确保适当的安全配置。另外在windows下cmake需要加上-DJSON_BuildTests=OFF参数禁止测试第三方的库。可以通过命令像这样 cmake .. -DCMAKE_TOOLCHAIN_FILE=D:\vcpkg\scripts\buildsystems\vcpkg.cmake -G "Visual Studio 17 2022" -DJSON_BuildTests=OFF -DSQLITE_ENABLE_FTS5=ON