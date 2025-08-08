#include "http_client.hpp"
#include <httplib.h>
#include <spdlog/spdlog.h>
#include <chrono>
#include <thread>
#include <regex>
#include <sstream>
#include <iomanip>

namespace mcp {

// HttpResponse实现
bool HttpResponse::is_json() const {
    auto it = headers.find("content-type");
    if (it != headers.end()) {
        return it->second.find("application/json") != std::string::npos;
    }
    return false;
}

nlohmann::json HttpResponse::get_json() const {
    try {
        return nlohmann::json::parse(body);
    } catch (const std::exception& e) {
        throw std::runtime_error("Failed to parse JSON response: " + std::string(e.what()));
    }
}

std::string HttpResponse::get_header(const std::string& name, const std::string& default_value) const {
    auto it = headers.find(name);
    if (it != headers.end()) {
        return it->second;
    }
    return default_value;
}

// Statistics实现
void HttpClient::Statistics::update(const HttpResponse& response) {
    total_requests++;
    if (response.success) {
        successful_requests++;
    } else {
        failed_requests++;
    }
    
    total_response_time += response.response_time;
    if (total_requests > 0) {
        average_response_time = total_response_time / total_requests;
    }
}

void HttpClient::Statistics::reset() {
    total_requests = 0;
    successful_requests = 0;
    failed_requests = 0;
    total_response_time = std::chrono::milliseconds{0};
    average_response_time = std::chrono::milliseconds{0};
}

nlohmann::json HttpClient::Statistics::to_json() const {
    nlohmann::json j;
    j["total_requests"] = total_requests;
    j["successful_requests"] = successful_requests;
    j["failed_requests"] = failed_requests;
    j["total_response_time_ms"] = total_response_time.count();
    j["average_response_time_ms"] = average_response_time.count();
    return j;
}

// HttpClient::Impl类
class HttpClient::Impl {
public:
    HttpRequestConfig config;
    std::unique_ptr<httplib::Client> client;
    std::string last_error;
    Statistics stats;
    
    explicit Impl(const HttpRequestConfig& cfg) : config(cfg) {
        // httplib客户端将在第一次请求时创建
    }
    
    std::unique_ptr<httplib::Client> create_client(const std::string& url) {
        // 解析URL
        std::regex url_regex(R"(^(https?)://([^:/]+)(?::(\d+))?(.*)$)");
        std::smatch matches;
        
        if (!std::regex_match(url, matches, url_regex)) {
            throw std::runtime_error("Invalid URL: " + url);
        }
        
        std::string scheme = matches[1].str();
        std::string host = matches[2].str();
        std::string port_str = matches[3].str();
        
        int port = 80;
        if (scheme == "https") {
            port = 443;
        }
        if (!port_str.empty()) {
            port = std::stoi(port_str);
        }
        
        auto client = std::make_unique<httplib::Client>(host, port);
        
        // 配置客户端
        client->set_connection_timeout(config.timeout);
        client->set_read_timeout(config.timeout);
        client->set_write_timeout(config.timeout);
        
        if (scheme == "https") {
            client->enable_server_certificate_verification(config.verify_ssl);
        }
        
        if (config.follow_redirects) {
            client->set_follow_location(true);
        }
        
        if (config.enable_compression) {
            client->set_compress(true);
        }
        
        // 设置代理
        if (!config.proxy_host.empty() && config.proxy_port > 0) {
            client->set_proxy(config.proxy_host, config.proxy_port);
            if (!config.proxy_username.empty()) {
                client->set_proxy_basic_auth(config.proxy_username, config.proxy_password);
            }
        }
        
        return client;
    }
    
