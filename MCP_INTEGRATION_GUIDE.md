# MCP 集成指南

## 概述

本指南将帮助你将本地内容管理服务器集成到支持 MCP (Model Context Protocol) 的大语言模型中，如 Claude Desktop、Cursor 等。

## 什么是 MCP？

MCP (Model Context Protocol) 是一个开放标准，允许大语言模型安全地连接到外部数据源和工具。通过 MCP，你可以让 AI 助手访问你的本地内容、数据库、API 等。

## 集成步骤

### 1. 安装依赖

首先安装 Node.js 依赖：

```bash
npm install
```

或者手动安装：

```bash
npm install @modelcontextprotocol/sdk node-fetch
```

### 2. 启动本地服务器

确保你的本地内容服务器正在运行：

```bash
# 编译服务器
make build

# 启动服务器
./build/server/mcp_server ./resources/config.json
```

服务器将在 `http://localhost:8080` 上运行。

### 3. 配置 MCP 客户端

#### 对于 Claude Desktop

1. 找到 Claude Desktop 的配置文件：
   - **macOS**: `~/Library/Application Support/Claude/claude_desktop_config.json`
   - **Windows**: `%APPDATA%\Claude\claude_desktop_config.json`

2. 将以下配置添加到配置文件中：

```json
{
  "mcpServers": {
    "local-content-server": {
      "command": "node",
      "args": [
        "~/local_content_mcp_server/web/mcp_bridge.js"
      ],
      "env": {
        "MCP_SERVER_URL": "http://localhost:8080"
      }
    }
  }
}
```

**注意**: 请将路径 `~/local_content_mcp_server/web/mcp_bridge.js` 替换为你的实际项目路径。

#### 对于其他 MCP 客户端

使用项目根目录下的 `mcp_config.json` 文件作为参考，根据你的 MCP 客户端文档进行相应配置。

### 4. 重启 MCP 客户端

配置完成后，重启你的 MCP 客户端（如 Claude Desktop）以加载新的服务器配置。

### 5. 验证集成

在 MCP 客户端中，你应该能够看到以下可用工具：

- **create_content**: 创建新内容
- **search_content**: 搜索内容
- **get_content**: 获取特定内容
- **list_content**: 列出所有内容
- **get_tags**: 获取所有标签
- **get_statistics**: 获取统计信息

## 可用工具详解

### 1. create_content
创建新的内容项。

**参数**:
- `title` (必需): 内容标题
- `content` (必需): 内容正文
- `content_type` (可选): 内容类型（如 "document", "note"）
- `tags` (可选): 逗号分隔的标签

### 2. search_content
在内容库中搜索。

**参数**:
- `query` (必需): 搜索查询
- `page` (可选): 页码（默认: 1）
- `page_size` (可选): 每页结果数（默认: 20）

### 3. get_content
根据 ID 获取特定内容。

**参数**:
- `id` (必需): 内容 ID

### 4. list_content
列出所有内容（分页）。

**参数**:
- `page` (可选): 页码（默认: 1）
- `page_size` (可选): 每页结果数（默认: 20）

### 5. get_tags
获取所有可用标签。

**参数**: 无

### 6. get_statistics
获取内容统计信息。

**参数**: 无

## 使用示例

一旦集成完成，你可以在 MCP 客户端中这样使用：

```
用户: "帮我搜索关于 'Python' 的内容"
AI: 我来为你搜索关于 Python 的内容...
[AI 会自动调用 search_content 工具]

用户: "创建一个新的笔记，标题是 '今日学习'，内容是 '学习了 MCP 协议的使用'"
AI: 我来为你创建这个笔记...
[AI 会自动调用 create_content 工具]
```

## 故障排除

### 1. 工具未显示
- 确保本地服务器正在运行
- 检查 MCP 配置文件路径是否正确
- 重启 MCP 客户端

### 2. 连接错误
- 确认服务器 URL 配置正确（默认: http://localhost:8080）
- 检查防火墙设置
- 查看服务器日志

### 3. Node.js 依赖问题
- 确保安装了所需的 npm 包
- 检查 Node.js 版本（推荐 v16+）

## 高级配置

### 自定义服务器 URL

如果你的服务器运行在不同的端口或主机上，可以修改环境变量：

```json
{
  "env": {
    "MCP_SERVER_URL": "http://your-server:port"
  }
}
```

### 添加认证

如果你的服务器需要认证，可以在 `mcp_bridge.js` 中添加认证头：

```javascript
headers: {
  'Content-Type': 'application/json',
  'Authorization': 'Bearer your-token'
}
```

## 安全注意事项

1. **本地访问**: 默认配置只允许本地访问，这是最安全的方式
2. **网络访问**: 如果需要远程访问，请确保使用 HTTPS 和适当的认证
3. **数据隐私**: MCP 桥接器只转发请求，不存储任何数据

## 支持的 MCP 客户端

- Claude Desktop
- Cursor IDE
- 其他支持 MCP 协议的应用

## 更多信息

- [MCP 官方文档](https://modelcontextprotocol.io/)
- [Claude Desktop MCP 指南](https://docs.anthropic.com/claude/docs/mcp)
- 项目 API 文档: `API_USAGE.md`

## 贡献

如果你发现问题或有改进建议，请提交 Issue 或 Pull Request。