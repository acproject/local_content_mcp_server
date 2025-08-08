#pragma once

#include "mcp_server.hpp"
#include <httplib.h>
#include <memory>
#include <string>

namespace mcp {

// HTTP处理器类
class HttpHandler {
public:
    explicit HttpHandler(std::shared_ptr<MCPServer> mcp_server);
    
    // 启动HTTP服务器
    bool start(const std::string& host, int port);
    void stop();
    
    // 获取服务器状态
    bool is_running() const { return server_ && server_->is_running(); }
    
private:
    std::shared_ptr<MCPServer> mcp_server_;
    std::unique_ptr<httplib::Server> server_;
    
    // 路由处理函数
    void setup_routes();
    
    // MCP协议端点
    void handle_mcp_request(const httplib::Request& req, httplib::Response& res);
    
    // RESTful API端点
    void handle_get_content(const httplib::Request& req, httplib::Response& res);
    void handle_create_content(const httplib::Request& req, httplib::Response& res);
    void handle_update_content(const httplib::Request& req, httplib::Response& res);
    void handle_delete_content(const httplib::Request& req, httplib::Response& res);
    void handle_search_content(const httplib::Request& req, httplib::Response& res);
    void handle_list_content(const httplib::Request& req, httplib::Response& res);
    void handle_get_tags(const httplib::Request& req, httplib::Response& res);
    void handle_get_statistics(const httplib::Request& req, httplib::Response& res);
    
    // 健康检查和信息端点
    void handle_health_check(const httplib::Request& req, httplib::Response& res);
    void handle_server_info(const httplib::Request& req, httplib::Response& res);
    
    // 静态文件服务（用于客户端界面）
    void handle_static_files(const httplib::Request& req, httplib::Response& res);
    
    // 辅助方法
    void set_cors_headers(httplib::Response& res);
    void send_json_response(httplib::Response& res, const nlohmann::json& json, int status = 200);
    void send_error_response(httplib::Response& res, const std::string& message, int status = 400);
    bool parse_json_body(const std::string& body, nlohmann::json& json, std::string& error_msg);
    int parse_int_param(const httplib::Request& req, const std::string& param, int default_value = 0);
    std::string get_param(const httplib::Request& req, const std::string& param, const std::string& default_value = "");
};

} // namespace mcp