    HttpResponse execute_with_retry(std::function<HttpResponse()> request_func) {
        HttpResponse response;
        int attempts = 0;
        
        do {
            response = request_func();
            
            if (response.success || attempts >= config.max_retries) {
                break;
            }
            
            attempts++;
            spdlog::warn("HTTP request failed, retrying... ({}/{})", attempts, config.max_retries);
            std::this_thread::sleep_for(config.retry_delay);
            
        } while (attempts <= config.max_retries);
        
        return response;
    }
};

// HttpClient实现
HttpClient::HttpClient(const HttpRequestConfig& config) 
    : pimpl_(std::make_unique<Impl>(config)) {
}

HttpClient::~HttpClient() = default;

HttpClient::HttpClient(HttpClient&&) noexcept = default;
HttpClient& HttpClient::operator=(HttpClient&&) noexcept = default;

HttpResponse HttpClient::get(const std::string& url, const std::map<std::string, std::string>& params) {
    std::string full_url = url;
    if (!params.empty()) {
        full_url += "?" + build_query_string(params);
    }
    
    return request("GET", full_url);
}

HttpResponse HttpClient::post(const std::string& url, const std::string& body, const std::string& content_type) {
    std::map<std::string, std::string> headers;
    headers["Content-Type"] = content_type;
    return request("POST", url, body, headers);
}

HttpResponse HttpClient::post(const std::string& url, const nlohmann::json& json) {
    return post(url, json.dump(), "application/json");
}

HttpResponse HttpClient::put(const std::string& url, const std::string& body, const std::string& content_type) {
    std::map<std::string, std::string> headers;
    headers["Content-Type"] = content_type;
    return request("PUT", url, body, headers);
}

HttpResponse HttpClient::put(const std::string& url, const nlohmann::json& json) {
    return put(url, json.dump(), "application/json");
}

HttpResponse HttpClient::patch(const std::string& url, const std::string& body, const std::string& content_type) {
    std::map<std::string, std::string> headers;
    headers["Content-Type"] = content_type;
    return request("PATCH", url, body, headers);
}

HttpResponse HttpClient::patch(const std::string& url, const nlohmann::json& json) {
    return patch(url, json.dump(), "application/json");
}

HttpResponse HttpClient::delete_request(const std::string& url) {
    return request("DELETE", url);
}

HttpResponse HttpClient::head(const std::string& url) {
    return request("HEAD", url);
}

HttpResponse HttpClient::options(const std::string& url) {
    return request("OPTIONS", url);
}

HttpResponse HttpClient::request(const std::string& method, const std::string& url, 
                                const std::string& body, 
                                const std::map<std::string, std::string>& headers) {
    return pimpl_->execute_with_retry([this, &method, &url, &body, &headers]() {
        return execute_request(method, url, body, headers);
    });
}

void HttpClient::set_config(const HttpRequestConfig& config) {
    pimpl_->config = config;
}

const HttpRequestConfig& HttpClient::get_config() const {
    return pimpl_->config;
}

void HttpClient::set_header(const std::string& name, const std::string& value) {
    pimpl_->config.headers[name] = value;
}

void HttpClient::remove_header(const std::string& name) {
    pimpl_->config.headers.erase(name);
}

void HttpClient::clear_headers() {
    pimpl_->config.headers.clear();
}

void HttpClient::set_bearer_token(const std::string& token) {
    pimpl_->config.auth_token = token;
    pimpl_->config.auth_type = "Bearer";
    set_header("Authorization", "Bearer " + token);
}

void HttpClient::set_basic_auth(const std::string& username, const std::string& password) {
    std::string credentials = username + ":" + password;
    std::string encoded = http_utils::base64_encode(credentials);
    set_header("Authorization", "Basic " + encoded);
}

void HttpClient::clear_auth() {
    pimpl_->config.auth_token.clear();
    remove_header("Authorization");
}

void HttpClient::set_timeout(std::chrono::seconds timeout) {
    pimpl_->config.timeout = timeout;
}

void HttpClient::set_proxy(const std::string& host, int port, 
                          const std::string& username, 
                          const std::string& password) {
    pimpl_->config.proxy_host = host;
    pimpl_->config.proxy_port = port;
    pimpl_->config.proxy_username = username;
    pimpl_->config.proxy_password = password;
}

void HttpClient::clear_proxy() {
    pimpl_->config.proxy_host.clear();
    pimpl_->config.proxy_port = 0;
    pimpl_->config.proxy_username.clear();
    pimpl_->config.proxy_password.clear();
}

void HttpClient::set_ssl_verification(bool verify) {
    pimpl_->config.verify_ssl = verify;
}

std::string HttpClient::get_last_error() const {
    return pimpl_->last_error;
}

void HttpClient::clear_error() {
    pimpl_->last_error.clear();
}

const HttpClient::Statistics& HttpClient::get_statistics() const {
    return pimpl_->stats;
}

void HttpClient::reset_statistics() {
    pimpl_->stats.reset();
}

HttpResponse HttpClient::execute_request(const std::string& method, const std::string& url, 
                                        const std::string& body, 
                                        const std::map<std::string, std::string>& headers) {
    HttpResponse response;
    auto start_time = std::chrono::steady_clock::now();
    
    try {
        auto client = pimpl_->create_client(url);
        
        // 解析URL路径
        std::regex url_regex(R"(^https?://[^/]+(.*)$)");
        std::smatch matches;
        std::string path = "/";
        
        if (std::regex_match(url, matches, url_regex)) {
            path = matches[1].str();
            if (path.empty()) {
                path = "/";
            }
        }
        
        // 准备头部
        httplib::Headers http_headers;
        
        // 添加默认头部
        http_headers.emplace("User-Agent", pimpl_->config.user_agent);
        
        // 添加配置中的头部
        for (const auto& [key, value] : pimpl_->config.headers) {
            http_headers.emplace(key, value);
        }
        
        // 添加请求特定的头部
        for (const auto& [key, value] : headers) {
            http_headers.emplace(key, value);
        }
        
        // 执行请求
        httplib::Result result;
        
        if (method == "GET") {
            result = client->Get(path, http_headers);
        } else if (method == "POST") {
            result = client->Post(path, http_headers, body, headers.count("Content-Type") ? headers.at("Content-Type") : "application/json");
        } else if (method == "PUT") {
            result = client->Put(path, http_headers, body, headers.count("Content-Type") ? headers.at("Content-Type") : "application/json");
        } else if (method == "PATCH") {
            result = client->Patch(path, http_headers, body, headers.count("Content-Type") ? headers.at("Content-Type") : "application/json");
        } else if (method == "DELETE") {
            result = client->Delete(path, http_headers);
        } else if (method == "HEAD") {
            result = client->Head(path, http_headers);
        } else if (method == "OPTIONS") {
            result = client->Options(path, http_headers);
        } else {
            throw std::runtime_error("Unsupported HTTP method: " + method);
        }
        
        auto end_time = std::chrono::steady_clock::now();
        response.response_time = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        
        if (result) {
            response.status_code = result->status;
            response.body = result->body;
            response.success = true;
            
            // 转换头部
            for (const auto& [key, value] : result->headers) {
                response.headers[key] = value;
            }
        } else {
            response.success = false;
            response.error_message = "HTTP request failed: " + httplib::to_string(result.error());
            handle_error(response.error_message);
        }
        
    } catch (const std::exception& e) {
        auto end_time = std::chrono::steady_clock::now();
        response.response_time = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        
        response.success = false;
        response.error_message = "Request exception: " + std::string(e.what());
        handle_error(response.error_message);
    }
    
    // 更新统计信息
    pimpl_->stats.update(response);
    
    return response;
}

HttpResponse HttpClient::handle_response(int status_code, const std::string& body, 
                                        const std::map<std::string, std::string>& headers) {
    HttpResponse response;
    response.status_code = status_code;
    response.body = body;
    response.headers = headers;
    response.success = (status_code >= 200 && status_code < 300);
    
    if (!response.success) {
        response.error_message = "HTTP " + std::to_string(status_code) + ": " + http_utils::get_status_message(status_code);
    }
    
    return response;
}

void HttpClient::handle_error(const std::string& error) {
    pimpl_->last_error = error;
    spdlog::error("HTTP Client Error: {}", error);
}

std::string HttpClient::build_query_string(const std::map<std::string, std::string>& params) const {
    std::ostringstream oss;
    bool first = true;
    
    for (const auto& [key, value] : params) {
        if (!first) {
            oss << "&";
        }
        oss << url_encode(key) << "=" << url_encode(value);
        first = false;
    }
    
    return oss.str();
}

std::string HttpClient::url_encode(const std::string& value) const {
    return http_utils::url_encode(value);
}

// http_utils命名空间实现
namespace http_utils {

std::string build_url(const std::string& base_url, const std::string& path) {
    std::string url = base_url;
    if (!url.empty() && url.back() == '/') {
        url.pop_back();
    }
    
    std::string full_path = path;
    if (!full_path.empty() && full_path.front() != '/') {
        full_path = "/" + full_path;
    }
    
    return url + full_path;
}

std::string add_query_params(const std::string& url, const std::map<std::string, std::string>& params) {
    if (params.empty()) {
        return url;
    }
    
    std::ostringstream oss;
    oss << url;
    
    if (url.find('?') == std::string::npos) {
        oss << "?";
    } else {
        oss << "&";
    }
    
    bool first = true;
    for (const auto& [key, value] : params) {
        if (!first) {
            oss << "&";
        }
        oss << url_encode(key) << "=" << url_encode(value);
        first = false;
    }
    
    return oss.str();
}

bool is_valid_url(const std::string& url) {
    std::regex url_regex(R"(^https?://[^\s/$.?#].[^\s]*$)");
    return std::regex_match(url, url_regex);
}

std::string get_content_type_for_json() {
    return "application/json";
}

std::string get_content_type_for_form() {
    return "application/x-www-form-urlencoded";
}

std::string get_content_type_for_text() {
    return "text/plain";
}

bool is_success_status(int status_code) {
    return status_code >= 200 && status_code < 300;
}

bool is_client_error_status(int status_code) {
    return status_code >= 400 && status_code < 500;
}

bool is_server_error_status(int status_code) {
    return status_code >= 500 && status_code < 600;
}

std::string get_status_message(int status_code) {
    switch (status_code) {
        case 200: return "OK";
        case 201: return "Created";
        case 204: return "No Content";
        case 400: return "Bad Request";
        case 401: return "Unauthorized";
        case 403: return "Forbidden";
        case 404: return "Not Found";
        case 405: return "Method Not Allowed";
        case 409: return "Conflict";
        case 422: return "Unprocessable Entity";
        case 500: return "Internal Server Error";
        case 502: return "Bad Gateway";
        case 503: return "Service Unavailable";
        case 504: return "Gateway Timeout";
        default: return "Unknown Status";
    }
}

std::string url_encode(const std::string& value) {
    std::ostringstream encoded;
    encoded.fill('0');
    encoded << std::hex;
    
    for (char c : value) {
        if (std::isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            encoded << c;
        } else {
            encoded << std::uppercase;
            encoded << '%' << std::setw(2) << static_cast<int>(static_cast<unsigned char>(c));
            encoded << std::nouppercase;
        }
    }
    
    return encoded.str();
}

std::string url_decode(const std::string& value) {
    std::string decoded;
    for (size_t i = 0; i < value.length(); ++i) {
        if (value[i] == '%' && i + 2 < value.length()) {
            int hex_value;
            std::istringstream hex_stream(value.substr(i + 1, 2));
            if (hex_stream >> std::hex >> hex_value) {
                decoded += static_cast<char>(hex_value);
                i += 2;
            } else {
                decoded += value[i];
            }
        } else if (value[i] == '+') {
            decoded += ' ';
        } else {
            decoded += value[i];
        }
    }
    return decoded;
}

std::string base64_encode(const std::string& value) {
    // 简单的base64编码实现
    const std::string chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string encoded;
    int val = 0, valb = -6;
    
    for (unsigned char c : value) {
        val = (val << 8) + c;
        valb += 8;
        while (valb >= 0) {
            encoded.push_back(chars[(val >> valb) & 0x3F]);
            valb -= 6;
        }
    }
    
    if (valb > -6) {
        encoded.push_back(chars[((val << 8) >> (valb + 8)) & 0x3F]);
    }
    
    while (encoded.size() % 4) {
        encoded.push_back('=');
    }
    
    return encoded;
}

std::string base64_decode(const std::string& value) {
    // 简单的base64解码实现
    const std::string chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string decoded;
    int val = 0, valb = -8;
    
    for (char c : value) {
        if (c == '=') break;
        size_t pos = chars.find(c);
        if (pos == std::string::npos) continue;
        
        val = (val << 6) + pos;
        valb += 6;
        if (valb >= 0) {
            decoded.push_back(char((val >> valb) & 0xFF));
            valb -= 8;
        }
    }
    
    return decoded;
}

bool is_json_content_type(const std::string& content_type) {
    return content_type.find("application/json") != std::string::npos;
}

nlohmann::json parse_json_response(const std::string& body) {
    try {
        return nlohmann::json::parse(body);
    } catch (const std::exception& e) {
        throw std::runtime_error("Failed to parse JSON: " + std::string(e.what()));
    }
}

std::string format_http_error(int status_code, const std::string& message) {
    return "HTTP " + std::to_string(status_code) + ": " + message;
}

// TimeoutGuard实现
TimeoutGuard::TimeoutGuard(std::chrono::seconds timeout) 
    : start_time_(std::chrono::steady_clock::now()), timeout_(timeout) {
}

TimeoutGuard::~TimeoutGuard() = default;

bool TimeoutGuard::is_timeout() const {
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - start_time_);
    return elapsed >= timeout_;
}

} // namespace http_utils

} // namespace mcp