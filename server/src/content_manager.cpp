#include "content_manager.hpp"
#include <spdlog/spdlog.h>
#include <algorithm>
#include <sstream>

namespace mcp {

// SearchResult JSON转换
nlohmann::json SearchResult::to_json() const {
    nlohmann::json j;
    j["items"] = nlohmann::json::array();
    for (const auto& item : items) {
        j["items"].push_back(item.to_json());
    }
    j["total_count"] = total_count;
    j["page"] = page;
    j["page_size"] = page_size;
    j["total_pages"] = (total_count + page_size - 1) / page_size;
    return j;
}

// ContentManager实现
ContentManager::ContentManager(std::shared_ptr<Database> db) : db_(std::move(db)) {
}

nlohmann::json ContentManager::create_content(const nlohmann::json& request) {
    try {
        std::string error_msg;
        if (!validate_content_item(request, error_msg)) {
            return create_error_response(error_msg, 400);
        }
        
        ContentItem item = ContentItem::from_json(request);
        auto id = db_->create_content(item);
        
        if (!id) {
            return create_error_response("Failed to create content", 500);
        }
        
        // 返回创建的内容
        auto created_item = db_->get_content(*id);
        if (created_item) {
            return create_success_response(created_item->to_json());
        } else {
            nlohmann::json result;
            result["id"] = *id;
            return create_success_response(result);
        }
        
    } catch (const std::exception& e) {
        spdlog::error("Error creating content: {}", e.what());
        return create_error_response("Internal server error", 500);
    }
}

nlohmann::json ContentManager::get_content(int64_t id) {
    try {
        auto item = db_->get_content(id);
        if (!item) {
            return create_error_response("Content not found", 404);
        }
        
        return create_success_response(item->to_json());
        
    } catch (const std::exception& e) {
        spdlog::error("Error getting content: {}", e.what());
        return create_error_response("Internal server error", 500);
    }
}

nlohmann::json ContentManager::update_content(int64_t id, const nlohmann::json& request) {
    try {
        // 检查内容是否存在
        auto existing = db_->get_content(id);
        if (!existing) {
            return create_error_response("Content not found", 404);
        }
        
        std::string error_msg;
        if (!validate_content_item(request, error_msg)) {
            return create_error_response(error_msg, 400);
        }
        
        ContentItem item = ContentItem::from_json(request);
        item.id = id;
        item.created_at = existing->created_at; // 保持原创建时间
        
        if (!db_->update_content(item)) {
            return create_error_response("Failed to update content", 500);
        }
        
        // 返回更新后的内容
        auto updated_item = db_->get_content(id);
        if (updated_item) {
            return create_success_response(updated_item->to_json());
        } else {
            return create_success_response();
        }
        
    } catch (const std::exception& e) {
        spdlog::error("Error updating content: {}", e.what());
        return create_error_response("Internal server error", 500);
    }
}

nlohmann::json ContentManager::delete_content(int64_t id) {
    try {
        // 检查内容是否存在
        auto existing = db_->get_content(id);
        if (!existing) {
            return create_error_response("Content not found", 404);
        }
        
        if (!db_->delete_content(id)) {
            return create_error_response("Failed to delete content", 500);
        }
        
        return create_success_response();
        
    } catch (const std::exception& e) {
        spdlog::error("Error deleting content: {}", e.what());
        return create_error_response("Internal server error", 500);
    }
}

nlohmann::json ContentManager::search_content(const std::string& query, int page, int page_size) {
    try {
        if (query.empty()) {
            return create_error_response("Search query cannot be empty", 400);
        }
        
        if (page < 1) page = 1;
        if (page_size < 1 || page_size > 100) page_size = 20;
        
        // 计算偏移量
        int limit = page_size;
        
        auto items = db_->search_content(query, limit);
        
        SearchResult result;
        result.items = items;
        result.total_count = static_cast<int>(items.size()); // 简化实现，实际应该查询总数
        result.page = page;
        result.page_size = page_size;
        
        return create_success_response(result.to_json());
        
    } catch (const std::exception& e) {
        spdlog::error("Error searching content: {}", e.what());
        return create_error_response("Internal server error", 500);
    }
}

nlohmann::json ContentManager::get_content_by_tag(const std::string& tag, int page, int page_size) {
    try {
        if (tag.empty()) {
            return create_error_response("Tag cannot be empty", 400);
        }
        
        if (page < 1) page = 1;
        if (page_size < 1 || page_size > 100) page_size = 20;
        
        auto items = db_->get_content_by_tag(tag, page_size);
        
        SearchResult result;
        result.items = items;
        result.total_count = static_cast<int>(items.size());
        result.page = page;
        result.page_size = page_size;
        
        return create_success_response(result.to_json());
        
    } catch (const std::exception& e) {
        spdlog::error("Error getting content by tag: {}", e.what());
        return create_error_response("Internal server error", 500);
    }
}

nlohmann::json ContentManager::get_recent_content(int limit) {
    try {
        if (limit < 1 || limit > 100) limit = 20;
        
        auto items = db_->get_recent_content(limit);
        
        nlohmann::json result = nlohmann::json::array();
        for (const auto& item : items) {
            result.push_back(item.to_json());
        }
        
        return create_success_response(result);
        
    } catch (const std::exception& e) {
        spdlog::error("Error getting recent content: {}", e.what());
        return create_error_response("Internal server error", 500);
    }
}

nlohmann::json ContentManager::list_content(int page, int page_size) {
    try {
        if (page < 1) page = 1;
        if (page_size < 1 || page_size > 100) page_size = 20;
        
        int offset = (page - 1) * page_size;
        auto items = db_->list_all_content(offset, page_size);
        auto total_count = db_->get_content_count();
        
        SearchResult result;
        result.items = items;
        result.total_count = static_cast<int>(total_count);
        result.page = page;
        result.page_size = page_size;
        
        return create_success_response(result.to_json());
        
    } catch (const std::exception& e) {
        spdlog::error("Error listing content: {}", e.what());
        return create_error_response("Internal server error", 500);
    }
}

nlohmann::json ContentManager::get_statistics() {
    try {
        auto total_count = db_->get_content_count();
        auto tags = db_->get_all_tags();
        
        nlohmann::json stats;
        stats["total_content"] = total_count;
        stats["total_tags"] = tags.size();
        stats["tags"] = tags;
        
        return create_success_response(stats);
        
    } catch (const std::exception& e) {
        spdlog::error("Error getting statistics: {}", e.what());
        return create_error_response("Internal server error", 500);
    }
}

nlohmann::json ContentManager::get_tags() {
    try {
        auto tags = db_->get_all_tags();
        return create_success_response(nlohmann::json(tags));
        
    } catch (const std::exception& e) {
        spdlog::error("Error getting tags: {}", e.what());
        return create_error_response("Internal server error", 500);
    }
}

nlohmann::json ContentManager::bulk_create(const nlohmann::json& items) {
    try {
        if (!items.is_array()) {
            return create_error_response("Items must be an array", 400);
        }
        
        std::vector<int64_t> created_ids;
        std::vector<std::string> errors;
        
        for (size_t i = 0; i < items.size(); ++i) {
            try {
                std::string error_msg;
                if (!validate_content_item(items[i], error_msg)) {
                    errors.push_back("Item " + std::to_string(i) + ": " + error_msg);
                    continue;
                }
                
                ContentItem item = ContentItem::from_json(items[i]);
                auto id = db_->create_content(item);
                
                if (id) {
                    created_ids.push_back(*id);
                } else {
                    errors.push_back("Item " + std::to_string(i) + ": Failed to create");
                }
                
            } catch (const std::exception& e) {
                errors.push_back("Item " + std::to_string(i) + ": " + e.what());
            }
        }
        
        nlohmann::json result;
        result["created_ids"] = created_ids;
        result["created_count"] = created_ids.size();
        result["total_count"] = items.size();
        
        if (!errors.empty()) {
            result["errors"] = errors;
        }
        
        return create_success_response(result);
        
    } catch (const std::exception& e) {
        spdlog::error("Error in bulk create: {}", e.what());
        return create_error_response("Internal server error", 500);
    }
}

nlohmann::json ContentManager::bulk_delete(const std::vector<int64_t>& ids) {
    try {
        if (ids.empty()) {
            return create_error_response("IDs list cannot be empty", 400);
        }
        
        int deleted_count = 0;
        std::vector<std::string> errors;
        
        for (int64_t id : ids) {
            try {
                if (db_->delete_content(id)) {
                    deleted_count++;
                } else {
                    errors.push_back("Failed to delete ID: " + std::to_string(id));
                }
            } catch (const std::exception& e) {
                errors.push_back("ID " + std::to_string(id) + ": " + e.what());
            }
        }
        
        nlohmann::json result;
        result["deleted_count"] = deleted_count;
        result["total_count"] = ids.size();
        
        if (!errors.empty()) {
            result["errors"] = errors;
        }
        
        return create_success_response(result);
        
    } catch (const std::exception& e) {
        spdlog::error("Error in bulk delete: {}", e.what());
        return create_error_response("Internal server error", 500);
    }
}

nlohmann::json ContentManager::export_content(const std::string& format) {
    try {
        if (format != "json") {
            return create_error_response("Only JSON format is supported", 400);
        }
        
        auto items = db_->list_all_content(0, 10000); // 导出所有内容
        
        nlohmann::json export_data;
        export_data["version"] = "1.0";
        export_data["exported_at"] = std::time(nullptr);
        export_data["content"] = nlohmann::json::array();
        
        for (const auto& item : items) {
            export_data["content"].push_back(item.to_json());
        }
        
        return create_success_response(export_data);
        
    } catch (const std::exception& e) {
        spdlog::error("Error exporting content: {}", e.what());
        return create_error_response("Internal server error", 500);
    }
}

nlohmann::json ContentManager::import_content(const nlohmann::json& data) {
    try {
        if (!data.contains("content") || !data["content"].is_array()) {
            return create_error_response("Invalid import data format", 400);
        }
        
        const auto& content_array = data["content"];
        return bulk_create(content_array);
        
    } catch (const std::exception& e) {
        spdlog::error("Error importing content: {}", e.what());
        return create_error_response("Internal server error", 500);
    }
}

// 辅助方法实现
nlohmann::json ContentManager::create_error_response(const std::string& message, int code) {
    nlohmann::json response;
    response["success"] = false;
    response["error"] = {
        {"code", code},
        {"message", message}
    };
    return response;
}

nlohmann::json ContentManager::create_success_response(const nlohmann::json& data) {
    nlohmann::json response;
    response["success"] = true;
    response["data"] = data;
    return response;
}

bool ContentManager::validate_content_item(const nlohmann::json& item, std::string& error_msg) {
    if (!item.is_object()) {
        error_msg = "Content item must be an object";
        return false;
    }
    
    if (!item.contains("title") || !item["title"].is_string()) {
        error_msg = "Title is required and must be a string";
        return false;
    }
    
    if (!item.contains("content") || !item["content"].is_string()) {
        error_msg = "Content is required and must be a string";
        return false;
    }
    
    const std::string title = item["title"];
    const std::string content = item["content"];
    
    if (title.empty()) {
        error_msg = "Title cannot be empty";
        return false;
    }
    
    if (content.empty()) {
        error_msg = "Content cannot be empty";
        return false;
    }
    
    if (title.length() > 500) {
        error_msg = "Title is too long (max 500 characters)";
        return false;
    }
    
    if (content.length() > 1024 * 1024) { // 1MB
        error_msg = "Content is too long (max 1MB)";
        return false;
    }
    
    // 验证content_type
    if (item.contains("content_type")) {
        if (!item["content_type"].is_string()) {
            error_msg = "Content type must be a string";
            return false;
        }
        
        const std::string content_type = item["content_type"];
        const std::vector<std::string> valid_types = {
            "text", "markdown", "html", "code", "json", "xml", "yaml"
        };
        
        if (std::find(valid_types.begin(), valid_types.end(), content_type) == valid_types.end()) {
            error_msg = "Invalid content type";
            return false;
        }
    }
    
    // 验证tags
    if (item.contains("tags") && !item["tags"].is_string()) {
        error_msg = "Tags must be a string";
        return false;
    }
    
    // 验证metadata
    if (item.contains("metadata") && !item["metadata"].is_object()) {
        error_msg = "Metadata must be an object";
        return false;
    }
    
    return true;
}

std::vector<std::string> ContentManager::parse_tags(const std::string& tags_str) {
    std::vector<std::string> tags;
    std::stringstream ss(tags_str);
    std::string tag;
    
    while (std::getline(ss, tag, ',')) {
        // 去除前后空格
        tag.erase(0, tag.find_first_not_of(" \t"));
        tag.erase(tag.find_last_not_of(" \t") + 1);
        if (!tag.empty()) {
            tags.push_back(tag);
        }
    }
    
    return tags;
}

std::string ContentManager::join_tags(const std::vector<std::string>& tags) {
    std::string result;
    for (size_t i = 0; i < tags.size(); ++i) {
        if (i > 0) result += ", ";
        result += tags[i];
    }
    return result;
}

} // namespace mcp