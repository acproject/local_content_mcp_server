#include "http_handler.hpp"
#include "config.hpp"
#include <spdlog/spdlog.h>
#include <thread>
#include <fstream>
#include <filesystem>
#include <string>

namespace mcp {

HttpHandler::HttpHandler(std::shared_ptr<MCPServer> mcp_server)
    : mcp_server_(mcp_server), server_(std::make_unique<httplib::Server>()), llama_service_(nullptr) {
    setup_routes();
}

bool HttpHandler::initialize() {
    auto& config = Config::instance();
    
    // 初始化文件上传管理器
    if (config.is_file_upload_enabled()) {
        file_upload_manager_ = std::make_unique<FileUploadManager>();
        if (!file_upload_manager_->initialize(config.get_upload_path())) {
            spdlog::error("Failed to initialize file upload manager");
            return false;
        }
        spdlog::info("File upload manager initialized");
    }
    
    // 初始化LLaMA服务
    if (config.is_llama_enabled()) {
        llama_service_ = &LlamaService::instance();
        if (!llama_service_->start()) {
            spdlog::error("Failed to start LLaMA service");
            return false;
        }
        spdlog::info("LLaMA service started");
    }
    
    return true;
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
    
    // 配置管理端点
    server_->Get("/api/config", [this](const httplib::Request& req, httplib::Response& res) {
        handle_get_config(req, res);
    });
    
    server_->Put("/api/config", [this](const httplib::Request& req, httplib::Response& res) {
        handle_update_config(req, res);
    });
    
    server_->Post("/api/config/save", [this](const httplib::Request& req, httplib::Response& res) {
        handle_save_config(req, res);
    });
    
    // 文件上传端点
    server_->Post("/api/files/upload", [this](const httplib::Request& req, httplib::Response& res) {
        handle_upload_file(req, res);
    });
    
    server_->Get("/api/files", [this](const httplib::Request& req, httplib::Response& res) {
        handle_list_files(req, res);
    });
    
    server_->Get("/api/files/([^/]+)", [this](const httplib::Request& req, httplib::Response& res) {
        handle_get_file(req, res);
    });
    
    server_->Delete("/api/files/([^/]+)", [this](const httplib::Request& req, httplib::Response& res) {
        handle_delete_file(req, res);
    });
    
    server_->Put("/api/files/([^/]+)", [this](const httplib::Request& req, httplib::Response& res) {
        handle_update_file_info(req, res);
    });
    
    server_->Get("/api/files/search", [this](const httplib::Request& req, httplib::Response& res) {
        handle_search_files(req, res);
    });
    
    server_->Get("/api/files/([^/]+)/content", [this](const httplib::Request& req, httplib::Response& res) {
        handle_get_file_content(req, res);
    });
    
    server_->Get("/files/([^/]+)", [this](const httplib::Request& req, httplib::Response& res) {
        handle_serve_file(req, res);
    });
    
    server_->Get("/api/files/stats", [this](const httplib::Request& req, httplib::Response& res) {
        handle_get_upload_stats(req, res);
    });
    
    // LLaMA端点
    server_->Post("/api/llama/generate", [this](const httplib::Request& req, httplib::Response& res) {
        handle_llama_generate(req, res);
    });
    
    server_->Post("/api/llama/generate/stream", [this](const httplib::Request& req, httplib::Response& res) {
        handle_llama_generate_stream(req, res);
    });
    
    server_->Post("/api/llama/model/load", [this](const httplib::Request& req, httplib::Response& res) {
        handle_llama_load_model(req, res);
    });
    
    server_->Post("/api/llama/model/unload", [this](const httplib::Request& req, httplib::Response& res) {
        handle_llama_unload_model(req, res);
    });
    
    server_->Get("/api/llama/model/info", [this](const httplib::Request& req, httplib::Response& res) {
        handle_llama_model_info(req, res);
    });
    
    server_->Get("/api/llama/status", [this](const httplib::Request& req, httplib::Response& res) {
        handle_llama_status(req, res);
    });
    
    server_->Get("/api/llama/config", [this](const httplib::Request& req, httplib::Response& res) {
        handle_llama_config(req, res);
    });
    
    server_->Get("/api/llama/stats", [this](const httplib::Request& req, httplib::Response& res) {
        handle_llama_stats(req, res);
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

// 配置管理端点实现
void HttpHandler::handle_get_config(const httplib::Request& req, httplib::Response& res) {
    try {
        auto& config = Config::instance();
        nlohmann::json config_json = config.to_json();
        send_json_response(res, config_json);
    } catch (const std::exception& e) {
        spdlog::error("Error getting config: {}", e.what());
        send_error_response(res, "Failed to get configuration", 500);
    }
}

void HttpHandler::handle_update_config(const httplib::Request& req, httplib::Response& res) {
    try {
        nlohmann::json request_json;
        std::string error_msg;
        
        if (!parse_json_body(req.body, request_json, error_msg)) {
            send_error_response(res, "Invalid JSON: " + error_msg, 400);
            return;
        }
        
        auto& config = Config::instance();
        if (!config.update_config(request_json)) {
            send_error_response(res, "Failed to update configuration", 400);
            return;
        }
        
        nlohmann::json response;
        response["success"] = true;
        response["message"] = "Configuration updated successfully";
        response["config"] = config.to_json();
        
        send_json_response(res, response);
        
        spdlog::info("Configuration updated");
    } catch (const std::exception& e) {
        spdlog::error("Error updating config: {}", e.what());
        send_error_response(res, "Failed to update configuration", 500);
    }
}

void HttpHandler::handle_save_config(const httplib::Request& req, httplib::Response& res) {
    try {
        std::string config_path = get_param(req, "path", "");
        
        auto& config = Config::instance();
        if (!config.save_config_to_file(config_path)) {
            send_error_response(res, "Failed to save configuration to file", 500);
            return;
        }
        
        nlohmann::json response;
        response["success"] = true;
        response["message"] = "Configuration saved successfully";
        response["path"] = config_path.empty() ? "default" : config_path;
        
        send_json_response(res, response);
        
        spdlog::info("Configuration saved to file: {}", config_path.empty() ? "default" : config_path);
    } catch (const std::exception& e) {
        spdlog::error("Error saving config: {}", e.what());
        send_error_response(res, "Failed to save configuration", 500);
    }
}

// 文件上传端点实现
void HttpHandler::handle_upload_file(const httplib::Request& req, httplib::Response& res) {
    try {
        if (!file_upload_manager_) {
            send_error_response(res, "File upload is not enabled", 503);
            return;
        }
        
        auto file = req.get_file_value("file");
        if (file.filename.empty()) {
            send_error_response(res, "No file provided", 400);
            return;
        }
        
        std::string description = get_param(req, "description", "");
        std::vector<std::string> tags;
        
        // 解析标签
        std::string tags_str = get_param(req, "tags", "");
        if (!tags_str.empty()) {
            std::istringstream iss(tags_str);
            std::string tag;
            while (std::getline(iss, tag, ',')) {
                tag.erase(0, tag.find_first_not_of(" \t"));
                tag.erase(tag.find_last_not_of(" \t") + 1);
                if (!tag.empty()) {
                    tags.push_back(tag);
                }
            }
        }
        
        auto result = file_upload_manager_->handle_upload(req, "file");
        
        if (result.success) {
            nlohmann::json response;
            response["success"] = true;
            response["message"] = result.message;
            response["file_id"] = result.file_info.id;
            response["file_info"] = result.file_info.to_json();
            
            send_json_response(res, response, 201);
            spdlog::info("File uploaded successfully: {}", file.filename);
        } else {
            send_error_response(res, result.message, 400);
        }
    } catch (const std::exception& e) {
        spdlog::error("Error uploading file: {}", e.what());
        send_error_response(res, "Failed to upload file", 500);
    }
}

void HttpHandler::handle_list_files(const httplib::Request& req, httplib::Response& res) {
    try {
        if (!file_upload_manager_) {
            send_error_response(res, "File upload is not enabled", 503);
            return;
        }
        
        int page = parse_int_param(req, "page", 1);
        int limit = parse_int_param(req, "limit", 20);
        std::string sort_by = get_param(req, "sort_by", "upload_time");
        std::string order = get_param(req, "order", "desc");
        
        auto files = file_upload_manager_->list_files(page, limit);
        
        nlohmann::json response;
        response["files"] = nlohmann::json::array();
        
        for (const auto& file : files) {
            response["files"].push_back(file.to_json());
        }
        
        response["page"] = page;
        response["limit"] = limit;
        response["total"] = files.size();
        
        send_json_response(res, response);
    } catch (const std::exception& e) {
        spdlog::error("Error listing files: {}", e.what());
        send_error_response(res, "Failed to list files", 500);
    }
}

void HttpHandler::handle_get_file(const httplib::Request& req, httplib::Response& res) {
    try {
        if (!file_upload_manager_) {
            send_error_response(res, "File upload is not enabled", 503);
            return;
        }
        
        std::string file_id = req.matches[1];
        auto file_info = file_upload_manager_->get_file_info(file_id);
        
        if (file_info.id.empty()) {
            send_error_response(res, "File not found", 404);
            return;
        }
        
        send_json_response(res, file_info.to_json());
    } catch (const std::exception& e) {
        spdlog::error("Error getting file info: {}", e.what());
        send_error_response(res, "Failed to get file information", 500);
    }
}

void HttpHandler::handle_delete_file(const httplib::Request& req, httplib::Response& res) {
    try {
        if (!file_upload_manager_) {
            send_error_response(res, "File upload is not enabled", 503);
            return;
        }
        
        std::string file_id = req.matches[1];
        
        if (file_upload_manager_->delete_file(file_id)) {
            nlohmann::json response;
            response["success"] = true;
            response["message"] = "File deleted successfully";
            
            send_json_response(res, response);
            spdlog::info("File deleted: {}", file_id);
        } else {
            send_error_response(res, "File not found or failed to delete", 404);
        }
    } catch (const std::exception& e) {
        spdlog::error("Error deleting file: {}", e.what());
        send_error_response(res, "Failed to delete file", 500);
    }
}

void HttpHandler::handle_update_file_info(const httplib::Request& req, httplib::Response& res) {
    try {
        if (!file_upload_manager_) {
            send_error_response(res, "File upload is not enabled", 503);
            return;
        }
        
        std::string file_id = req.matches[1];
        
        nlohmann::json request_json;
        std::string error_msg;
        
        if (!parse_json_body(req.body, request_json, error_msg)) {
            send_error_response(res, "Invalid JSON: " + error_msg, 400);
            return;
        }
        
        std::string description = request_json.value("description", "");
        std::vector<std::string> tags = request_json.value("tags", std::vector<std::string>{});
        
        nlohmann::json update_data;
        if (!description.empty()) update_data["description"] = description;
        if (!tags.empty()) update_data["tags"] = tags;
        
        if (file_upload_manager_->update_file_info(file_id, update_data)) {
            auto updated_info = file_upload_manager_->get_file_info(file_id);
            
            nlohmann::json response;
            response["success"] = true;
            response["message"] = "File information updated successfully";
            response["file_info"] = updated_info.to_json();
            
            send_json_response(res, response);
            spdlog::info("File info updated: {}", file_id);
        } else {
            send_error_response(res, "File not found or failed to update", 404);
        }
    } catch (const std::exception& e) {
        spdlog::error("Error updating file info: {}", e.what());
        send_error_response(res, "Failed to update file information", 500);
    }
}

void HttpHandler::handle_search_files(const httplib::Request& req, httplib::Response& res) {
    try {
        if (!file_upload_manager_) {
            send_error_response(res, "File upload is not enabled", 503);
            return;
        }
        
        std::string query = get_param(req, "q", "");
        if (query.empty()) {
            send_error_response(res, "Search query is required", 400);
            return;
        }
        
        int page = parse_int_param(req, "page", 1);
        int limit = parse_int_param(req, "limit", 20);
        
        auto files = file_upload_manager_->search_files(query);
        
        nlohmann::json response;
        response["files"] = nlohmann::json::array();
        
        for (const auto& file : files) {
            response["files"].push_back(file.to_json());
        }
        
        response["query"] = query;
        response["page"] = page;
        response["limit"] = limit;
        response["total"] = files.size();
        
        send_json_response(res, response);
    } catch (const std::exception& e) {
        spdlog::error("Error searching files: {}", e.what());
        send_error_response(res, "Failed to search files", 500);
    }
}

void HttpHandler::handle_get_file_content(const httplib::Request& req, httplib::Response& res) {
    try {
        if (!file_upload_manager_) {
            send_error_response(res, "File upload is not enabled", 503);
            return;
        }
        
        std::string file_id = req.matches[1];
        auto content = file_upload_manager_->get_file_content(file_id);
        
        if (content.empty()) {
            send_error_response(res, "File not found or failed to read content", 404);
            return;
        }
        
        auto file_info = file_upload_manager_->get_file_info(file_id);
        
        nlohmann::json response;
        response["file_id"] = file_id;
        response["filename"] = file_info.filename;
        response["content"] = content;
        response["size"] = content.size();
        
        send_json_response(res, response);
    } catch (const std::exception& e) {
        spdlog::error("Error getting file content: {}", e.what());
        send_error_response(res, "Failed to get file content", 500);
    }
}

void HttpHandler::handle_serve_file(const httplib::Request& req, httplib::Response& res) {
    try {
        if (!file_upload_manager_) {
            send_error_response(res, "File upload is not enabled", 503);
            return;
        }
        
        std::string file_id = req.matches[1];
        
        if (file_upload_manager_->serve_file(file_id, res)) {
            // 文件已经被serve_file方法处理
            return;
        } else {
            send_error_response(res, "File not found", 404);
        }
    } catch (const std::exception& e) {
        spdlog::error("Error serving file: {}", e.what());
        send_error_response(res, "Failed to serve file", 500);
    }
}

void HttpHandler::handle_get_upload_stats(const httplib::Request& req, httplib::Response& res) {
    try {
        if (!file_upload_manager_) {
            send_error_response(res, "File upload is not enabled", 503);
            return;
        }
        
        auto stats = file_upload_manager_->get_upload_statistics();
        send_json_response(res, stats);
    } catch (const std::exception& e) {
        spdlog::error("Error getting upload stats: {}", e.what());
        send_error_response(res, "Failed to get upload statistics", 500);
    }
}

// LLaMA端点实现
void HttpHandler::handle_llama_generate(const httplib::Request& req, httplib::Response& res) {
    try {
        if (!llama_service_ || !llama_service_->is_running()) {
            send_error_response(res, "LLaMA service is not available", 503);
            return;
        }
        
        nlohmann::json request_json;
        std::string error_msg;
        
        if (!parse_json_body(req.body, request_json, error_msg)) {
            send_error_response(res, "Invalid JSON: " + error_msg, 400);
            return;
        }
        
        LlamaRequest llama_request;
        llama_request.from_json(request_json);
        
        auto response_data = llama_service_->process_request(llama_request);
        send_json_response(res, response_data.to_json());
        
    } catch (const std::exception& e) {
        spdlog::error("Error in LLaMA generation: {}", e.what());
        send_error_response(res, "Failed to generate text", 500);
    }
}

void HttpHandler::handle_llama_generate_stream(const httplib::Request& req, httplib::Response& res) {
    try {
        if (!llama_service_ || !llama_service_->is_running()) {
            send_error_response(res, "LLaMA service is not available", 503);
            return;
        }
        
        nlohmann::json request_json;
        std::string error_msg;
        
        if (!parse_json_body(req.body, request_json, error_msg)) {
            send_error_response(res, "Invalid JSON: " + error_msg, 400);
            return;
        }
        
        LlamaRequest llama_request;
        llama_request.from_json(request_json);
        llama_request.stream = true;
        
        // 设置SSE头
        res.set_header("Content-Type", "text/event-stream");
        res.set_header("Cache-Control", "no-cache");
        res.set_header("Connection", "keep-alive");
        set_cors_headers(res);
        
        // 流式响应处理
        res.set_content_provider(
            "text/event-stream",
            [this, llama_request](size_t offset, httplib::DataSink& sink) {
                // 这里应该实现真正的流式生成
                // 目前作为示例，我们发送一个完整的响应
                auto response_data = llama_service_->process_request(llama_request);
                
                std::string event_data = "data: " + response_data.to_json().dump() + "\n\n";
                sink.write(event_data.c_str(), event_data.size());
                
                return true; // 完成
            }
        );
        
    } catch (const std::exception& e) {
        spdlog::error("Error in LLaMA stream generation: {}", e.what());
        send_error_response(res, "Failed to generate stream", 500);
    }
}

void HttpHandler::handle_llama_load_model(const httplib::Request& req, httplib::Response& res) {
    try {
        if (!llama_service_) {
            send_error_response(res, "LLaMA service is not available", 503);
            return;
        }
        
        nlohmann::json request_json;
        std::string error_msg;
        
        if (!parse_json_body(req.body, request_json, error_msg)) {
            send_error_response(res, "Invalid JSON: " + error_msg, 400);
            return;
        }
        
        std::string model_path = request_json.value("model_path", "");
        if (model_path.empty()) {
            send_error_response(res, "Model path is required", 400);
            return;
        }
        
        // 这里需要实现模型加载逻辑
        // 目前作为占位符
        nlohmann::json response;
        response["success"] = true;
        response["message"] = "Model loading initiated";
        response["model_path"] = model_path;
        
        send_json_response(res, response);
        
    } catch (const std::exception& e) {
        spdlog::error("Error loading LLaMA model: {}", e.what());
        send_error_response(res, "Failed to load model", 500);
    }
}

void HttpHandler::handle_llama_unload_model(const httplib::Request& req, httplib::Response& res) {
    try {
        if (!llama_service_) {
            send_error_response(res, "LLaMA service is not available", 503);
            return;
        }
        
        // 这里需要实现模型卸载逻辑
        nlohmann::json response;
        response["success"] = true;
        response["message"] = "Model unloaded successfully";
        
        send_json_response(res, response);
        
    } catch (const std::exception& e) {
        spdlog::error("Error unloading LLaMA model: {}", e.what());
        send_error_response(res, "Failed to unload model", 500);
    }
}

void HttpHandler::handle_llama_model_info(const httplib::Request& req, httplib::Response& res) {
    try {
        if (!llama_service_) {
            send_error_response(res, "LLaMA service is not available", 503);
            return;
        }
        
        auto status = llama_service_->get_status();
        auto model_info = status.value("model_info", nlohmann::json{});
        
        send_json_response(res, model_info);
        
    } catch (const std::exception& e) {
        spdlog::error("Error getting LLaMA model info: {}", e.what());
        send_error_response(res, "Failed to get model information", 500);
    }
}

void HttpHandler::handle_llama_status(const httplib::Request& req, httplib::Response& res) {
    try {
        if (!llama_service_) {
            nlohmann::json status;
            status["running"] = false;
            status["available"] = false;
            status["message"] = "LLaMA service is not initialized";
            
            send_json_response(res, status);
            return;
        }
        
        auto status = llama_service_->get_status();
        send_json_response(res, status);
        
    } catch (const std::exception& e) {
        spdlog::error("Error getting LLaMA status: {}", e.what());
        send_error_response(res, "Failed to get LLaMA status", 500);
    }
}

void HttpHandler::handle_llama_config(const httplib::Request& req, httplib::Response& res) {
    try {
        if (!llama_service_) {
            send_error_response(res, "LLaMA service is not available", 503);
            return;
        }
        
        auto status = llama_service_->get_status();
        auto config = status.value("config", nlohmann::json{});
        
        send_json_response(res, config);
        
    } catch (const std::exception& e) {
        spdlog::error("Error getting LLaMA config: {}", e.what());
        send_error_response(res, "Failed to get LLaMA configuration", 500);
    }
}

void HttpHandler::handle_llama_stats(const httplib::Request& req, httplib::Response& res) {
    try {
        if (!llama_service_) {
            send_error_response(res, "LLaMA service is not available", 503);
            return;
        }
        
        auto status = llama_service_->get_status();
        auto stats = status.value("statistics", nlohmann::json{});
        
        send_json_response(res, stats);
        
    } catch (const std::exception& e) {
        spdlog::error("Error getting LLaMA stats: {}", e.what());
        send_error_response(res, "Failed to get LLaMA statistics", 500);
    }
}

} // namespace mcp