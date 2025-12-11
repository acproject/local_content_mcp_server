#include "database.hpp"
#include <spdlog/spdlog.h>
#include <filesystem>
#include <ctime>
#include <sstream>

namespace mcp {

// ContentItem JSON转换
nlohmann::json ContentItem::to_json() const {
    nlohmann::json j;
    j["id"] = id;
    j["title"] = title;
    j["content"] = content;
    j["content_type"] = content_type;
    j["tags"] = tags;
    j["created_at"] = created_at;
    j["updated_at"] = updated_at;
    
    // 解析metadata JSON字符串
    if (!metadata.empty()) {
        try {
            j["metadata"] = nlohmann::json::parse(metadata);
        } catch (const std::exception&) {
            j["metadata"] = nlohmann::json::object();
        }
    } else {
        j["metadata"] = nlohmann::json::object();
    }
    
    return j;
}

ContentItem ContentItem::from_json(const nlohmann::json& j) {
    ContentItem item;
    item.id = j.value("id", 0);
    item.title = j.value("title", "");
    item.content = j.value("content", "");
    item.content_type = j.value("content_type", "text");
    if (item.content_type == "document") {
        item.content_type = "text";
    }
    item.tags = j.value("tags", "");
    
    // 处理metadata
    if (j.contains("metadata") && j["metadata"].is_object()) {
        item.metadata = j["metadata"].dump();
    } else {
        item.metadata = "{}";
    }
    
    auto now = std::time(nullptr);
    item.created_at = j.value("created_at", now);
    item.updated_at = j.value("updated_at", now);
    
    return item;
}

// Database实现
Database::Database(const std::string& db_path) : db_(nullptr), db_path_(db_path) {
}

Database::~Database() {
    if (db_) {
        sqlite3_close(db_);
    }
}

bool Database::initialize() {
    // 确保数据库目录存在
    std::filesystem::path db_file(db_path_);
    std::filesystem::create_directories(db_file.parent_path());
    
    // 打开数据库
    int rc = sqlite3_open(db_path_.c_str(), &db_);
    if (rc != SQLITE_OK) {
        spdlog::error("Cannot open database: {}", sqlite3_errmsg(db_));
        return false;
    }
    
    // 启用外键约束
    execute_sql("PRAGMA foreign_keys = ON;");
    
    // 创建表
    return create_tables();
}

bool Database::create_tables() {
    const std::string create_content_table = R"(
        CREATE TABLE IF NOT EXISTS content (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            title TEXT NOT NULL,
            content TEXT NOT NULL,
            content_type TEXT DEFAULT 'text',
            tags TEXT DEFAULT '',
            metadata TEXT DEFAULT '{}',
            created_at INTEGER NOT NULL,
            updated_at INTEGER NOT NULL
        );
    )";
    
    const std::string create_indexes = R"(
        CREATE INDEX IF NOT EXISTS idx_content_title ON content(title);
        CREATE INDEX IF NOT EXISTS idx_content_tags ON content(tags);
        CREATE INDEX IF NOT EXISTS idx_content_type ON content(content_type);
        CREATE INDEX IF NOT EXISTS idx_content_created_at ON content(created_at);
        CREATE INDEX IF NOT EXISTS idx_content_updated_at ON content(updated_at);
        CREATE VIRTUAL TABLE IF NOT EXISTS content_fts USING fts5(
            title, content, tags, content=content, content_rowid=id
        );
    )";
    
    return execute_sql(create_content_table) && execute_sql(create_indexes);
}

bool Database::execute_sql(const std::string& sql) {
    char* err_msg = nullptr;
    int rc = sqlite3_exec(db_, sql.c_str(), nullptr, nullptr, &err_msg);
    
    if (rc != SQLITE_OK) {
        spdlog::error("SQL error: {}", err_msg ? err_msg : "Unknown error");
        if (err_msg) {
            sqlite3_free(err_msg);
        }
        return false;
    }
    
    return true;
}

