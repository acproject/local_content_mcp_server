#include "config.hpp"
#include <fstream>
#include <spdlog/spdlog.h>

namespace mcp {

Config& Config::instance() {
    static Config instance;
    return instance;
}

bool Config::load_from_file(const std::string& config_path) {
    try {
        std::ifstream file(config_path);
        if (!file.is_open()) {
            spdlog::error("Failed to open config file: {}", config_path);
            load_defaults();
            return false;
        }
        
        nlohmann::json config;
        file >> config;
        current_config_path_ = config_path;
        return load_from_json(config);
    } catch (const std::exception& e) {
        spdlog::error("Failed to load config from file {}: {}", config_path, e.what());
        load_defaults();
        return false;
    }
}

bool Config::load_from_json(const nlohmann::json& config) {
    try {
        // 先设置默认值
        load_defaults();
        
        // 应用配置
        apply_config(config);
        
        return validate();
    } catch (const std::exception& e) {
        spdlog::error("Failed to load config from JSON: {}", e.what());
        return false;
    }
}

bool Config::validate() const {
    // 验证端口范围
    if (port_ < 1 || port_ > 65535) {
        spdlog::error("Invalid port: {}", port_);
        return false;
    }
    
    // 验证主机地址
    if (host_.empty()) {
        spdlog::error("Host cannot be empty");
        return false;
    }
    
    // 验证数据库路径
    if (database_path_.empty()) {
        spdlog::error("Database path cannot be empty");
        return false;
    }
    
    // 验证内容大小限制
    if (max_content_size_ <= 0) {
        spdlog::error("Max content size must be positive");
        return false;
    }
    
    // 验证分页参数
    if (default_page_size_ <= 0 || max_page_size_ <= 0) {
        spdlog::error("Page sizes must be positive");
        return false;
    }
    
    if (default_page_size_ > max_page_size_) {
        spdlog::error("Default page size cannot be larger than max page size");
        return false;
    }
    
    return true;
}

nlohmann::json Config::to_json() const {
    nlohmann::json config;
    
    config["host"] = host_;
    config["port"] = port_;
    config["database_path"] = database_path_;
    config["log_level"] = log_level_;
    config["log_file"] = log_file_;
    config["max_content_size"] = max_content_size_;
    config["default_page_size"] = default_page_size_;
    config["max_page_size"] = max_page_size_;
    config["enable_cors"] = enable_cors_;
    config["cors_origin"] = cors_origin_;
    config["static_files_path"] = static_files_path_;
    config["enable_static_files"] = enable_static_files_;
    
    // 文件上传配置
    config["upload_path"] = upload_path_;
    config["max_file_size"] = max_file_size_;
    config["allowed_file_types"] = allowed_file_types_;
    config["enable_file_upload"] = enable_file_upload_;
    
    // LLaMA.cpp配置
    config["llama_model_path"] = llama_model_path_;
    config["llama_executable_path"] = llama_executable_path_;
    config["llama_context_size"] = llama_context_size_;
    config["llama_threads"] = llama_threads_;
    config["llama_temperature"] = llama_temperature_;
    config["llama_max_tokens"] = llama_max_tokens_;
    config["enable_llama"] = enable_llama_;
    
    return config;
}

void Config::load_defaults() {
    host_ = "127.0.0.1";
    port_ = 8080;
    database_path_ = "./data/content.db";
    log_level_ = "info";
    log_file_ = "";
    max_content_size_ = 1024 * 1024; // 1MB
    default_page_size_ = 20;
    max_page_size_ = 100;
    enable_cors_ = true;
    cors_origin_ = "*";
    static_files_path_ = "./web";
    enable_static_files_ = true;
    
    // 文件上传默认配置
    upload_path_ = "./uploads";
    max_file_size_ = 10 * 1024 * 1024; // 10MB
    allowed_file_types_ = {".txt", ".md", ".pdf", ".doc", ".docx", ".jpg", ".png", ".gif"};
    enable_file_upload_ = true;
    
    // LLaMA.cpp默认配置
    llama_model_path_ = "";
    llama_executable_path_ = "./llama.cpp/main";
    llama_context_size_ = 2048;
    llama_threads_ = 4;
    llama_temperature_ = 0.7f;
    llama_max_tokens_ = 512;
    enable_llama_ = false;
}

void Config::apply_config(const nlohmann::json& config) {
    if (config.contains("host")) {
        host_ = config["host"].get<std::string>();
    }
    if (config.contains("port")) {
        port_ = config["port"].get<int>();
    }
    if (config.contains("database_path")) {
        database_path_ = config["database_path"].get<std::string>();
    }
    if (config.contains("log_level")) {
        log_level_ = config["log_level"].get<std::string>();
    }
    if (config.contains("log_file")) {
        log_file_ = config["log_file"].get<std::string>();
    }
    if (config.contains("max_content_size")) {
        max_content_size_ = config["max_content_size"].get<int>();
    }
    if (config.contains("default_page_size")) {
        default_page_size_ = config["default_page_size"].get<int>();
    }
    if (config.contains("max_page_size")) {
        max_page_size_ = config["max_page_size"].get<int>();
    }
    if (config.contains("enable_cors")) {
        enable_cors_ = config["enable_cors"].get<bool>();
    }
    if (config.contains("cors_origin")) {
        cors_origin_ = config["cors_origin"].get<std::string>();
    }
    if (config.contains("static_files_path")) {
        static_files_path_ = config["static_files_path"].get<std::string>();
    }
    if (config.contains("enable_static_files")) {
        enable_static_files_ = config["enable_static_files"].get<bool>();
    }
    
    // 文件上传配置
    if (config.contains("upload_path")) {
        upload_path_ = config["upload_path"].get<std::string>();
    }
    if (config.contains("max_file_size")) {
        max_file_size_ = config["max_file_size"].get<int>();
    }
    if (config.contains("allowed_file_types")) {
        allowed_file_types_ = config["allowed_file_types"].get<std::vector<std::string>>();
    }
    if (config.contains("enable_file_upload")) {
        enable_file_upload_ = config["enable_file_upload"].get<bool>();
    }
    
    // LLaMA.cpp配置
    if (config.contains("llama_model_path")) {
        llama_model_path_ = config["llama_model_path"].get<std::string>();
    }
    if (config.contains("llama_executable_path")) {
        llama_executable_path_ = config["llama_executable_path"].get<std::string>();
    }
    if (config.contains("llama_context_size")) {
        llama_context_size_ = config["llama_context_size"].get<int>();
    }
    if (config.contains("llama_threads")) {
        llama_threads_ = config["llama_threads"].get<int>();
    }
    if (config.contains("llama_temperature")) {
        llama_temperature_ = config["llama_temperature"].get<float>();
    }
    if (config.contains("llama_max_tokens")) {
        llama_max_tokens_ = config["llama_max_tokens"].get<int>();
    }
    if (config.contains("enable_llama")) {
        enable_llama_ = config["enable_llama"].get<bool>();
    }
}

bool Config::update_config(const nlohmann::json& new_config) {
    try {
        // 应用新配置
        apply_config(new_config);
        
        // 验证配置
        if (!validate()) {
            spdlog::error("Invalid configuration after update");
            return false;
        }
        
        // 如果有配置文件路径，保存到文件
        if (!current_config_path_.empty()) {
            return save_config_to_file(current_config_path_);
        }
        
        return true;
    } catch (const std::exception& e) {
        spdlog::error("Failed to update config: {}", e.what());
        return false;
    }
}

bool Config::save_config_to_file(const std::string& config_path) const {
    try {
        std::ofstream file(config_path);
        if (!file.is_open()) {
            spdlog::error("Failed to open config file for writing: {}", config_path);
            return false;
        }
        
        nlohmann::json config = to_json();
        file << config.dump(4); // 格式化输出
        
        spdlog::info("Configuration saved to: {}", config_path);
        return true;
    } catch (const std::exception& e) {
        spdlog::error("Failed to save config to file {}: {}", config_path, e.what());
        return false;
    }
}

} // namespace mcp