#pragma once

#include <string>
#include <memory>
#include <functional>
#include <nlohmann/json.hpp>

namespace mcp {

// MCP客户端响应结构
struct MCPResponse {
    bool success = false;
    nlohmann::json data;
    std::string error_message;
    int error_code = 0;
    
    // JSON转换
    void from_json(const nlohmann::json& j);
    nlohmann::json to_json() const;
};

// MCP客户端配置
struct MCPClientConfig {
    std::string server_host = "localhost";
    int server_port = 8080;
    std::string base_path = "/mcp";
    int timeout_seconds = 30;
    bool enable_ssl = false;
    std::string user_agent = "MCP-Client/1.0";
    
    // 认证配置
    std::string auth_token;
    std::string auth_header = "Authorization";
    
    // 重试配置
    int max_retries = 3;
    int retry_delay_ms = 1000;
    
    // 日志配置
    bool enable_logging = true;
    std::string log_level = "info";
};

// MCP客户端类
class MCPClient {
public:
    explicit MCPClient(const MCPClientConfig& config = MCPClientConfig{});
    ~MCPClient();
    
    // 禁用拷贝构造和赋值
    MCPClient(const MCPClient&) = delete;
    MCPClient& operator=(const MCPClient&) = delete;
    
    // 移动构造和赋值
    MCPClient(MCPClient&&) noexcept;
    MCPClient& operator=(MCPClient&&) noexcept;
    
    // 连接管理
    bool connect();
    void disconnect();
    bool is_connected() const;
    
    // MCP协议方法
    MCPResponse initialize(const std::string& client_name = "MCP-Client", const std::string& client_version = "1.0");
    MCPResponse list_tools();
    MCPResponse call_tool(const std::string& tool_name, const nlohmann::json& arguments = nlohmann::json::object());
    MCPResponse list_resources();
    MCPResponse read_resource(const std::string& uri);
    
    // 通用请求方法
    MCPResponse send_request(const nlohmann::json& request);
    
    // 配置管理
    void set_config(const MCPClientConfig& config);
    const MCPClientConfig& get_config() const;
    
    // 错误处理
    std::string get_last_error() const;
    void clear_error();
    
    // 回调函数类型
    using ErrorCallback = std::function<void(const std::string& error)>;
    using ResponseCallback = std::function<void(const MCPResponse& response)>;
    
    // 设置回调
    void set_error_callback(ErrorCallback callback);
    void set_response_callback(ResponseCallback callback);
    
    // 异步请求（如果需要）
    void send_request_async(const nlohmann::json& request, ResponseCallback callback);
    
    // 工具方法
    static std::string build_server_url(const std::string& host, int port, bool ssl = false);
    static nlohmann::json create_mcp_request(const std::string& method, const nlohmann::json& params = nlohmann::json::object());
    
private:
    class Impl;
    std::unique_ptr<Impl> pimpl_;
    
    // 内部方法
    void handle_error(const std::string& error);
    void handle_response(const MCPResponse& response);
    MCPResponse parse_response(const std::string& response_body);
    std::string build_request_url(const std::string& endpoint = "") const;
};

// 便利函数
namespace client_utils {
    // 创建标准的MCP请求
    nlohmann::json create_initialize_request(const std::string& client_name, const std::string& client_version);
    nlohmann::json create_list_tools_request();
    nlohmann::json create_call_tool_request(const std::string& tool_name, const nlohmann::json& arguments);
    nlohmann::json create_list_resources_request();
    nlohmann::json create_read_resource_request(const std::string& uri);
    
    // 响应解析
    bool is_success_response(const nlohmann::json& response);
    std::string extract_error_message(const nlohmann::json& response);
    nlohmann::json extract_result_data(const nlohmann::json& response);
    
    // 配置加载
    MCPClientConfig load_config_from_file(const std::string& file_path);
    MCPClientConfig load_config_from_json(const nlohmann::json& json);
    bool save_config_to_file(const MCPClientConfig& config, const std::string& file_path);
    
    // URL构建
    std::string build_http_url(const std::string& host, int port, const std::string& path = "/");
    std::string build_https_url(const std::string& host, int port, const std::string& path = "/");
    
    // 错误处理
    std::string format_error_message(const std::string& operation, const std::string& details);
    
    // 重试逻辑
    template<typename Func>
    auto retry_operation(Func&& func, int max_retries, int delay_ms) -> decltype(func());
}

} // namespace mcp