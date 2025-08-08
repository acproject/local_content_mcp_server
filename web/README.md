# 内容管理系统 - Web前端

这是一个基于React和TypeScript开发的内容管理系统前端界面，提供了直观的Web界面来管理内容。

## 功能特性

- 📝 **内容管理**: 创建、编辑、查看和删除内容
- 🔍 **搜索功能**: 支持按标题、内容和标签搜索
- 🏷️ **标签管理**: 查看所有标签并按标签筛选内容
- 📱 **响应式设计**: 支持桌面和移动设备
- 🎨 **Material-UI**: 现代化的用户界面
- 📄 **分页支持**: 大量内容的分页显示
- 🔄 **实时更新**: 与后端API实时同步

## 技术栈

- **React 18**: 前端框架
- **TypeScript**: 类型安全的JavaScript
- **Material-UI**: UI组件库
- **React Router**: 路由管理
- **Axios**: HTTP客户端

## 快速开始

### 1. 安装依赖

```bash
cd web
npm install
```

### 2. 启动开发服务器

```bash
npm start
```

应用将在 http://localhost:3000 启动

### 3. 确保后端服务运行

确保内容管理服务器在 http://localhost:8080 运行

## 可用脚本

- `npm start`: 启动开发服务器
- `npm run build`: 构建生产版本
- `npm test`: 运行测试
- `npm run eject`: 弹出配置（不可逆）

## 项目结构

```
web/
├── public/
│   └── index.html          # HTML模板
├── src/
│   ├── components/         # React组件
│   │   ├── ContentList.tsx    # 内容列表
│   │   ├── ContentForm.tsx    # 内容表单
│   │   ├── ContentDetail.tsx  # 内容详情
│   │   ├── ContentSearch.tsx  # 搜索功能
│   │   └── TagsView.tsx       # 标签视图
│   ├── services/
│   │   └── api.ts          # API服务
│   ├── App.tsx             # 主应用组件
│   └── index.tsx           # 应用入口
├── package.json            # 项目配置
└── tsconfig.json          # TypeScript配置
```

## 页面说明

### 首页 (`/`)
- 显示所有内容的列表
- 支持分页浏览
- 提供快速操作按钮（查看、编辑、删除）

### 创建内容 (`/create`)
- 创建新的内容项
- 支持多种内容类型（文本、Markdown、HTML、代码）
- 标签管理

### 编辑内容 (`/edit/:id`)
- 编辑现有内容
- 保持原有格式和标签

### 内容详情 (`/content/:id`)
- 查看完整的内容详情
- 根据内容类型渲染不同样式
- 提供编辑和删除操作

### 搜索 (`/search`)
- 全文搜索功能
- 关键词高亮显示
- 搜索结果预览

### 标签管理 (`/tags`)
- 查看所有标签
- 按标签筛选内容
- 标签统计信息

## 配置

### 环境变量

在 `.env` 文件中配置：

```
REACT_APP_API_URL=http://localhost:8080/api
PORT=3000
```

### API配置

默认API地址为 `http://localhost:8080/api`，可通过环境变量 `REACT_APP_API_URL` 修改。

## 部署

### 构建生产版本

```bash
npm run build
```

构建文件将生成在 `build/` 目录中。

### 静态文件服务

可以使用任何静态文件服务器来部署构建后的文件，例如：

```bash
npx serve -s build
```

## 开发说明

### 添加新功能

1. 在 `src/components/` 中创建新组件
2. 在 `src/services/api.ts` 中添加相应的API调用
3. 在 `App.tsx` 中添加路由配置

### 样式定制

项目使用Material-UI主题系统，可以在 `App.tsx` 中修改主题配置。

### API集成

所有API调用都通过 `src/services/api.ts` 中的 `ContentAPI` 类进行，便于统一管理和错误处理。

## 故障排除

### 常见问题

1. **无法连接到后端**: 检查后端服务是否运行在正确端口
2. **CORS错误**: 确保后端配置了正确的CORS设置
3. **构建失败**: 检查TypeScript类型错误

### 调试

开发模式下，可以在浏览器开发者工具中查看网络请求和控制台日志。