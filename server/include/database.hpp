#pragma once

#include <string>
#include <vector>
#include <memory>
#include <optional>
#include <sqlite3.h>
#include <nlohmann/json.hpp>

namespace mcp {

// 内容项结构
struct ContentItem {
    int64_t id;
    std::string title;
    std::string content;
    std::string content_type; // text, markdown, code, etc.
    std::string tags;
    std::string metadata; // JSON格式的元数据
    int64_t created_at;
    int64_t updated_at;
    
    nlohmann::json to_json() const;
    static ContentItem from_json(const nlohmann::json& j);
};

// 数据库管理类
class Database {
public:
    explicit Database(const std::string& db_path);
    ~Database();
    
    // 禁用拷贝
    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;
    
    // 初始化数据库
    bool initialize();
    
    // 内容管理
    std::optional<int64_t> create_content(const ContentItem& item);
    std::optional<ContentItem> get_content(int64_t id);
    bool update_content(const ContentItem& item);
    bool delete_content(int64_t id);
    
    // 查询功能
    std::vector<ContentItem> search_content(const std::string& query, int limit = 50);
    std::vector<ContentItem> get_content_by_tag(const std::string& tag, int limit = 50);
    std::vector<ContentItem> get_recent_content(int limit = 20);
    std::vector<ContentItem> list_all_content(int offset = 0, int limit = 50);
    
    // 统计信息
    int64_t get_content_count();
    std::vector<std::string> get_all_tags();
    
private:
    sqlite3* db_;
    std::string db_path_;
    
    bool execute_sql(const std::string& sql);
    bool create_tables();
    ContentItem row_to_content_item(sqlite3_stmt* stmt);
};

} // namespace mcp