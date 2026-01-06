#pragma once

#include <string>
#include <vector>
#include <nlohmann/json.hpp>

namespace mcp {

// 配置类
class Config {
public:
    // 单例模式
    static Config& instance();
    
    // 加载配置
    bool load_from_file(const std::string& config_path);
    bool load_from_json(const nlohmann::json& config);
    
    // 服务器配置
    std::string get_host() const { return host_; }
    int get_port() const { return port_; }
    
    // 数据库配置
    std::string get_database_path() const { return database_path_; }
    
    // 日志配置
    std::string get_log_level() const { return log_level_; }
    std::string get_log_file() const { return log_file_; }
    
    // 内容配置
    int get_max_content_size() const { return max_content_size_; }
    int get_default_page_size() const { return default_page_size_; }
    int get_max_page_size() const { return max_page_size_; }
    
    // 安全配置
    bool is_cors_enabled() const { return enable_cors_; }
    std::string get_cors_origin() const { return cors_origin_; }
    
    // 静态文件配置
    std::string get_static_files_path() const { return static_files_path_; }
    bool is_static_files_enabled() const { return enable_static_files_; }
    
    // 文件上传配置
    std::string get_upload_path() const { return upload_path_; }
    int get_max_file_size() const { return max_file_size_; }
    std::vector<std::string> get_allowed_file_types() const { return allowed_file_types_; }
    bool is_file_upload_enabled() const { return enable_file_upload_; }
    
    // LLaMA.cpp配置
    std::string get_llama_model_path() const { return llama_model_path_; }
    std::string get_llama_executable_path() const { return llama_executable_path_; }
    int get_llama_context_size() const { return llama_context_size_; }
    int get_llama_threads() const { return llama_threads_; }
    float get_llama_temperature() const { return llama_temperature_; }
    int get_llama_max_tokens() const { return llama_max_tokens_; }
    bool is_llama_enabled() const { return enable_llama_; }
    
    // Ollama配置
    std::string get_ollama_host() const { return ollama_host_; }
    int get_ollama_port() const { return ollama_port_; }
    std::string get_ollama_model() const { return ollama_model_; }
    float get_ollama_temperature() const { return ollama_temperature_; }
    int get_ollama_max_tokens() const { return ollama_max_tokens_; }
    int get_ollama_timeout() const { return ollama_timeout_; }
    bool is_ollama_enabled() const { return enable_ollama_; }
    
    // 服务端配置管理
    bool update_config(const nlohmann::json& new_config);
    bool save_config_to_file(const std::string& config_path) const;
    
    // 设置配置（用于测试）
    void set_host(const std::string& host) { host_ = host; }
    void set_port(int port) { port_ = port; }
    void set_database_path(const std::string& path) { database_path_ = path; }
    
    // 验证配置
    bool validate() const;
    
    // 获取完整配置
    nlohmann::json to_json() const;
    
private:
    Config() = default;
    
    // 服务器配置
    std::string host_ = "127.0.0.1";
    int port_ = 8086;
    
    // 数据库配置
    std::string database_path_ = "./data/content.db";
    
    // 日志配置
    std::string log_level_ = "info";
    std::string log_file_ = "";
    
    // 内容配置
    int max_content_size_ = 1024 * 1024; // 1MB
    int default_page_size_ = 20;
    int max_page_size_ = 100;
    
    // 安全配置
    bool enable_cors_ = true;
    std::string cors_origin_ = "*";
    
    // 静态文件配置
    std::string static_files_path_ = "./web";
    bool enable_static_files_ = true;
    
    // 文件上传配置
    std::string upload_path_ = "./uploads";
    int max_file_size_ = 10 * 1024 * 1024; // 10MB
    std::vector<std::string> allowed_file_types_ = {".txt", ".md", ".pdf", ".doc", ".docx", ".jpg", ".png", ".gif"};
    bool enable_file_upload_ = true;
    
    // LLaMA.cpp配置
    std::string llama_model_path_ = "";
    std::string llama_executable_path_ = "./llama.cpp/main";
    int llama_context_size_ = 2048;
    int llama_threads_ = 4;
    float llama_temperature_ = 0.7f;
    int llama_max_tokens_ = 512;
    bool enable_llama_ = false;
    
    // Ollama配置
    std::string ollama_host_ = "localhost";
    int ollama_port_ = 11434;
    std::string ollama_model_ = "llama2";
    float ollama_temperature_ = 0.7f;
    int ollama_max_tokens_ = 512;
    int ollama_timeout_ = 30;
    bool enable_ollama_ = false;
    
    // 辅助方法
    void load_defaults();
    void apply_config(const nlohmann::json& config);
    std::string current_config_path_;
};

} // namespace mcp