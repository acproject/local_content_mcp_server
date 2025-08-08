#pragma once

#include <string>
#include <vector>
#include <memory>
#include <nlohmann/json.hpp>
#include <future>

namespace mcp {

// LLaMA 请求参数
struct LlamaRequest {
    std::string prompt;
    int max_tokens = 512;
    float temperature = 0.7f;
    float top_p = 0.9f;
    int top_k = 40;
    std::vector<std::string> stop_sequences;
    bool stream = false;
    
    nlohmann::json to_json() const;
    void from_json(const nlohmann::json& j);
};

// LLaMA 响应
struct LlamaResponse {
    bool success;
    std::string text;
    std::string error_message;
    int tokens_generated;
    double generation_time;
    
    nlohmann::json to_json() const;
};

// LLaMA 模型信息
struct ModelInfo {
    std::string model_path;
    std::string model_name;
    bool is_loaded;
    size_t context_size;
    size_t vocab_size;
    std::string architecture;
    
    nlohmann::json to_json() const;
};

// LLaMA 客户端
class LlamaClient {
public:
    LlamaClient();
    ~LlamaClient();
    
    // 初始化和配置
    bool initialize();
    bool load_model(const std::string& model_path);
    bool unload_model();
    bool is_model_loaded() const;
    
    // 文本生成
    LlamaResponse generate(const LlamaRequest& request);
    std::future<LlamaResponse> generate_async(const LlamaRequest& request);
    
    // 流式生成（回调函数接收每个token）
    bool generate_stream(const LlamaRequest& request, 
                        std::function<bool(const std::string& token)> callback);
    
    // 模型信息
    ModelInfo get_model_info() const;
    
    // 配置管理
    bool update_config(const nlohmann::json& config);
    nlohmann::json get_config() const;
    
    // 健康检查
    bool health_check();
    
    // 统计信息
    nlohmann::json get_statistics();
    void reset_statistics();
    
private:
    class Impl;
    std::unique_ptr<Impl> pimpl_;
    
    // 辅助方法
    std::string escape_shell_arg(const std::string& arg);
    std::vector<std::string> build_command_args(const LlamaRequest& request);
    LlamaResponse parse_output(const std::string& output, double generation_time);
};

// LLaMA 服务管理器
class LlamaService {
public:
    static LlamaService& instance();
    
    // 服务管理
    bool start();
    bool stop();
    bool restart();
    bool is_running() const;
    
    // 请求处理
    LlamaResponse process_request(const LlamaRequest& request);
    std::future<LlamaResponse> process_request_async(const LlamaRequest& request);
    
    // 配置
    bool update_config(const nlohmann::json& config);
    nlohmann::json get_status();
    
private:
    LlamaService() = default;
    std::unique_ptr<LlamaClient> client_;
    bool running_ = false;
    mutable std::mutex mutex_;
    
    // 统计信息
    struct Statistics {
        size_t total_requests = 0;
        size_t successful_requests = 0;
        size_t failed_requests = 0;
        double total_generation_time = 0.0;
        size_t total_tokens_generated = 0;
        
        nlohmann::json to_json() const;
        void update(const LlamaResponse& response);
        void reset();
    } stats_;
};

} // namespace mcp