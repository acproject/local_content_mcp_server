#pragma once

#include <string>
#include <vector>
#include <memory>
#include <nlohmann/json.hpp>
#include <httplib.h>

namespace mcp {

// 文件信息结构
struct FileInfo {
    std::string id;
    std::string filename;
    std::string original_name;
    std::string file_path;
    std::string mime_type;
    size_t file_size;
    std::string upload_time;
    std::string description;
    std::vector<std::string> tags;
    
    nlohmann::json to_json() const;
    void from_json(const nlohmann::json& j);
};

// 文件上传结果
struct UploadResult {
    bool success;
    std::string message;
    FileInfo file_info;
    
    nlohmann::json to_json() const;
};

// 文件上传管理器
class FileUploadManager {
public:
    FileUploadManager();
    ~FileUploadManager();
    
    // 初始化上传目录
    bool initialize(const std::string& upload_path);
    
    // 处理文件上传
    UploadResult handle_upload(const httplib::Request& req, const std::string& field_name = "file");
    
    // 文件管理
    std::vector<FileInfo> list_files(int page = 1, int page_size = 20);
    FileInfo get_file_info(const std::string& file_id);
    bool delete_file(const std::string& file_id);
    bool update_file_info(const std::string& file_id, const nlohmann::json& update_data);
    
    // 文件验证
    bool is_allowed_file_type(const std::string& filename) const;
    bool is_valid_file_size(size_t file_size) const;
    
    // 获取文件内容
    std::string get_file_content(const std::string& file_id);
    bool serve_file(const std::string& file_id, httplib::Response& res);
    
    // 搜索文件
    std::vector<FileInfo> search_files(const std::string& query, const std::vector<std::string>& tags = {});
    
    // 统计信息
    nlohmann::json get_upload_statistics();
    
private:
    class Impl;
    std::unique_ptr<Impl> pimpl_;
    
    // 辅助方法
    std::string generate_file_id();
    std::string get_mime_type(const std::string& filename);
    std::string get_file_extension(const std::string& filename) const;
    bool create_upload_directory(const std::string& path);
    std::string sanitize_filename(const std::string& filename);
};

} // namespace mcp