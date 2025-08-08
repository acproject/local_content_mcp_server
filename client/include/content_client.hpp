#pragma once

#include "mcp_client.hpp"
#include "http_client.hpp"
#include "database.hpp" // 共享数据结构
#include <vector>
#include <optional>

namespace mcp {

// 内容客户端响应结构
template<typename T>
struct ContentResponse {
    bool success = false;
    T data;
    std::string error_message;
    int error_code = 0;
    
    // 便利方法
    bool is_success() const { return success; }
    const T& get_data() const { return data; }
    std::string get_error() const { return error_message; }
};

// 分页结果
template<typename T>
struct PagedResult {
    std::vector<T> items;
    int total_count = 0;
    int page = 1;
    int page_size = 20;
    int total_pages = 0;
    bool has_next = false;
    bool has_previous = false;
    
    // JSON转换
    nlohmann::json to_json() const;
    void from_json(const nlohmann::json& j);
};

// 搜索选项
struct SearchOptions {
    std::string query;
    std::vector<std::string> tags;
    int page = 1;
    int page_size = 20;
    std::string sort_by = "created_at"; // created_at, updated_at, title
    std::string sort_order = "desc"; // asc, desc
    
    nlohmann::json to_json() const;
    void from_json(const nlohmann::json& j);
};

// 内容创建请求
struct CreateContentRequest {
    std::string title;
    std::string content;
    std::vector<std::string> tags;
    std::string content_type = "text/plain";
    
    nlohmann::json to_json() const;
    void from_json(const nlohmann::json& j);
};

// 内容更新请求
struct UpdateContentRequest {
    std::optional<std::string> title;
    std::optional<std::string> content;
    std::optional<std::vector<std::string>> tags;
    std::optional<std::string> content_type;
    
    nlohmann::json to_json() const;
    void from_json(const nlohmann::json& j);
};

// 统计信息
struct ContentStatistics {
    int total_items = 0;
    int total_tags = 0;
    std::string oldest_item_date;
    std::string newest_item_date;
    std::map<std::string, int> tag_counts;
    std::map<std::string, int> content_type_counts;
    
    nlohmann::json to_json() const;
    void from_json(const nlohmann::json& j);
};

// 内容客户端类
class ContentClient {
public:
    // 构造函数
    explicit ContentClient(const MCPClientConfig& config = MCPClientConfig{});
    explicit ContentClient(std::shared_ptr<MCPClient> mcp_client);
    explicit ContentClient(std::shared_ptr<HttpClient> http_client, const std::string& base_url);
    
    ~ContentClient();
    
    // 禁用拷贝构造和赋值
    ContentClient(const ContentClient&) = delete;
    ContentClient& operator=(const ContentClient&) = delete;
    
    // 移动构造和赋值
    ContentClient(ContentClient&&) noexcept;
    ContentClient& operator=(ContentClient&&) noexcept;
    
    // 连接管理
    bool connect();
    void disconnect();
    bool is_connected() const;
    
    // 内容管理 - MCP协议方式
    ContentResponse<ContentItem> create_content(const CreateContentRequest& request);
    ContentResponse<ContentItem> get_content(int64_t id);
    ContentResponse<ContentItem> update_content(int64_t id, const UpdateContentRequest& request);
    ContentResponse<bool> delete_content(int64_t id);
    
    // 查询和搜索 - MCP协议方式
    ContentResponse<PagedResult<ContentItem>> search_content(const SearchOptions& options);
    ContentResponse<PagedResult<ContentItem>> list_content(int page = 1, int page_size = 20);
    ContentResponse<std::vector<std::string>> get_tags();
    ContentResponse<ContentStatistics> get_statistics();
    
    // 内容管理 - REST API方式
    ContentResponse<ContentItem> create_content_rest(const CreateContentRequest& request);
    ContentResponse<ContentItem> get_content_rest(int64_t id);
    ContentResponse<ContentItem> update_content_rest(int64_t id, const UpdateContentRequest& request);
    ContentResponse<bool> delete_content_rest(int64_t id);
    
    // 查询和搜索 - REST API方式
    ContentResponse<PagedResult<ContentItem>> search_content_rest(const SearchOptions& options);
    ContentResponse<PagedResult<ContentItem>> list_content_rest(int page = 1, int page_size = 20);
    ContentResponse<std::vector<std::string>> get_tags_rest();
    ContentResponse<ContentStatistics> get_statistics_rest();
    
