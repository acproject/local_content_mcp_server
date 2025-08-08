#pragma once

#include "database.hpp"
#include <memory>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

namespace mcp {

// 搜索结果
struct SearchResult {
    std::vector<ContentItem> items;
    int total_count;
    int page;
    int page_size;
    
    nlohmann::json to_json() const;
};

// 内容管理器
class ContentManager {
public:
    explicit ContentManager(std::shared_ptr<Database> db);
    
    // 内容操作
    nlohmann::json create_content(const nlohmann::json& request);
    nlohmann::json get_content(int64_t id);
    nlohmann::json update_content(int64_t id, const nlohmann::json& request);
    nlohmann::json delete_content(int64_t id);
    
    // 搜索和查询
    nlohmann::json search_content(const std::string& query, int page = 1, int page_size = 20);
    nlohmann::json get_content_by_tag(const std::string& tag, int page = 1, int page_size = 20);
    nlohmann::json get_recent_content(int limit = 20);
    nlohmann::json list_content(int page = 1, int page_size = 20);
    
    // 统计和元数据
    nlohmann::json get_statistics();
    nlohmann::json get_tags();
    
    // 批量操作
    nlohmann::json bulk_create(const nlohmann::json& items);
    nlohmann::json bulk_delete(const std::vector<int64_t>& ids);
    
    // 导入导出
    nlohmann::json export_content(const std::string& format = "json");
    nlohmann::json import_content(const nlohmann::json& data);
    
private:
    std::shared_ptr<Database> db_;
    
    // 辅助方法
    nlohmann::json create_error_response(const std::string& message, int code = 400);
    nlohmann::json create_success_response(const nlohmann::json& data = nlohmann::json::object());
    bool validate_content_item(const nlohmann::json& item, std::string& error_msg);
    std::vector<std::string> parse_tags(const std::string& tags_str);
    std::string join_tags(const std::vector<std::string>& tags);
};

} // namespace mcp