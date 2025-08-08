#include "mcp_server.hpp"
#include <spdlog/spdlog.h>

namespace mcp {

// MCPTool JSON转换
nlohmann::json MCPTool::to_json() const {
    nlohmann::json j;
    j["name"] = name;
    j["description"] = description;
    j["inputSchema"] = input_schema;
    return j;
}

// MCPResource JSON转换
nlohmann::json MCPResource::to_json() const {
    nlohmann::json j;
    j["uri"] = uri;
    j["name"] = name;
    j["description"] = description;
    j["mimeType"] = mime_type;
    return j;
}

// MCPServer实现
MCPServer::MCPServer(std::shared_ptr<ContentManager> content_manager) 
    : content_manager_(std::move(content_manager)) {
    initialize_tools();
}

void MCPServer::initialize_tools() {
    // 创建内容工具
    MCPTool create_tool;
    create_tool.name = "create_content";
    create_tool.description = "Create a new content item";
    create_tool.input_schema = {
        {"type", "object"},
        {"properties", {
            {"title", {{"type", "string"}, {"description", "Content title"}}},
            {"content", {{"type", "string"}, {"description", "Content body"}}},
            {"content_type", {{"type", "string"}, {"description", "Content type (text, markdown, code, etc.)"}, {"default", "text"}}},
            {"tags", {{"type", "string"}, {"description", "Comma-separated tags"}}},
            {"metadata", {{"type", "object"}, {"description", "Additional metadata"}}}
        }},
        {"required", {"title", "content"}}
    };
    tools_[create_tool.name] = create_tool;
    tool_handlers_[create_tool.name] = [this](const nlohmann::json& args) { return tool_create_content(args); };
    
    // 获取内容工具
    MCPTool get_tool;
    get_tool.name = "get_content";
    get_tool.description = "Get content by ID";
    get_tool.input_schema = {
        {"type", "object"},
        {"properties", {
            {"id", {{"type", "integer"}, {"description", "Content ID"}}}
        }},
        {"required", {"id"}}
    };
    tools_[get_tool.name] = get_tool;
    tool_handlers_[get_tool.name] = [this](const nlohmann::json& args) { return tool_get_content(args); };
    
    // 更新内容工具
    MCPTool update_tool;
    update_tool.name = "update_content";
    update_tool.description = "Update existing content";
    update_tool.input_schema = {
        {"type", "object"},
        {"properties", {
            {"id", {{"type", "integer"}, {"description", "Content ID"}}},
            {"title", {{"type", "string"}, {"description", "Content title"}}},
            {"content", {{"type", "string"}, {"description", "Content body"}}},
            {"content_type", {{"type", "string"}, {"description", "Content type"}}},
            {"tags", {{"type", "string"}, {"description", "Comma-separated tags"}}},
            {"metadata", {{"type", "object"}, {"description", "Additional metadata"}}}
        }},
        {"required", {"id", "title", "content"}}
    };
    tools_[update_tool.name] = update_tool;
    tool_handlers_[update_tool.name] = [this](const nlohmann::json& args) { return tool_update_content(args); };
    
    // 删除内容工具
    MCPTool delete_tool;
    delete_tool.name = "delete_content";
    delete_tool.description = "Delete content by ID";
    delete_tool.input_schema = {
        {"type", "object"},
        {"properties", {
            {"id", {{"type", "integer"}, {"description", "Content ID"}}}
        }},
        {"required", {"id"}}
    };
    tools_[delete_tool.name] = delete_tool;
    tool_handlers_[delete_tool.name] = [this](const nlohmann::json& args) { return tool_delete_content(args); };
    
    // 搜索内容工具
    MCPTool search_tool;
    search_tool.name = "search_content";
    search_tool.description = "Search content using full-text search";
    search_tool.input_schema = {
        {"type", "object"},
        {"properties", {
            {"query", {{"type", "string"}, {"description", "Search query"}}},
            {"page", {{"type", "integer"}, {"description", "Page number"}, {"default", 1}}},
            {"page_size", {{"type", "integer"}, {"description", "Items per page"}, {"default", 20}}}
        }},
        {"required", {"query"}}
    };
    tools_[search_tool.name] = search_tool;
    tool_handlers_[search_tool.name] = [this](const nlohmann::json& args) { return tool_search_content(args); };
    
    // 列出内容工具
    MCPTool list_tool;
    list_tool.name = "list_content";
    list_tool.description = "List all content with pagination";
    list_tool.input_schema = {
        {"type", "object"},
        {"properties", {
            {"page", {{"type", "integer"}, {"description", "Page number"}, {"default", 1}}},
            {"page_size", {{"type", "integer"}, {"description", "Items per page"}, {"default", 20}}}
        }}
    };
    tools_[list_tool.name] = list_tool;
    tool_handlers_[list_tool.name] = [this](const nlohmann::json& args) { return tool_list_content(args); };
    
    // 获取标签工具
    MCPTool tags_tool;
    tags_tool.name = "get_tags";
    tags_tool.description = "Get all available tags";
    tags_tool.input_schema = {
        {"type", "object"},
        {"properties", {}}
    };
    tools_[tags_tool.name] = tags_tool;
    tool_handlers_[tags_tool.name] = [this](const nlohmann::json& args) { return tool_get_tags(args); };
    
    // 获取统计信息工具
    MCPTool stats_tool;
    stats_tool.name = "get_statistics";
    stats_tool.description = "Get content statistics";
    stats_tool.input_schema = {
        {"type", "object"},
        {"properties", {}}
    };
    tools_[stats_tool.name] = stats_tool;
    tool_handlers_[stats_tool.name] = [this](const nlohmann::json& args) { return tool_get_statistics(args); };
}

nlohmann::json MCPServer::handle_initialize(const nlohmann::json& params) {
    nlohmann::json response;
    response["protocolVersion"] = "2024-11-05";
    response["capabilities"] = {
        {"tools", nlohmann::json::object()},
        {"resources", nlohmann::json::object()}
    };
    response["serverInfo"] = {
        {"name", "Local Content MCP Server"},
        {"version", "1.0.0"}
    };
    
    spdlog::info("MCP Server initialized with client: {}", 
                 params.value("clientInfo", nlohmann::json::object()).value("name", "unknown"));
    
    return response;
}

nlohmann::json MCPServer::handle_list_tools() {
    nlohmann::json tools_array = nlohmann::json::array();
    
    for (const auto& [name, tool] : tools_) {
        tools_array.push_back(tool.to_json());
    }
    
    nlohmann::json response;
    response["tools"] = tools_array;
    
    return response;
}

nlohmann::json MCPServer::handle_call_tool(const std::string& tool_name, const nlohmann::json& arguments) {
    auto it = tool_handlers_.find(tool_name);
    if (it == tool_handlers_.end()) {
        return create_error_response(-1, "Unknown tool: " + tool_name);
    }
    
    try {
        auto result = it->second(arguments);
        
        nlohmann::json response;
        response["content"] = {
            {
                {"type", "text"},
                {"text", result.dump(2)}
            }
        };
        
        return response;
        
    } catch (const std::exception& e) {
        spdlog::error("Error calling tool {}: {}", tool_name, e.what());
        return create_error_response(-2, "Tool execution error: " + std::string(e.what()));
    }
}

nlohmann::json MCPServer::handle_list_resources() {
    nlohmann::json resources_array = nlohmann::json::array();
    
    // 添加一些示例资源
    MCPResource content_resource;
    content_resource.uri = "content://all";
    content_resource.name = "All Content";
    content_resource.description = "All content items in the database";
    content_resource.mime_type = "application/json";
    resources_array.push_back(content_resource.to_json());
    
    MCPResource stats_resource;
    stats_resource.uri = "stats://summary";
    stats_resource.name = "Content Statistics";
    stats_resource.description = "Summary statistics of the content database";
    stats_resource.mime_type = "application/json";
    resources_array.push_back(stats_resource.to_json());
    
    nlohmann::json response;
    response["resources"] = resources_array;
    
    return response;
}

nlohmann::json MCPServer::handle_read_resource(const std::string& uri) {
    nlohmann::json response;
    
    if (uri == "content://all") {
        auto result = content_manager_->list_content(1, 100);
        response["contents"] = {
            {
                {"uri", uri},
                {"mimeType", "application/json"},
                {"text", result.dump(2)}
            }
        };
    } else if (uri == "stats://summary") {
        auto result = content_manager_->get_statistics();
        response["contents"] = {
            {
                {"uri", uri},
                {"mimeType", "application/json"},
                {"text", result.dump(2)}
            }
        };
    } else {
        return create_error_response(-1, "Unknown resource: " + uri);
    }
    
    return response;
}

nlohmann::json MCPServer::handle_request(const nlohmann::json& request) {
    try {
        std::string error_msg;
        if (!validate_request(request, error_msg)) {
            return create_error_response(-1, error_msg);
        }
        
        const std::string method = request["method"];
        const auto& params = request.value("params", nlohmann::json::object());
        
        if (method == "initialize") {
            return handle_initialize(params);
        } else if (method == "tools/list") {
            return handle_list_tools();
        } else if (method == "tools/call") {
            const std::string tool_name = params["name"];
            const auto& arguments = params.value("arguments", nlohmann::json::object());
            return handle_call_tool(tool_name, arguments);
        } else if (method == "resources/list") {
            return handle_list_resources();
        } else if (method == "resources/read") {
            const std::string uri = params["uri"];
            return handle_read_resource(uri);
        } else {
            return create_error_response(-1, "Unknown method: " + method);
        }
        
    } catch (const std::exception& e) {
        spdlog::error("Error handling request: {}", e.what());
        return create_error_response(-2, "Internal server error");
    }
}

nlohmann::json MCPServer::get_server_info() const {
    nlohmann::json info;
    info["name"] = "Local Content MCP Server";
    info["version"] = "1.0.0";
    info["description"] = "A local content management server implementing the MCP protocol";
    info["protocol_version"] = "2024-11-05";
    info["tools_count"] = tools_.size();
    
    nlohmann::json tools_list = nlohmann::json::array();
    for (const auto& [name, tool] : tools_) {
        tools_list.push_back(name);
    }
    info["available_tools"] = tools_list;
    
    return info;
}

// 工具处理函数实现
nlohmann::json MCPServer::tool_create_content(const nlohmann::json& args) {
    return content_manager_->create_content(args);
}

nlohmann::json MCPServer::tool_get_content(const nlohmann::json& args) {
    if (!args.contains("id") || !args["id"].is_number_integer()) {
        return create_error_response(-1, "ID parameter is required and must be an integer");
    }
    
    int64_t id = args["id"];
    return content_manager_->get_content(id);
}

nlohmann::json MCPServer::tool_update_content(const nlohmann::json& args) {
    if (!args.contains("id") || !args["id"].is_number_integer()) {
        return create_error_response(-1, "ID parameter is required and must be an integer");
    }
    
    int64_t id = args["id"];
    return content_manager_->update_content(id, args);
}

nlohmann::json MCPServer::tool_delete_content(const nlohmann::json& args) {
    if (!args.contains("id") || !args["id"].is_number_integer()) {
        return create_error_response(-1, "ID parameter is required and must be an integer");
    }
    
    int64_t id = args["id"];
    return content_manager_->delete_content(id);
}

nlohmann::json MCPServer::tool_search_content(const nlohmann::json& args) {
    if (!args.contains("query") || !args["query"].is_string()) {
        return create_error_response(-1, "Query parameter is required and must be a string");
    }
    
    const std::string query = args["query"];
    int page = args.value("page", 1);
    int page_size = args.value("page_size", 20);
    
    return content_manager_->search_content(query, page, page_size);
}

nlohmann::json MCPServer::tool_list_content(const nlohmann::json& args) {
    int page = args.value("page", 1);
    int page_size = args.value("page_size", 20);
    
    return content_manager_->list_content(page, page_size);
}

nlohmann::json MCPServer::tool_get_tags(const nlohmann::json& /* args */) {
    return content_manager_->get_tags();
}

nlohmann::json MCPServer::tool_get_statistics(const nlohmann::json& /* args */) {
    return content_manager_->get_statistics();
}

// 辅助方法实现
nlohmann::json MCPServer::create_error_response(int code, const std::string& message) {
    nlohmann::json response;
    response["error"] = {
        {"code", code},
        {"message", message}
    };
    return response;
}

nlohmann::json MCPServer::create_success_response(const nlohmann::json& result) {
    nlohmann::json response;
    response["result"] = result;
    return response;
}

bool MCPServer::validate_request(const nlohmann::json& request, std::string& error_msg) {
    if (!request.is_object()) {
        error_msg = "Request must be a JSON object";
        return false;
    }
    
    if (!request.contains("method") || !request["method"].is_string()) {
        error_msg = "Method field is required and must be a string";
        return false;
    }
    
    // 可以添加更多验证逻辑
    return true;
}

} // namespace mcp