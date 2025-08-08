#include "mcp_client.hpp"
#include "http_client.hpp"
#include <spdlog/spdlog.h>
#include <random>
#include <thread>
#include <future>
#include <fstream>

namespace mcp {

// MCPResponse实现
void MCPResponse::from_json(const nlohmann::json& j) {
    try {
        if (j.contains("error")) {
            success = false;
            if (j["error"].contains("code")) {
                error_code = j["error"]["code"].get<int>();
            }
            if (j["error"].contains("message")) {
                error_message = j["error"]["message"].get<std::string>();
            }
        } else {
            success = true;
            if (j.contains("result")) {
                data = j["result"];
            } else {
                data = j;
            }
        }
    } catch (const std::exception& e) {
        success = false;
        error_message = "Failed to parse response: " + std::string(e.what());
        error_code = -1;
    }
}

nlohmann::json MCPResponse::to_json() const {
    nlohmann::json j;
    j["success"] = success;
    
    if (success) {
        j["result"] = data;
    } else {
        j["error"] = {
            {"code", error_code},
            {"message", error_message}
        };
    }
    
    return j;
}

// MCPClient::Impl类
class MCPClient::Impl {
public:
    MCPClientConfig config;
    std::unique_ptr<HttpClient> http_client;
    std::string last_error;
    bool connected = false;
    ErrorCallback error_callback;
    ResponseCallback response_callback;
    
    // 请求ID生成
    std::random_device rd;
    std::mt19937 gen;
    
    Impl(const MCPClientConfig& cfg) : config(cfg), gen(rd()) {
        HttpRequestConfig http_config;
        http_config.timeout = std::chrono::seconds(config.timeout_seconds);
        http_config.user_agent = config.user_agent;
        
        if (!config.auth_token.empty()) {
            http_config.headers[config.auth_header] = config.auth_token;
        }
        
        http_client = std::make_unique<HttpClient>(http_config);
    }
    
    std::string generate_request_id() {
        std::uniform_int_distribution<> dis(1000000, 9999999);
        return std::to_string(dis(gen));
    }
    
