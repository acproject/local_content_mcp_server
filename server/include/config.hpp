#pragma once

#include <string>
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
    int port_ = 8080;
    
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
    
    // 辅助方法
    void load_defaults();
    void apply_config(const nlohmann::json& config);
};

} // namespace mcp