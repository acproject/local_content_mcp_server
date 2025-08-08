#include "file_upload.hpp"
#include "config.hpp"
#include <fstream>
#include <filesystem>
#include <random>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <string>
#include <spdlog/spdlog.h>

namespace mcp {

// FileInfo 实现
nlohmann::json FileInfo::to_json() const {
    nlohmann::json j;
    j["id"] = id;
    j["filename"] = filename;
    j["original_name"] = original_name;
    j["file_path"] = file_path;
    j["mime_type"] = mime_type;
    j["file_size"] = file_size;
    j["upload_time"] = upload_time;
    j["description"] = description;
    j["tags"] = tags;
    return j;
}

void FileInfo::from_json(const nlohmann::json& j) {
    id = j.value("id", "");
    filename = j.value("filename", "");
    original_name = j.value("original_name", "");
    file_path = j.value("file_path", "");
    mime_type = j.value("mime_type", "");
    file_size = j.value("file_size", 0);
    upload_time = j.value("upload_time", "");
    description = j.value("description", "");
    tags = j.value("tags", std::vector<std::string>{});
}

// UploadResult 实现
nlohmann::json UploadResult::to_json() const {
    nlohmann::json j;
    j["success"] = success;
    j["message"] = message;
    if (success) {
        j["file_info"] = file_info.to_json();
    }
    return j;
}

// FileUploadManager::Impl 实现
class FileUploadManager::Impl {
public:
    std::string upload_path_;
    std::string metadata_file_;
    std::vector<FileInfo> files_;
    std::random_device rd_;
    std::mt19937 gen_;
    
    Impl() : gen_(rd_()) {}
    
    bool load_metadata() {
        try {
            if (!std::filesystem::exists(metadata_file_)) {
                return true; // 文件不存在是正常的
            }
            
            std::ifstream file(metadata_file_);
            if (!file.is_open()) {
                spdlog::error("Failed to open metadata file: {}", metadata_file_);
                return false;
            }
            
            nlohmann::json j;
            file >> j;
            
            files_.clear();
            for (const auto& item : j["files"]) {
                FileInfo info;
                info.from_json(item);
                files_.push_back(info);
            }
            
            return true;
        } catch (const std::exception& e) {
            spdlog::error("Failed to load metadata: {}", e.what());
            return false;
        }
    }
    