std::optional<int64_t> Database::create_content(const ContentItem& item) {
    const std::string sql = R"(
        INSERT INTO content (title, content, content_type, tags, metadata, created_at, updated_at)
        VALUES (?, ?, ?, ?, ?, ?, ?);
    )";
    
    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr);
    
    if (rc != SQLITE_OK) {
        spdlog::error("Failed to prepare statement: {}", sqlite3_errmsg(db_));
        return std::nullopt;
    }
    
    auto now = std::time(nullptr);
    sqlite3_bind_text(stmt, 1, item.title.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, item.content.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, item.content_type.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 4, item.tags.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 5, item.metadata.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int64(stmt, 6, now);
    sqlite3_bind_int64(stmt, 7, now);
    
    rc = sqlite3_step(stmt);
    int64_t id = 0;
    
    if (rc == SQLITE_DONE) {
        id = sqlite3_last_insert_rowid(db_);
    } else {
        spdlog::error("Failed to insert content: {}", sqlite3_errmsg(db_));
    }
    
    sqlite3_finalize(stmt);
    
    if (id > 0) {
        // 更新FTS索引
        const std::string fts_sql = "INSERT INTO content_fts(rowid, title, content, tags) VALUES (?, ?, ?, ?)";
        sqlite3_prepare_v2(db_, fts_sql.c_str(), -1, &stmt, nullptr);
        sqlite3_bind_int64(stmt, 1, id);
        sqlite3_bind_text(stmt, 2, item.title.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 3, item.content.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 4, item.tags.c_str(), -1, SQLITE_STATIC);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
        
        return id;
    }
    
    return std::nullopt;
}

std::optional<ContentItem> Database::get_content(int64_t id) {
    const std::string sql = "SELECT * FROM content WHERE id = ?";
    
    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr);
    
    if (rc != SQLITE_OK) {
        spdlog::error("Failed to prepare statement: {}", sqlite3_errmsg(db_));
        return std::nullopt;
    }
    
    sqlite3_bind_int64(stmt, 1, id);
    
    rc = sqlite3_step(stmt);
    std::optional<ContentItem> result;
    
    if (rc == SQLITE_ROW) {
        result = row_to_content_item(stmt);
    }
    
    sqlite3_finalize(stmt);
    return result;
}

bool Database::update_content(const ContentItem& item) {
    const std::string sql = R"(
        UPDATE content 
        SET title = ?, content = ?, content_type = ?, tags = ?, metadata = ?, updated_at = ?
        WHERE id = ?;
    )";
    
    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr);
    
    if (rc != SQLITE_OK) {
        spdlog::error("Failed to prepare statement: {}", sqlite3_errmsg(db_));
        return false;
    }
    
    auto now = std::time(nullptr);
    sqlite3_bind_text(stmt, 1, item.title.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, item.content.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, item.content_type.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 4, item.tags.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 5, item.metadata.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int64(stmt, 6, now);
    sqlite3_bind_int64(stmt, 7, item.id);
    
    rc = sqlite3_step(stmt);
    bool success = (rc == SQLITE_DONE);
    
    if (!success) {
        spdlog::error("Failed to update content: {}", sqlite3_errmsg(db_));
    }
    
    sqlite3_finalize(stmt);
    
    if (success) {
        // 更新FTS索引
        const std::string fts_sql = "UPDATE content_fts SET title = ?, content = ?, tags = ? WHERE rowid = ?";
        sqlite3_prepare_v2(db_, fts_sql.c_str(), -1, &stmt, nullptr);
        sqlite3_bind_text(stmt, 1, item.title.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 2, item.content.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 3, item.tags.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_int64(stmt, 4, item.id);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
    }
    
    return success;
}

bool Database::delete_content(int64_t id) {
    // 先删除FTS索引
    const std::string fts_sql = "DELETE FROM content_fts WHERE rowid = ?";
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db_, fts_sql.c_str(), -1, &stmt, nullptr);
    sqlite3_bind_int64(stmt, 1, id);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    // 删除主记录
    const std::string sql = "DELETE FROM content WHERE id = ?";
    
    int rc = sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr);
    
    if (rc != SQLITE_OK) {
        spdlog::error("Failed to prepare statement: {}", sqlite3_errmsg(db_));
        return false;
    }
    
    sqlite3_bind_int64(stmt, 1, id);
    
    rc = sqlite3_step(stmt);
    bool success = (rc == SQLITE_DONE);
    
    if (!success) {
        spdlog::error("Failed to delete content: {}", sqlite3_errmsg(db_));
    }
    
    sqlite3_finalize(stmt);
    return success;
}

std::vector<ContentItem> Database::search_content(const std::string& query, int limit) {
    std::vector<ContentItem> results;
    
    const std::string sql = R"(
        SELECT c.* FROM content c
        JOIN content_fts fts ON c.id = fts.rowid
        WHERE content_fts MATCH ?
        ORDER BY rank
        LIMIT ?;
    )";
    
    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr);
    
    if (rc != SQLITE_OK) {
        spdlog::error("Failed to prepare statement: {}", sqlite3_errmsg(db_));
        return results;
    }
    
    sqlite3_bind_text(stmt, 1, query.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 2, limit);
    
    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        results.push_back(row_to_content_item(stmt));
    }
    
    sqlite3_finalize(stmt);
    return results;
}

