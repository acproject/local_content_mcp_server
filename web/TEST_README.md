# MCP 功能测试脚本

本目录包含了针对 MCP (Model Context Protocol) 服务器各个功能的测试脚本。

## 测试文件列表

### 单独功能测试

1. **test_get_tags.js** - 测试获取所有标签功能
   - 获取系统中所有标签及其使用次数
   - 验证标签数据格式和统计信息

2. **test_get_statistics.js** - 测试获取统计信息功能
   - 获取内容总数、标签总数等统计数据
   - 验证统计信息的准确性

3. **test_list_content.js** - 测试列出内容功能
   - 分页获取内容列表
   - 验证分页参数和返回数据格式

4. **test_get_content.js** - 测试根据 ID 获取内容功能
   - 根据内容 ID 获取详细信息
   - 测试存在和不存在的内容 ID

5. **test_search_content.js** - 测试内容搜索功能
   - 关键词搜索内容
   - 测试分页搜索
   - 验证搜索结果格式

6. **test_create_content.js** - 测试创建内容功能
   - 创建不同类型的内容
   - 测试标签处理
   - 验证创建后的内容

### 综合测试

7. **test_all_mcp_functions.js** - 综合测试脚本
   - 依次运行所有单独测试
   - 提供测试总结和统计
   - 显示通过/失败的测试列表

## 使用方法

### 前提条件

1. 确保 MCP 服务器正在运行：
   ```bash
   cd /path/to/local_content_mcp_server
   ./build/server/mcp_server
   ```

2. 服务器默认运行在 `http://localhost:8086`

### 运行单独测试

```bash
# 进入 web 目录
cd web

# 运行特定功能测试
node test_get_tags.js
node test_get_statistics.js
node test_list_content.js
node test_get_content.js
node test_search_content.js
node test_create_content.js
```

### 运行综合测试

```bash
# 运行所有测试
node test_all_mcp_functions.js

# 查看帮助信息
node test_all_mcp_functions.js --help
```

## 测试输出说明

### 成功标识
- ✅ 表示测试通过
- 📊 表示统计信息
- 📋 表示列表数据
- 🔍 表示搜索或验证操作

### 失败标识
- ❌ 表示测试失败
- ⚠️ 表示警告信息
- 📭 表示空结果

### 测试数据
- 🆔 内容 ID
- 📝 标题
- 📂 内容类型
- 🏷️ 标签
- 📅 时间戳
- 📄 内容预览

## 测试覆盖的功能

### MCP 工具测试
- `get_tags` - 获取标签列表
- `get_statistics` - 获取统计信息
- `list_content` - 列出内容（分页）
- `get_content` - 根据 ID 获取内容
- `search_content` - 搜索内容
- `create_content` - 创建新内容

### 测试场景
- 正常功能测试
- 边界条件测试（空参数、不存在的 ID 等）
- 错误处理测试
- 数据格式验证
- 分页功能测试

## 故障排除

### 常见问题

1. **连接错误**
   ```
   ❌ 请求失败: fetch failed
   ```
   - 检查服务器是否正在运行
   - 确认服务器地址和端口正确

2. **模块导入错误**
   ```
   ReferenceError: require is not defined
   ```
   - 项目使用 ES 模块，确保使用 `import` 语法

3. **权限错误**
   ```
   Permission denied
   ```
   - 确保测试脚本有执行权限
   - 使用 `chmod +x test_*.js` 添加执行权限

### 调试技巧

1. **查看服务器日志**
   - 服务器会输出请求日志，有助于调试

2. **单独运行测试**
   - 如果综合测试失败，可以单独运行各个测试文件

3. **检查数据库状态**
   - 确保数据库文件存在且可访问

## 扩展测试

如需添加新的测试用例：

1. 创建新的测试文件，遵循现有命名规范
2. 使用相同的输出格式和错误处理
3. 在 `test_all_mcp_functions.js` 中添加新测试文件
4. 更新本 README 文档

## 依赖项

- Node.js (v14+)
- node-fetch (v3+)
- 运行中的 MCP 服务器

## 许可证

与主项目相同的许可证。