    bool save_metadata() {
        try {
            nlohmann::json j;
            j["files"] = nlohmann::json::array();
            
            for (const auto& file : files_) {
                j["files"].push_back(file.to_json());
            }
            
            std::ofstream file(metadata_file_);
            if (!file.is_open()) {
                spdlog::error("Failed to open metadata file for writing: {}", metadata_file_);
                return false;
            }
            
            file << j.dump(4);
            return true;
        } catch (const std::exception& e) {
            spdlog::error("Failed to save metadata: {}", e.what());
            return false;
        }
    }
};

// FileUploadManager 实现
FileUploadManager::FileUploadManager() : pimpl_(std::make_unique<Impl>()) {}

FileUploadManager::~FileUploadManager() = default;

bool FileUploadManager::initialize(const std::string& upload_path) {
    pimpl_->upload_path_ = upload_path;
    pimpl_->metadata_file_ = upload_path + "/metadata.json";
    
    // 创建上传目录
    if (!create_upload_directory(upload_path)) {
        return false;
    }
    
    // 加载元数据
    return pimpl_->load_metadata();
}

UploadResult FileUploadManager::handle_upload(const httplib::Request& req, const std::string& field_name) {
    UploadResult result;
    result.success = false;
    
    try {
        auto file_item = req.get_file_value(field_name);
        if (!file_item.filename.empty()) {
            // 验证文件类型
            if (!is_allowed_file_type(file_item.filename)) {
                result.message = "File type not allowed";
                return result;
            }
            
            // 验证文件大小
            if (!is_valid_file_size(file_item.content.size())) {
                result.message = "File size exceeds limit";
                return result;
            }
            
            // 生成文件信息
            FileInfo file_info;
            file_info.id = generate_file_id();
            file_info.original_name = file_item.filename;
            file_info.filename = sanitize_filename(file_item.filename);
            file_info.mime_type = get_mime_type(file_item.filename);
            file_info.file_size = file_item.content.size();
            
            // 生成时间戳
            auto now = std::chrono::system_clock::now();
            auto time_t = std::chrono::system_clock::to_time_t(now);
            std::stringstream ss;
            ss << std::put_time(std::gmtime(&time_t), "%Y-%m-%dT%H:%M:%SZ");
            file_info.upload_time = ss.str();
            
            // 构建文件路径
            std::string file_extension = get_file_extension(file_item.filename);
            file_info.file_path = pimpl_->upload_path_ + "/" + file_info.id + file_extension;
            
            // 保存文件
            std::ofstream outfile(file_info.file_path, std::ios::binary);
            if (!outfile.is_open()) {
                result.message = "Failed to save file";
                return result;
            }
            outfile.write(file_item.content.data(), file_item.content.size());
            outfile.close();
            
            // 添加到文件列表
            pimpl_->files_.push_back(file_info);
            
            // 保存元数据
            if (!pimpl_->save_metadata()) {
                result.message = "Failed to save metadata";
                return result;
            }
            
            result.success = true;
            result.message = "File uploaded successfully";
            result.file_info = file_info;
        } else {
            result.message = "No file provided";
        }
    } catch (const std::exception& e) {
        result.message = "Upload failed: " + std::string(e.what());
        spdlog::error("Upload error: {}", e.what());
    }
    
    return result;
}

std::vector<FileInfo> FileUploadManager::list_files(int page, int page_size) {
    std::vector<FileInfo> result;
    
    int start = (page - 1) * page_size;
    int end = std::min(start + page_size, static_cast<int>(pimpl_->files_.size()));
    
    for (int i = start; i < end; ++i) {
        result.push_back(pimpl_->files_[i]);
    }
    
    return result;
}

FileInfo FileUploadManager::get_file_info(const std::string& file_id) {
    auto it = std::find_if(pimpl_->files_.begin(), pimpl_->files_.end(),
                          [&file_id](const FileInfo& info) {
                              return info.id == file_id;
                          });
    
    if (it != pimpl_->files_.end()) {
        return *it;
    }
    
    return FileInfo{}; // 返回空的FileInfo
}

bool FileUploadManager::delete_file(const std::string& file_id) {
    auto it = std::find_if(pimpl_->files_.begin(), pimpl_->files_.end(),
                          [&file_id](const FileInfo& info) {
                              return info.id == file_id;
                          });
    
    if (it != pimpl_->files_.end()) {
        // 删除物理文件
        try {
            std::filesystem::remove(it->file_path);
        } catch (const std::exception& e) {
            spdlog::warn("Failed to delete physical file: {}", e.what());
        }
        
        // 从列表中移除
        pimpl_->files_.erase(it);
        
        // 保存元数据
        return pimpl_->save_metadata();
    }
    
    return false;
}

bool FileUploadManager::update_file_info(const std::string& file_id, const nlohmann::json& update_data) {
    auto it = std::find_if(pimpl_->files_.begin(), pimpl_->files_.end(),
                          [&file_id](const FileInfo& info) {
                              return info.id == file_id;
                          });
    
    if (it != pimpl_->files_.end()) {
        if (update_data.contains("description")) {
            it->description = update_data["description"].get<std::string>();
        }
        if (update_data.contains("tags")) {
            it->tags = update_data["tags"].get<std::vector<std::string>>();
        }
        
        return pimpl_->save_metadata();
    }
    
    return false;
}

bool FileUploadManager::is_allowed_file_type(const std::string& filename) const {
    auto& config = Config::instance();
    auto allowed_types = config.get_allowed_file_types();
    
    std::string extension = get_file_extension(filename);
    std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
    
    return std::find(allowed_types.begin(), allowed_types.end(), extension) != allowed_types.end();
}

bool FileUploadManager::is_valid_file_size(size_t file_size) const {
    auto& config = Config::instance();
    return file_size <= static_cast<size_t>(config.get_max_file_size());
}

std::string FileUploadManager::get_file_content(const std::string& file_id) {
    FileInfo info = get_file_info(file_id);
    if (info.id.empty()) {
        return "";
    }
    
    std::ifstream file(info.file_path, std::ios::binary);
    if (!file.is_open()) {
        return "";
    }
    
    std::string content((std::istreambuf_iterator<char>(file)),
                       std::istreambuf_iterator<char>());
    return content;
}

bool FileUploadManager::serve_file(const std::string& file_id, httplib::Response& res) {
    FileInfo info = get_file_info(file_id);
    if (info.id.empty()) {
        return false;
    }
    
    std::ifstream file(info.file_path, std::ios::binary);
    if (!file.is_open()) {
        return false;
    }
    
    std::string content((std::istreambuf_iterator<char>(file)),
                       std::istreambuf_iterator<char>());
    
    res.set_content(content, info.mime_type);
    res.set_header("Content-Disposition", "attachment; filename=\"" + info.original_name + "\"");
    
    return true;
}

std::vector<FileInfo> FileUploadManager::search_files(const std::string& query, const std::vector<std::string>& tags) {
    std::vector<FileInfo> result;
    
    for (const auto& file : pimpl_->files_) {
        bool matches = true;
        
        // 检查查询字符串
        if (!query.empty()) {
            std::string lower_query = query;
            std::transform(lower_query.begin(), lower_query.end(), lower_query.begin(), ::tolower);
            
            std::string lower_filename = file.filename;
            std::transform(lower_filename.begin(), lower_filename.end(), lower_filename.begin(), ::tolower);
            
            std::string lower_description = file.description;
            std::transform(lower_description.begin(), lower_description.end(), lower_description.begin(), ::tolower);
            
            if (lower_filename.find(lower_query) == std::string::npos &&
                lower_description.find(lower_query) == std::string::npos) {
                matches = false;
            }
        }
        
        // 检查标签
        if (matches && !tags.empty()) {
            for (const auto& tag : tags) {
                if (std::find(file.tags.begin(), file.tags.end(), tag) == file.tags.end()) {
                    matches = false;
                    break;
                }
            }
        }
        
        if (matches) {
            result.push_back(file);
        }
    }
    
    return result;
}

nlohmann::json FileUploadManager::get_upload_statistics() {
    nlohmann::json stats;
    
    stats["total_files"] = pimpl_->files_.size();
    
    size_t total_size = 0;
    std::map<std::string, int> type_counts;
    
    for (const auto& file : pimpl_->files_) {
        total_size += file.file_size;
        
        std::string extension = get_file_extension(file.filename);
        type_counts[extension]++;
    }
    
    stats["total_size"] = total_size;
    stats["file_types"] = type_counts;
    
    return stats;
}

std::string FileUploadManager::generate_file_id() {
    std::uniform_int_distribution<> dis(0, 15);
    std::string id;
    
    for (int i = 0; i < 32; ++i) {
        int val = dis(pimpl_->gen_);
        if (val < 10) {
            id += char('0' + val);
        } else {
            id += char('a' + val - 10);
        }
    }
    
    return id;
}

std::string FileUploadManager::get_mime_type(const std::string& filename) {
    std::string extension = get_file_extension(filename);
    std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
    
    static std::map<std::string, std::string> mime_types = {
        {".txt", "text/plain"},
        {".md", "text/markdown"},
        {".pdf", "application/pdf"},
        {".doc", "application/msword"},
        {".docx", "application/vnd.openxmlformats-officedocument.wordprocessingml.document"},
        {".jpg", "image/jpeg"},
        {".jpeg", "image/jpeg"},
        {".png", "image/png"},
        {".gif", "image/gif"},
        {".html", "text/html"},
        {".css", "text/css"},
        {".js", "application/javascript"},
        {".json", "application/json"}
    };
    
    auto it = mime_types.find(extension);
    return (it != mime_types.end()) ? it->second : "application/octet-stream";
}

std::string FileUploadManager::get_file_extension(const std::string& filename) const {
    size_t pos = filename.find_last_of('.');
    if (pos != std::string::npos) {
        return filename.substr(pos);
    }
    return "";
}

bool FileUploadManager::create_upload_directory(const std::string& path) {
    try {
        std::filesystem::create_directories(path);
        return true;
    } catch (const std::exception& e) {
        spdlog::error("Failed to create upload directory {}: {}", path, e.what());
        return false;
    }
}

std::string FileUploadManager::sanitize_filename(const std::string& filename) {
    std::string sanitized = filename;
    
    // 替换不安全的字符
    std::replace(sanitized.begin(), sanitized.end(), '/', '_');
    std::replace(sanitized.begin(), sanitized.end(), '\\', '_');
    std::replace(sanitized.begin(), sanitized.end(), ':', '_');
    std::replace(sanitized.begin(), sanitized.end(), '*', '_');
    std::replace(sanitized.begin(), sanitized.end(), '?', '_');
    std::replace(sanitized.begin(), sanitized.end(), '"', '_');
    std::replace(sanitized.begin(), sanitized.end(), '<', '_');
    std::replace(sanitized.begin(), sanitized.end(), '>', '_');
    std::replace(sanitized.begin(), sanitized.end(), '|', '_');
    
    return sanitized;
}

} // namespace mcp