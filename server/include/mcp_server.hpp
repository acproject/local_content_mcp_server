#pragma once

#include "content_manager.hpp"
#include "config.hpp"
#include <memory>
#include <string>
#include <unordered_map>
#include <functional>
#include <nlohmann/json.hpp>

namespace mcp {

// MCP工具定义
struct MCPTool {
    std::string name;
    std::string description;
    nlohmann::json input_schema;
    
    nlohmann::json to_json() const;
};

// MCP资源定义
struct MCPResource {
    std::string uri;
    std::string name;
    std::string description;
    std::string mime_type;
    
    nlohmann::json to_json() const;
};

// MCP服务器类
class MCPServer {
public:
    explicit MCPServer(std::shared_ptr<ContentManager> content_manager);
    
    // MCP协议方法
    nlohmann::json handle_initialize(const nlohmann::json& params);
    nlohmann::json handle_list_tools();
    nlohmann::json handle_call_tool(const std::string& tool_name, const nlohmann::json& arguments);
    nlohmann::json handle_list_resources();
    nlohmann::json handle_read_resource(const std::string& uri);
    
    // 通用请求处理
    nlohmann::json handle_request(const nlohmann::json& request);
    
    // 获取服务器信息
    nlohmann::json get_server_info() const;
    
private:
    std::shared_ptr<ContentManager> content_manager_;
    std::unordered_map<std::string, MCPTool> tools_;
    std::unordered_map<std::string, std::function<nlohmann::json(const nlohmann::json&)>> tool_handlers_;
    
    // 初始化工具
    void initialize_tools();
    
    // 工具处理函数
    nlohmann::json tool_create_content(const nlohmann::json& args);
    nlohmann::json tool_get_content(const nlohmann::json& args);
    nlohmann::json tool_update_content(const nlohmann::json& args);
    nlohmann::json tool_delete_content(const nlohmann::json& args);
    nlohmann::json tool_search_content(const nlohmann::json& args);
    nlohmann::json tool_list_content(const nlohmann::json& args);
    nlohmann::json tool_get_tags(const nlohmann::json& args);
    nlohmann::json tool_get_statistics(const nlohmann::json& args);
    
    // 辅助方法
    nlohmann::json create_error_response(int code, const std::string& message);
    nlohmann::json create_success_response(const nlohmann::json& result = nlohmann::json::object());
    bool validate_request(const nlohmann::json& request, std::string& error_msg);
};

} // namespace mcp