    std::string build_url(const std::string& endpoint = "") const {
        std::string protocol = config.enable_ssl ? "https" : "http";
        std::string url = protocol + "://" + config.server_host + ":" + std::to_string(config.server_port);
        
        if (!config.base_path.empty()) {
            if (config.base_path[0] != '/') {
                url += "/";
            }
            url += config.base_path;
        }
        
        if (!endpoint.empty()) {
            if (endpoint[0] != '/') {
                url += "/";
            }
            url += endpoint;
        }
        
        return url;
    }
};

// MCPClient实现
MCPClient::MCPClient(const MCPClientConfig& config) 
    : pimpl_(std::make_unique<Impl>(config)) {
    if (config.enable_logging) {
        spdlog::set_level(spdlog::level::from_str(config.log_level));
    }
}

MCPClient::~MCPClient() = default;

MCPClient::MCPClient(MCPClient&&) noexcept = default;
MCPClient& MCPClient::operator=(MCPClient&&) noexcept = default;

bool MCPClient::connect() {
    try {
        // 测试连接
        auto response = pimpl_->http_client->get(pimpl_->build_url("/health"));
        pimpl_->connected = response.is_success();
        
        if (pimpl_->connected) {
            spdlog::info("Connected to MCP server at {}:{}", 
                        pimpl_->config.server_host, pimpl_->config.server_port);
        } else {
            pimpl_->last_error = "Failed to connect to server: " + response.error_message;
            handle_error(pimpl_->last_error);
        }
        
        return pimpl_->connected;
        
    } catch (const std::exception& e) {
        pimpl_->last_error = "Connection error: " + std::string(e.what());
        handle_error(pimpl_->last_error);
        return false;
    }
}

void MCPClient::disconnect() {
    pimpl_->connected = false;
    spdlog::info("Disconnected from MCP server");
}

bool MCPClient::is_connected() const {
    return pimpl_->connected;
}

MCPResponse MCPClient::initialize(const std::string& client_name, const std::string& client_version) {
    auto request = client_utils::create_initialize_request(client_name, client_version);
    return send_request(request);
}

MCPResponse MCPClient::list_tools() {
    auto request = client_utils::create_list_tools_request();
    return send_request(request);
}

MCPResponse MCPClient::call_tool(const std::string& tool_name, const nlohmann::json& arguments) {
    auto request = client_utils::create_call_tool_request(tool_name, arguments);
    return send_request(request);
}

MCPResponse MCPClient::list_resources() {
    auto request = client_utils::create_list_resources_request();
    return send_request(request);
}

MCPResponse MCPClient::read_resource(const std::string& uri) {
    auto request = client_utils::create_read_resource_request(uri);
    return send_request(request);
}

MCPResponse MCPClient::send_request(const nlohmann::json& request) {
    MCPResponse mcp_response;
    
    try {
        std::string url = pimpl_->build_url();
        
        // 执行HTTP请求，支持重试
        HttpResponse http_response;
        int retries = 0;
        
        do {
            http_response = pimpl_->http_client->post(url, request);
            
            if (http_response.is_success()) {
                break;
            }
            
            if (retries < pimpl_->config.max_retries) {
                spdlog::warn("Request failed, retrying... ({}/{})", retries + 1, pimpl_->config.max_retries);
                std::this_thread::sleep_for(std::chrono::milliseconds(pimpl_->config.retry_delay_ms));
            }
            
            retries++;
        } while (retries <= pimpl_->config.max_retries);
        
        mcp_response = parse_response(http_response.body);
        
        if (!http_response.is_success()) {
            mcp_response.success = false;
            mcp_response.error_code = http_response.status_code;
            mcp_response.error_message = "HTTP Error: " + std::to_string(http_response.status_code) + " - " + http_response.error_message;
        }
        
        handle_response(mcp_response);
        
    } catch (const std::exception& e) {
        mcp_response.success = false;
        mcp_response.error_message = "Request failed: " + std::string(e.what());
        mcp_response.error_code = -1;
        handle_error(mcp_response.error_message);
    }
    
    return mcp_response;
}

void MCPClient::set_config(const MCPClientConfig& config) {
    pimpl_->config = config;
    
    // 更新HTTP客户端配置
    HttpRequestConfig http_config;
    http_config.timeout = std::chrono::seconds(config.timeout_seconds);
    http_config.user_agent = config.user_agent;
    
    if (!config.auth_token.empty()) {
        http_config.headers[config.auth_header] = config.auth_token;
    }
    
    pimpl_->http_client->set_config(http_config);
}

const MCPClientConfig& MCPClient::get_config() const {
    return pimpl_->config;
}

std::string MCPClient::get_last_error() const {
    return pimpl_->last_error;
}

void MCPClient::clear_error() {
    pimpl_->last_error.clear();
}

void MCPClient::set_error_callback(ErrorCallback callback) {
    pimpl_->error_callback = std::move(callback);
}

void MCPClient::set_response_callback(ResponseCallback callback) {
    pimpl_->response_callback = std::move(callback);
}

void MCPClient::send_request_async(const nlohmann::json& request, ResponseCallback callback) {
    std::thread([this, request, callback]() {
        auto response = send_request(request);
        if (callback) {
            callback(response);
        }
    }).detach();
}

std::string MCPClient::build_server_url(const std::string& host, int port, bool ssl) {
    std::string protocol = ssl ? "https" : "http";
    return protocol + "://" + host + ":" + std::to_string(port);
}

nlohmann::json MCPClient::create_mcp_request(const std::string& method, const nlohmann::json& params) {
    nlohmann::json request;
    request["jsonrpc"] = "2.0";
    request["method"] = method;
    request["params"] = params;
    
    // 生成请求ID
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(1000000, 9999999);
    request["id"] = dis(gen);
    
    return request;
}

void MCPClient::handle_error(const std::string& error) {
    pimpl_->last_error = error;
    spdlog::error("MCP Client Error: {}", error);
    
    if (pimpl_->error_callback) {
        pimpl_->error_callback(error);
    }
}

void MCPClient::handle_response(const MCPResponse& response) {
    if (pimpl_->response_callback) {
        pimpl_->response_callback(response);
    }
}

MCPResponse MCPClient::parse_response(const std::string& response_body) {
    MCPResponse response;
    
    try {
        if (response_body.empty()) {
            response.success = false;
            response.error_message = "Empty response body";
            response.error_code = -1;
            return response;
        }
        
        auto json = nlohmann::json::parse(response_body);
        response.from_json(json);
        
    } catch (const std::exception& e) {
        response.success = false;
        response.error_message = "Failed to parse JSON response: " + std::string(e.what());
        response.error_code = -1;
    }
    
    return response;
}

std::string MCPClient::build_request_url(const std::string& endpoint) const {
    return pimpl_->build_url(endpoint);
}

// client_utils命名空间实现
namespace client_utils {

nlohmann::json create_initialize_request(const std::string& client_name, const std::string& client_version) {
    nlohmann::json params;
    params["protocolVersion"] = "2024-11-05";
    params["capabilities"] = nlohmann::json::object();
    params["clientInfo"] = {
        {"name", client_name},
        {"version", client_version}
    };
    
    return MCPClient::create_mcp_request("initialize", params);
}

nlohmann::json create_list_tools_request() {
    return MCPClient::create_mcp_request("tools/list", nlohmann::json::object());
}

nlohmann::json create_call_tool_request(const std::string& tool_name, const nlohmann::json& arguments) {
    nlohmann::json params;
    params["name"] = tool_name;
    params["arguments"] = arguments;
    
    return MCPClient::create_mcp_request("tools/call", params);
}

nlohmann::json create_list_resources_request() {
    return MCPClient::create_mcp_request("resources/list", nlohmann::json::object());
}

nlohmann::json create_read_resource_request(const std::string& uri) {
    nlohmann::json params;
    params["uri"] = uri;
    
    return MCPClient::create_mcp_request("resources/read", params);
}

bool is_success_response(const nlohmann::json& response) {
    return !response.contains("error");
}

std::string extract_error_message(const nlohmann::json& response) {
    if (response.contains("error") && response["error"].contains("message")) {
        return response["error"]["message"].get<std::string>();
    }
    return "Unknown error";
}

nlohmann::json extract_result_data(const nlohmann::json& response) {
    if (response.contains("result")) {
        return response["result"];
    }
    return nlohmann::json::object();
}

MCPClientConfig load_config_from_file(const std::string& file_path) {
    MCPClientConfig config;
    
    try {
        std::ifstream file(file_path);
        if (file.is_open()) {
            nlohmann::json json;
            file >> json;
            config = load_config_from_json(json);
        }
    } catch (const std::exception& e) {
        spdlog::error("Failed to load config from file {}: {}", file_path, e.what());
    }
    
    return config;
}

MCPClientConfig load_config_from_json(const nlohmann::json& json) {
    MCPClientConfig config;
    
    try {
        if (json.contains("server_host")) {
            config.server_host = json["server_host"].get<std::string>();
        }
        if (json.contains("server_port")) {
            config.server_port = json["server_port"].get<int>();
        }
        if (json.contains("base_path")) {
            config.base_path = json["base_path"].get<std::string>();
        }
        if (json.contains("timeout_seconds")) {
            config.timeout_seconds = json["timeout_seconds"].get<int>();
        }
        if (json.contains("enable_ssl")) {
            config.enable_ssl = json["enable_ssl"].get<bool>();
        }
        if (json.contains("user_agent")) {
            config.user_agent = json["user_agent"].get<std::string>();
        }
        if (json.contains("auth_token")) {
            config.auth_token = json["auth_token"].get<std::string>();
        }
        if (json.contains("auth_header")) {
            config.auth_header = json["auth_header"].get<std::string>();
        }
        if (json.contains("max_retries")) {
            config.max_retries = json["max_retries"].get<int>();
        }
        if (json.contains("retry_delay_ms")) {
            config.retry_delay_ms = json["retry_delay_ms"].get<int>();
        }
        if (json.contains("enable_logging")) {
            config.enable_logging = json["enable_logging"].get<bool>();
        }
        if (json.contains("log_level")) {
            config.log_level = json["log_level"].get<std::string>();
        }
    } catch (const std::exception& e) {
        spdlog::error("Failed to parse config JSON: {}", e.what());
    }
    
    return config;
}

bool save_config_to_file(const MCPClientConfig& config, const std::string& file_path) {
    try {
        nlohmann::json json;
        json["server_host"] = config.server_host;
        json["server_port"] = config.server_port;
        json["base_path"] = config.base_path;
        json["timeout_seconds"] = config.timeout_seconds;
        json["enable_ssl"] = config.enable_ssl;
        json["user_agent"] = config.user_agent;
        json["auth_token"] = config.auth_token;
        json["auth_header"] = config.auth_header;
        json["max_retries"] = config.max_retries;
        json["retry_delay_ms"] = config.retry_delay_ms;
        json["enable_logging"] = config.enable_logging;
        json["log_level"] = config.log_level;
        
        std::ofstream file(file_path);
        if (file.is_open()) {
            file << json.dump(2);
            return true;
        }
    } catch (const std::exception& e) {
        spdlog::error("Failed to save config to file {}: {}", file_path, e.what());
    }
    
    return false;
}

std::string build_http_url(const std::string& host, int port, const std::string& path) {
    return "http://" + host + ":" + std::to_string(port) + path;
}

std::string build_https_url(const std::string& host, int port, const std::string& path) {
    return "https://" + host + ":" + std::to_string(port) + path;
}

std::string format_error_message(const std::string& operation, const std::string& details) {
    return "Operation '" + operation + "' failed: " + details;
}

} // namespace client_utils

} // namespace mcp