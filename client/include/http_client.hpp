#pragma once

#include <string>
#include <map>
#include <memory>
#include <chrono>
#include <nlohmann/json.hpp>

namespace mcp {

// HTTP响应结构
struct HttpResponse {
    int status_code = 0;
    std::string body;
    std::map<std::string, std::string> headers;
    bool success = false;
    std::string error_message;
    std::chrono::milliseconds response_time{0};
    
    // 便利方法
    bool is_success() const { return success && status_code >= 200 && status_code < 300; }
    bool is_json() const;
    nlohmann::json get_json() const;
    std::string get_header(const std::string& name, const std::string& default_value = "") const;
};

// HTTP请求配置
struct HttpRequestConfig {
    std::map<std::string, std::string> headers;
    std::chrono::seconds timeout{30};
    bool follow_redirects = true;
    int max_redirects = 5;
    bool verify_ssl = true;
    std::string user_agent = "MCP-HTTP-Client/1.0";
    
    // 认证
    std::string auth_token;
    std::string auth_type = "Bearer"; // Bearer, Basic, etc.
    
    // 代理设置
    std::string proxy_host;
    int proxy_port = 0;
    std::string proxy_username;
    std::string proxy_password;
    
    // 重试设置
    int max_retries = 0;
    std::chrono::milliseconds retry_delay{1000};
    
    // 压缩
    bool enable_compression = true;
};

// HTTP客户端类
class HttpClient {
public:
    explicit HttpClient(const HttpRequestConfig& config = HttpRequestConfig{});
    ~HttpClient();
    
    // 禁用拷贝构造和赋值
    HttpClient(const HttpClient&) = delete;
    HttpClient& operator=(const HttpClient&) = delete;
    
    // 移动构造和赋值
    HttpClient(HttpClient&&) noexcept;
    HttpClient& operator=(HttpClient&&) noexcept;
    
    // HTTP方法
    HttpResponse get(const std::string& url, const std::map<std::string, std::string>& params = {});
    HttpResponse post(const std::string& url, const std::string& body, const std::string& content_type = "application/json");
    HttpResponse post(const std::string& url, const nlohmann::json& json);
    HttpResponse put(const std::string& url, const std::string& body, const std::string& content_type = "application/json");
    HttpResponse put(const std::string& url, const nlohmann::json& json);
    HttpResponse patch(const std::string& url, const std::string& body, const std::string& content_type = "application/json");
    HttpResponse patch(const std::string& url, const nlohmann::json& json);
    HttpResponse delete_request(const std::string& url);
    HttpResponse head(const std::string& url);
    HttpResponse options(const std::string& url);
    
    // 通用请求方法
    HttpResponse request(const std::string& method, const std::string& url, 
                        const std::string& body = "", 
                        const std::map<std::string, std::string>& headers = {});
    
    // 配置管理
    void set_config(const HttpRequestConfig& config);
    const HttpRequestConfig& get_config() const;
    
    // 头部管理
    void set_header(const std::string& name, const std::string& value);
    void remove_header(const std::string& name);
    void clear_headers();
    
    // 认证
    void set_bearer_token(const std::string& token);
    void set_basic_auth(const std::string& username, const std::string& password);
    void clear_auth();
    
    // 超时设置
    void set_timeout(std::chrono::seconds timeout);
    
    // 代理设置
    void set_proxy(const std::string& host, int port, 
                   const std::string& username = "", 
                   const std::string& password = "");
    void clear_proxy();
    
    // SSL设置
    void set_ssl_verification(bool verify);
    
    // 错误处理
    std::string get_last_error() const;
    void clear_error();
    
    // 统计信息
    struct Statistics {
        size_t total_requests = 0;
        size_t successful_requests = 0;
        size_t failed_requests = 0;
        std::chrono::milliseconds total_response_time{0};
        std::chrono::milliseconds average_response_time{0};
        
        void update(const HttpResponse& response);
        void reset();
        nlohmann::json to_json() const;
    };
    
    const Statistics& get_statistics() const;
    void reset_statistics();
    
private:
    class Impl;
    std::unique_ptr<Impl> pimpl_;
    
    // 内部方法
    HttpResponse execute_request(const std::string& method, const std::string& url, 
                                const std::string& body, 
                                const std::map<std::string, std::string>& headers);
    HttpResponse handle_response(int status_code, const std::string& body, 
                                const std::map<std::string, std::string>& headers);
    void handle_error(const std::string& error);
    std::string build_query_string(const std::map<std::string, std::string>& params) const;
    std::string url_encode(const std::string& value) const;
};

// 便利函数
namespace http_utils {
    // URL处理
    std::string build_url(const std::string& base_url, const std::string& path);
    std::string add_query_params(const std::string& url, const std::map<std::string, std::string>& params);
    bool is_valid_url(const std::string& url);
    
    // 头部处理
    std::map<std::string, std::string> parse_headers(const std::string& headers_string);
    std::string format_headers(const std::map<std::string, std::string>& headers);
    
    // 内容类型
    std::string get_content_type_for_json();
    std::string get_content_type_for_form();
    std::string get_content_type_for_text();
    
    // 状态码处理
    bool is_success_status(int status_code);
    bool is_client_error_status(int status_code);
    bool is_server_error_status(int status_code);
    std::string get_status_message(int status_code);
    
    // 编码处理
    std::string url_encode(const std::string& value);
    std::string url_decode(const std::string& value);
    std::string base64_encode(const std::string& value);
    std::string base64_decode(const std::string& value);
    
    // JSON处理
    bool is_json_content_type(const std::string& content_type);
    nlohmann::json parse_json_response(const std::string& body);
    
    // 错误处理
    std::string format_http_error(int status_code, const std::string& message);
    
    // 重试逻辑
    template<typename Func>
    auto retry_http_request(Func&& func, int max_retries, std::chrono::milliseconds delay) -> decltype(func());
    
    // 超时处理
    class TimeoutGuard {
    public:
        explicit TimeoutGuard(std::chrono::seconds timeout);
        ~TimeoutGuard();
        bool is_timeout() const;
        
    private:
        std::chrono::steady_clock::time_point start_time_;
        std::chrono::seconds timeout_;
    };
}

} // namespace mcp