std::vector<ContentItem> Database::get_content_by_tag(const std::string& tag, int limit) {
    std::vector<ContentItem> results;
    
    const std::string sql = R"(
        SELECT * FROM content 
        WHERE tags LIKE ?
        ORDER BY updated_at DESC
        LIMIT ?;
    )";
    
    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr);
    
    if (rc != SQLITE_OK) {
        spdlog::error("Failed to prepare statement: {}", sqlite3_errmsg(db_));
        return results;
    }
    
    std::string tag_pattern = "%" + tag + "%";
    sqlite3_bind_text(stmt, 1, tag_pattern.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 2, limit);
    
    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        results.push_back(row_to_content_item(stmt));
    }
    
    sqlite3_finalize(stmt);
    return results;
}

std::vector<ContentItem> Database::get_recent_content(int limit) {
    std::vector<ContentItem> results;
    
    const std::string sql = R"(
        SELECT * FROM content 
        ORDER BY updated_at DESC
        LIMIT ?;
    )";
    
    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr);
    
    if (rc != SQLITE_OK) {
        spdlog::error("Failed to prepare statement: {}", sqlite3_errmsg(db_));
        return results;
    }
    
    sqlite3_bind_int(stmt, 1, limit);
    
    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        results.push_back(row_to_content_item(stmt));
    }
    
    sqlite3_finalize(stmt);
    return results;
}

std::vector<ContentItem> Database::list_all_content(int offset, int limit) {
    std::vector<ContentItem> results;
    
    const std::string sql = R"(
        SELECT * FROM content 
        ORDER BY updated_at DESC
        LIMIT ? OFFSET ?;
    )";
    
    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr);
    
    if (rc != SQLITE_OK) {
        spdlog::error("Failed to prepare statement: {}", sqlite3_errmsg(db_));
        return results;
    }
    
    sqlite3_bind_int(stmt, 1, limit);
    sqlite3_bind_int(stmt, 2, offset);
    
    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        results.push_back(row_to_content_item(stmt));
    }
    
    sqlite3_finalize(stmt);
    return results;
}

int64_t Database::get_content_count() {
    const std::string sql = "SELECT COUNT(*) FROM content";
    
    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr);
    
    if (rc != SQLITE_OK) {
        spdlog::error("Failed to prepare statement: {}", sqlite3_errmsg(db_));
        return 0;
    }
    
    int64_t count = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        count = sqlite3_column_int64(stmt, 0);
    }
    
    sqlite3_finalize(stmt);
    return count;
}

std::vector<std::string> Database::get_all_tags() {
    std::vector<std::string> tags;
    
    const std::string sql = "SELECT DISTINCT tags FROM content WHERE tags != ''";
    
    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr);
    
    if (rc != SQLITE_OK) {
        spdlog::error("Failed to prepare statement: {}", sqlite3_errmsg(db_));
        return tags;
    }
    
    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        const char* tags_str = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        if (tags_str) {
            // 简单的标签分割（假设用逗号分隔）
            std::string tag_line(tags_str);
            std::stringstream ss(tag_line);
            std::string tag;
            while (std::getline(ss, tag, ',')) {
                // 去除前后空格
                tag.erase(0, tag.find_first_not_of(" \t"));
                tag.erase(tag.find_last_not_of(" \t") + 1);
                if (!tag.empty()) {
                    tags.push_back(tag);
                }
            }
        }
    }
    
    sqlite3_finalize(stmt);
    
    // 去重
    std::sort(tags.begin(), tags.end());
    tags.erase(std::unique(tags.begin(), tags.end()), tags.end());
    
    return tags;
}

ContentItem Database::row_to_content_item(sqlite3_stmt* stmt) {
    ContentItem item;
    
    item.id = sqlite3_column_int64(stmt, 0);
    
    const char* title = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
    item.title = title ? title : "";
    
    const char* content = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
    item.content = content ? content : "";
    
    const char* content_type = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
    item.content_type = content_type ? content_type : "text";
    
    const char* tags = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
    item.tags = tags ? tags : "";
    
    const char* metadata = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
    item.metadata = metadata ? metadata : "{}";
    
    item.created_at = sqlite3_column_int64(stmt, 6);
    item.updated_at = sqlite3_column_int64(stmt, 7);
    
    return item;
}

} // namespace mcp