    // 批量操作
    ContentResponse<std::vector<ContentItem>> create_content_batch(const std::vector<CreateContentRequest>& requests);
    ContentResponse<std::vector<ContentItem>> get_content_batch(const std::vector<int64_t>& ids);
    ContentResponse<bool> delete_content_batch(const std::vector<int64_t>& ids);
    
    // 导入导出
    ContentResponse<bool> export_content(const std::string& file_path, const std::vector<int64_t>& ids = {});
    ContentResponse<std::vector<ContentItem>> import_content(const std::string& file_path);
    
    // 配置管理
    void set_mcp_config(const MCPClientConfig& config);
    void set_http_base_url(const std::string& base_url);
    void set_preferred_protocol(const std::string& protocol); // "mcp" or "rest"
    
    // 错误处理
    std::string get_last_error() const;
    void clear_error();
    
    // 回调函数
    using ProgressCallback = std::function<void(int current, int total, const std::string& operation)>;
    void set_progress_callback(ProgressCallback callback);
    
    // 缓存管理
    void enable_cache(bool enable = true);
    void clear_cache();
    void set_cache_ttl(std::chrono::seconds ttl);
    
    // 统计信息
    struct ClientStatistics {
        size_t total_requests = 0;
        size_t successful_requests = 0;
        size_t failed_requests = 0;
        size_t cache_hits = 0;
        size_t cache_misses = 0;
        std::chrono::milliseconds total_response_time{0};
        
        nlohmann::json to_json() const;
        void reset();
    };
    
    const ClientStatistics& get_client_statistics() const;
    void reset_client_statistics();
    
private:
    class Impl;
    std::unique_ptr<Impl> pimpl_;
    
    // 内部方法
    template<typename T>
    ContentResponse<T> handle_mcp_response(const MCPResponse& response);
    
    template<typename T>
    ContentResponse<T> handle_http_response(const HttpResponse& response);
    
    void handle_error(const std::string& error);
    void update_statistics(bool success, std::chrono::milliseconds response_time);
    
    // 缓存相关
    template<typename T>
    std::optional<T> get_from_cache(const std::string& key);
    
    template<typename T>
    void put_to_cache(const std::string& key, const T& value);
    
    std::string build_cache_key(const std::string& operation, const std::string& params);
};

// 便利函数
namespace content_utils {
    // 内容验证
    bool validate_content_item(const ContentItem& item, std::string& error_message);
    bool validate_create_request(const CreateContentRequest& request, std::string& error_message);
    bool validate_update_request(const UpdateContentRequest& request, std::string& error_message);
    
    // 内容转换
    CreateContentRequest content_item_to_create_request(const ContentItem& item);
    UpdateContentRequest content_item_to_update_request(const ContentItem& item);
    
    // 搜索选项构建
    SearchOptions build_search_options(const std::string& query, 
                                      const std::vector<std::string>& tags = {},
                                      int page = 1, int page_size = 20);
    
    // 标签处理
    std::vector<std::string> parse_tags(const std::string& tags_string, char delimiter = ',');
    std::string format_tags(const std::vector<std::string>& tags, char delimiter = ',');
    std::vector<std::string> normalize_tags(const std::vector<std::string>& tags);
    
    // 内容格式化
    std::string format_content_summary(const ContentItem& item, size_t max_length = 100);
    std::string format_content_title(const ContentItem& item, size_t max_length = 50);
    
    // 时间处理
    std::string format_timestamp(const std::string& timestamp);
    std::string get_relative_time(const std::string& timestamp);
    
    // 导入导出
    nlohmann::json export_content_to_json(const std::vector<ContentItem>& items);
    std::vector<ContentItem> import_content_from_json(const nlohmann::json& json);
    bool export_content_to_file(const std::vector<ContentItem>& items, const std::string& file_path);
    std::vector<ContentItem> import_content_from_file(const std::string& file_path);
    
    // 统计分析
    ContentStatistics analyze_content(const std::vector<ContentItem>& items);
    std::map<std::string, int> count_tags(const std::vector<ContentItem>& items);
    std::map<std::string, int> count_content_types(const std::vector<ContentItem>& items);
    
    // 错误处理
    std::string format_content_error(const std::string& operation, const std::string& details);
    
    // 配置加载
    MCPClientConfig load_content_client_config(const std::string& file_path);
    bool save_content_client_config(const MCPClientConfig& config, const std::string& file_path);
}

} // namespace mcp