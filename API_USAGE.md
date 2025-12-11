# MCP API 使用指南

## 概述

本服务器提供了一个专门为大语言模型设计的 MCP API 端点：`/api/mcp`

## API 端点

**URL**: `POST http://localhost:8080/api/mcp`

**Content-Type**: `application/json`

## 请求格式

```json
{
  "method": "<MCP方法名>",
  "params": {
    // 方法参数
  },
  "id": 1
}
```

## 响应格式

```json
{
  "success": true,
  "result": {
    // 结果数据
  },
  "method": "<请求的方法名>",
  "timestamp": 1754713722
}
```

## 支持的 MCP 方法

### 1. 初始化连接

```bash
curl -X POST "http://localhost:8080/api/mcp" \
  -H "Content-Type: application/json" \
  -d '{
    "method": "initialize",
    "params": {
      "protocolVersion": "2024-11-05",
      "capabilities": {}
    },
    "id": 1
  }'
```

### 2. 获取可用工具列表

```bash
curl -X POST "http://localhost:8080/api/mcp" \
  -H "Content-Type: application/json" \
  -d '{
    "method": "tools/list",
    "params": {},
    "id": 2
  }'
```

### 3. 调用工具

#### 创建内容

```bash
curl -X POST "http://localhost:8080/api/mcp" \
  -H "Content-Type: application/json" \
  -d '{
    "method": "tools/call",
    "params": {
      "name": "create_content",
      "arguments": {
        "title": "测试文档",
        "content": "这是一个测试文档的内容",
        "content_type": "text",
        "tags": "test,document"
      }
    },
    "id": 3
  }'
```

#### 搜索内容

```bash
curl -X POST "http://localhost:8080/api/mcp" \
  -H "Content-Type: application/json" \
  -d '{
    "method": "tools/call",
    "params": {
      "name": "search_content",
      "arguments": {
        "query": "测试",
        "page": 1,
        "page_size": 10
      }
    },
    "id": 4
  }'
```

#### 获取内容

```bash
curl -X POST "http://localhost:8080/api/mcp" \
  -H "Content-Type: application/json" \
  -d '{
    "method": "tools/call",
    "params": {
      "name": "get_content",
      "arguments": {
        "id": 1
      }
    },
    "id": 5
  }'
```

#### 更新内容

```bash
curl -X POST "http://localhost:8080/api/mcp" \
  -H "Content-Type: application/json" \
  -d '{
    "method": "tools/call",
    "params": {
      "name": "update_content",
      "arguments": {
        "id": 1,
        "title": "更新后的测试文档",
        "content": "这是更新后的内容",
        "content_type": "text",
        "tags": "test,updated"
      }
    },
    "id": 6
  }'
```

### 4. 获取资源列表

```bash
curl -X POST "http://localhost:8080/api/mcp" \
  -H "Content-Type: application/json" \
  -d '{
    "method": "resources/list",
    "params": {},
    "id": 6
  }'
```

### 5. 读取资源

```bash
curl -X POST "http://localhost:8080/api/mcp" \
  -H "Content-Type: application/json" \
  -d '{
    "method": "resources/read",
    "params": {
      "uri": "content://1"
    },
    "id": 7
  }'
```

## 错误处理

当请求出错时，响应格式如下：

```json
{
  "success": false,
  "error": {
    "code": -1,
    "message": "错误描述"
  },
  "method": "<请求的方法名>",
  "timestamp": 1754713722
}
```

## 与标准 MCP 端点的区别

- **标准 MCP 端点** (`/mcp`): 完全符合 MCP 协议规范，适用于 MCP 客户端
- **API 端点** (`/api/mcp`): 为大语言模型优化，提供更友好的响应格式，包含成功状态、时间戳等额外信息

## 注意事项

1. 所有请求必须使用 POST 方法
2. Content-Type 必须设置为 `application/json`
3. 请求体必须是有效的 JSON 格式
4. `method` 和 `params` 字段是必需的
5. `id` 字段可选，用于请求跟踪