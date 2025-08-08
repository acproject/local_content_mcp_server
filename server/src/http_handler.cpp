#include "http_handler.hpp"
#include <spdlog/spdlog.h>
#include <thread>

namespace mcp {

HttpHandler::HttpHandler(std::shared_ptr<MCPServer> mcp_server) 
    : mcp_server_(std::move(mcp_server)), server_(std::make_unique<httplib::Server>()) {
    setup_routes();
}

bool HttpHandler::start(const std::string& host, int port) {
    spdlog::info("Starting HTTP server on {}:{}", host, port);
    
    // 在新线程中启动服务器
    std::thread server_thread([this, host, port]() {
        if (!server_->listen(host.c_str(), port)) {
            spdlog::error("Failed to start HTTP server");
        }
    });
    
    server_thread.detach();
    
    // 等待一小段时间确保服务器启动
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    return server_->is_running();
}

void HttpHandler::stop() {
    if (server_) {
        server_->stop();
        spdlog::info("HTTP server stopped");
    }
}

void HttpHandler::setup_routes() {
    // 设置CORS中间件
    server_->set_pre_routing_handler([this](const httplib::Request& req, httplib::Response& res) {
        set_cors_headers(res);
        return httplib::Server::HandlerResponse::Unhandled;
    });
    
    // 处理OPTIONS请求（CORS预检）
    server_->Options(".*", [this](const httplib::Request& req, httplib::Response& res) {
        set_cors_headers(res);
        res.status = 200;
    });
    
    // MCP协议端点
    server_->Post("/mcp", [this](const httplib::Request& req, httplib::Response& res) {
        handle_mcp_request(req, res);
    });
    
    // RESTful API端点
    server_->Get("/api/content/(\\d+)", [this](const httplib::Request& req, httplib::Response& res) {
        handle_get_content(req, res);
    });
    
    server_->Post("/api/content", [this](const httplib::Request& req, httplib::Response& res) {
        handle_create_content(req, res);
    });
    
    server_->Put("/api/content/(\\d+)", [this](const httplib::Request& req, httplib::Response& res) {
        handle_update_content(req, res);
    });
    
    server_->Delete("/api/content/(\\d+)", [this](const httplib::Request& req, httplib::Response& res) {
        handle_delete_content(req, res);
    });
    
    server_->Get("/api/content/search", [this](const httplib::Request& req, httplib::Response& res) {
        handle_search_content(req, res);
    });
    
    server_->Get("/api/content", [this](const httplib::Request& req, httplib::Response& res) {
        handle_list_content(req, res);
    });
    
    server_->Get("/api/tags", [this](const httplib::Request& req, httplib::Response& res) {
        handle_get_tags(req, res);
    });
    
    server_->Get("/api/statistics", [this](const httplib::Request& req, httplib::Response& res) {
        handle_get_statistics(req, res);
    });
    
    // 健康检查和信息端点
    server_->Get("/health", [this](const httplib::Request& req, httplib::Response& res) {
        handle_health_check(req, res);
    });
    
    server_->Get("/info", [this](const httplib::Request& req, httplib::Response& res) {
        handle_server_info(req, res);
    });
    
    // 静态文件服务
    server_->Get("/", [this](const httplib::Request& req, httplib::Response& res) {
        handle_static_files(req, res);
    });
    
    server_->Get("/(.*)", [this](const httplib::Request& req, httplib::Response& res) {
        handle_static_files(req, res);
    });
    
    // 错误处理
    server_->set_error_handler([](const httplib::Request& req, httplib::Response& res) {
        nlohmann::json error;
        error["error"] = {
            {"code", res.status},
            {"message", "HTTP Error"}
        };
        res.set_content(error.dump(), "application/json");
    });
    
    spdlog::info("HTTP routes configured");
}

void HttpHandler::handle_mcp_request(const httplib::Request& req, httplib::Response& res) {
    try {
        nlohmann::json request_json;
        std::string error_msg;
        
        if (!parse_json_body(req.body, request_json, error_msg)) {
            send_error_response(res, "Invalid JSON: " + error_msg, 400);
            return;
        }
        
        auto response = mcp_server_->handle_request(request_json);
        send_json_response(res, response);
        
    } catch (const std::exception& e) {
        spdlog::error("Error handling MCP request: {}", e.what());
        send_error_response(res, "Internal server error", 500);
    }
}

void HttpHandler::handle_get_content(const httplib::Request& req, httplib::Response& res) {
    try {
        int64_t id = std::stoll(req.matches[1]);
        
        nlohmann::json tool_args;
        tool_args["id"] = id;
        
        auto response = mcp_server_->handle_call_tool("get_content", tool_args);
        
        // 从MCP响应中提取实际数据
        if (response.contains("content") && response["content"].is_array() && !response["content"].empty()) {
            auto content_text = response["content"][0]["text"].get<std::string>();
            auto content_json = nlohmann::json::parse(content_text);
            send_json_response(res, content_json);
        } else {
            send_json_response(res, response);
        }
        
    } catch (const std::exception& e) {
        spdlog::error("Error getting content: {}", e.what());
        send_error_response(res, "Invalid content ID", 400);
    }
}

void HttpHandler::handle_create_content(const httplib::Request& req, httplib::Response& res) {
    try {
        nlohmann::json request_json;
        std::string error_msg;
        
        if (!parse_json_body(req.body, request_json, error_msg)) {
            send_error_response(res, "Invalid JSON: " + error_msg, 400);
            return;
        }
        
        auto response = mcp_server_->handle_call_tool("create_content", request_json);
        
        // 从MCP响应中提取实际数据
        if (response.contains("content") && response["content"].is_array() && !response["content"].empty()) {
            auto content_text = response["content"][0]["text"].get<std::string>();
            auto content_json = nlohmann::json::parse(content_text);
            send_json_response(res, content_json, 201);
        } else {
            send_json_response(res, response, 201);
        }
        
    } catch (const std::exception& e) {
        spdlog::error("Error creating content: {}", e.what());
        send_error_response(res, "Failed to create content", 500);
    }
}

void HttpHandler::handle_update_content(const httplib::Request& req, httplib::Response& res) {
    try {
        int64_t id = std::stoll(req.matches[1]);
        
        nlohmann::json request_json;
        std::string error_msg;
        
        if (!parse_json_body(req.body, request_json, error_msg)) {
            send_error_response(res, "Invalid JSON: " + error_msg, 400);
            return;
        }
        
        request_json["id"] = id;
        
        auto response = mcp_server_->handle_call_tool("update_content", request_json);
        
        // 从MCP响应中提取实际数据
        if (response.contains("content") && response["content"].is_array() && !response["content"].empty()) {
            auto content_text = response["content"][0]["text"].get<std::string>();
            auto content_json = nlohmann::json::parse(content_text);
            send_json_response(res, content_json);
        } else {
            send_json_response(res, response);
        }
        
    } catch (const std::exception& e) {
        spdlog::error("Error updating content: {}", e.what());
        send_error_response(res, "Failed to update content", 500);
    }
}

void HttpHandler::handle_delete_content(const httplib::Request& req, httplib::Response& res) {
    try {
        int64_t id = std::stoll(req.matches[1]);
        
        nlohmann::json tool_args;
        tool_args["id"] = id;
        
        auto response = mcp_server_->handle_call_tool("delete_content", tool_args);
        
        // 从MCP响应中提取实际数据
        if (response.contains("content") && response["content"].is_array() && !response["content"].empty()) {
            auto content_text = response["content"][0]["text"].get<std::string>();
            auto content_json = nlohmann::json::parse(content_text);
            send_json_response(res, content_json);
        } else {
            send_json_response(res, response);
        }
        
    } catch (const std::exception& e) {
        spdlog::error("Error deleting content: {}", e.what());
        send_error_response(res, "Failed to delete content", 500);
    }
}

void HttpHandler::handle_search_content(const httplib::Request& req, httplib::Response& res) {
    try {
        std::string query = get_param(req, "q", "");
        if (query.empty()) {
            send_error_response(res, "Query parameter 'q' is required", 400);
            return;
        }
        
        int page = parse_int_param(req, "page", 1);
        int page_size = parse_int_param(req, "page_size", 20);
        
        nlohmann::json tool_args;
        tool_args["query"] = query;
        tool_args["page"] = page;
        tool_args["page_size"] = page_size;
        
        auto response = mcp_server_->handle_call_tool("search_content", tool_args);
        
        // 从MCP响应中提取实际数据
        if (response.contains("content") && response["content"].is_array() && !response["content"].empty()) {
            auto content_text = response["content"][0]["text"].get<std::string>();
            auto content_json = nlohmann::json::parse(content_text);
            send_json_response(res, content_json);
        } else {
            send_json_response(res, response);
        }
        
    } catch (const std::exception& e) {
        spdlog::error("Error searching content: {}", e.what());
        send_error_response(res, "Search failed", 500);
    }
}

void HttpHandler::handle_list_content(const httplib::Request& req, httplib::Response& res) {
    try {
        int page = parse_int_param(req, "page", 1);
        int page_size = parse_int_param(req, "page_size", 20);
        
        nlohmann::json tool_args;
        tool_args["page"] = page;
        tool_args["page_size"] = page_size;
        
        auto response = mcp_server_->handle_call_tool("list_content", tool_args);
        
        // 从MCP响应中提取实际数据
        if (response.contains("content") && response["content"].is_array() && !response["content"].empty()) {
            auto content_text = response["content"][0]["text"].get<std::string>();
            auto content_json = nlohmann::json::parse(content_text);
            send_json_response(res, content_json);
        } else {
            send_json_response(res, response);
        }
        
    } catch (const std::exception& e) {
        spdlog::error("Error listing content: {}", e.what());
        send_error_response(res, "Failed to list content", 500);
    }
}

void HttpHandler::handle_get_tags(const httplib::Request& req, httplib::Response& res) {
    try {
        nlohmann::json tool_args = nlohmann::json::object();
        
        auto response = mcp_server_->handle_call_tool("get_tags", tool_args);
        
        // 从MCP响应中提取实际数据
        if (response.contains("content") && response["content"].is_array() && !response["content"].empty()) {
            auto content_text = response["content"][0]["text"].get<std::string>();
            auto content_json = nlohmann::json::parse(content_text);
            send_json_response(res, content_json);
        } else {
            send_json_response(res, response);
        }
        
    } catch (const std::exception& e) {
        spdlog::error("Error getting tags: {}", e.what());
        send_error_response(res, "Failed to get tags", 500);
    }
}

void HttpHandler::handle_get_statistics(const httplib::Request& req, httplib::Response& res) {
    try {
        nlohmann::json tool_args = nlohmann::json::object();
        
        auto response = mcp_server_->handle_call_tool("get_statistics", tool_args);
        
        // 从MCP响应中提取实际数据
        if (response.contains("content") && response["content"].is_array() && !response["content"].empty()) {
            auto content_text = response["content"][0]["text"].get<std::string>();
            auto content_json = nlohmann::json::parse(content_text);
            send_json_response(res, content_json);
        } else {
            send_json_response(res, response);
        }
        
    } catch (const std::exception& e) {
        spdlog::error("Error getting statistics: {}", e.what());
        send_error_response(res, "Failed to get statistics", 500);
    }
}

void HttpHandler::handle_health_check(const httplib::Request& req, httplib::Response& res) {
    nlohmann::json health;
    health["status"] = "healthy";
    health["timestamp"] = std::time(nullptr);
    health["server"] = "Local Content MCP Server";
    
    send_json_response(res, health);
}

void HttpHandler::handle_server_info(const httplib::Request& req, httplib::Response& res) {
    auto info = mcp_server_->get_server_info();
    send_json_response(res, info);
}

void HttpHandler::handle_static_files(const httplib::Request& req, httplib::Response& res) {
    // 简单的静态文件处理
    std::string path = req.path;
    if (path == "/") {
        path = "/index.html";
    }
    
    // 这里可以实现静态文件服务
    // 目前返回一个简单的HTML页面
    if (path == "/index.html") {
        std::string html = R"(
<!DOCTYPE html>
<html>
<head>
    <title>Local Content MCP Server</title>
    <meta charset="utf-8">
    <style>
        body { font-family: Arial, sans-serif; margin: 40px; }
        .container { max-width: 800px; margin: 0 auto; }
        .api-endpoint { background: #f5f5f5; padding: 10px; margin: 10px 0; border-radius: 5px; }
        .method { font-weight: bold; color: #007acc; }
    </style>
</head>
<body>
    <div class="container">
        <h1>Local Content MCP Server</h1>
        <p>Welcome to the Local Content Management MCP Server!</p>
        
        <h2>API Endpoints</h2>
        <div class="api-endpoint">
            <span class="method">GET</span> /health - Health check
        </div>
        <div class="api-endpoint">
            <span class="method">GET</span> /info - Server information
        </div>
        <div class="api-endpoint">
            <span class="method">POST</span> /mcp - MCP protocol endpoint
        </div>
        <div class="api-endpoint">
            <span class="method">GET</span> /api/content - List content
        </div>
        <div class="api-endpoint">
            <span class="method">POST</span> /api/content - Create content
        </div>
        <div class="api-endpoint">
            <span class="method">GET</span> /api/content/{id} - Get content by ID
        </div>
        <div class="api-endpoint">
            <span class="method">PUT</span> /api/content/{id} - Update content
        </div>
        <div class="api-endpoint">
            <span class="method">DELETE</span> /api/content/{id} - Delete content
        </div>
        <div class="api-endpoint">
            <span class="method">GET</span> /api/content/search?q={query} - Search content
        </div>
        <div class="api-endpoint">
            <span class="method">GET</span> /api/tags - Get all tags
        </div>
        <div class="api-endpoint">
            <span class="method">GET</span> /api/statistics - Get statistics
        </div>
        
        <h2>MCP Protocol</h2>
        <p>This server implements the Model Context Protocol (MCP) for content management.</p>
        <p>Use the <code>/mcp</code> endpoint to interact with the server using MCP protocol.</p>
    </div>
</body>
</html>
        )";
        
        res.set_content(html, "text/html");
    } else {
        res.status = 404;
        send_error_response(res, "File not found", 404);
    }
}

// 辅助方法实现
void HttpHandler::set_cors_headers(httplib::Response& res) {
    res.set_header("Access-Control-Allow-Origin", "*");
    res.set_header("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
    res.set_header("Access-Control-Allow-Headers", "Content-Type, Authorization");
    res.set_header("Access-Control-Max-Age", "86400");
}

void HttpHandler::send_json_response(httplib::Response& res, const nlohmann::json& json, int status) {
    res.status = status;
    res.set_content(json.dump(2), "application/json");
}

void HttpHandler::send_error_response(httplib::Response& res, const std::string& message, int status) {
    nlohmann::json error;
    error["success"] = false;
    error["error"] = {
        {"code", status},
        {"message", message}
    };
    
    res.status = status;
    res.set_content(error.dump(2), "application/json");
}

bool HttpHandler::parse_json_body(const std::string& body, nlohmann::json& json, std::string& error_msg) {
    try {
        json = nlohmann::json::parse(body);
        return true;
    } catch (const std::exception& e) {
        error_msg = e.what();
        return false;
    }
}

int HttpHandler::parse_int_param(const httplib::Request& req, const std::string& param, int default_value) {
    auto it = req.params.find(param);
    if (it != req.params.end()) {
        try {
            return std::stoi(it->second);
        } catch (const std::exception&) {
            return default_value;
        }
    }
    return default_value;
}

std::string HttpHandler::get_param(const httplib::Request& req, const std::string& param, const std::string& default_value) {
    auto it = req.params.find(param);
    if (it != req.params.end()) {
        return it->second;
    }
    return default_value;
}

} // namespace mcp