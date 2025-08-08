#include "content_client.hpp"
#include <spdlog/spdlog.h>
#include <fstream>
#include <chrono>
#include <unordered_map>
#include <algorithm>
#include <type_traits>

namespace mcp {

// PagedResult模板特化实现
template<typename T>
nlohmann::json PagedResult<T>::to_json() const {
    nlohmann::json j;
    nlohmann::json items_json = nlohmann::json::array();
    for (const auto& item : items) {
        if constexpr (std::is_same_v<T, ContentItem>) {
            items_json.push_back(item.to_json());
        } else {
            items_json.push_back(item);
        }
    }
    j["items"] = items_json;
    j["total_count"] = total_count;
    j["page"] = page;
    j["page_size"] = page_size;
    j["total_pages"] = total_pages;
    j["has_next"] = has_next;
    j["has_previous"] = has_previous;
    return j;
}

template<typename T>
void PagedResult<T>::from_json(const nlohmann::json& j) {
    if (j.contains("items")) {
        auto items_json = j["items"];
        items.clear();
        for (const auto& item_json : items_json) {
            if constexpr (std::is_same_v<T, ContentItem>) {
                items.push_back(ContentItem::from_json(item_json));
            } else {
                items.push_back(item_json.get<T>());
            }
        }
    }
    if (j.contains("total_count")) {
        total_count = j["total_count"].get<int>();
    }
    if (j.contains("page")) {
        page = j["page"].get<int>();
    }
    if (j.contains("page_size")) {
        page_size = j["page_size"].get<int>();
    }
    if (j.contains("total_pages")) {
        total_pages = j["total_pages"].get<int>();
    }
    if (j.contains("has_next")) {
        has_next = j["has_next"].get<bool>();
    }
    if (j.contains("has_previous")) {
        has_previous = j["has_previous"].get<bool>();
    }
}

// SearchOptions实现
nlohmann::json SearchOptions::to_json() const {
    nlohmann::json j;
    j["query"] = query;
    j["tags"] = tags;
    j["page"] = page;
    j["page_size"] = page_size;
    j["sort_by"] = sort_by;
    j["sort_order"] = sort_order;
    return j;
}

void SearchOptions::from_json(const nlohmann::json& j) {
    if (j.contains("query")) {
        query = j["query"].get<std::string>();
    }
    if (j.contains("tags")) {
        tags = j["tags"].get<std::vector<std::string>>();
    }
    if (j.contains("page")) {
        page = j["page"].get<int>();
    }
    if (j.contains("page_size")) {
        page_size = j["page_size"].get<int>();
    }
    if (j.contains("sort_by")) {
        sort_by = j["sort_by"].get<std::string>();
    }
    if (j.contains("sort_order")) {
        sort_order = j["sort_order"].get<std::string>();
    }
}

// CreateContentRequest实现
nlohmann::json CreateContentRequest::to_json() const {
    nlohmann::json j;
    j["title"] = title;
    j["content"] = content;
    j["tags"] = tags;
    j["content_type"] = content_type;
    return j;
}

void CreateContentRequest::from_json(const nlohmann::json& j) {
    if (j.contains("title")) {
        title = j["title"].get<std::string>();
    }
    if (j.contains("content")) {
        content = j["content"].get<std::string>();
    }
    if (j.contains("tags")) {
        tags = j["tags"].get<std::vector<std::string>>();
    }
    if (j.contains("content_type")) {
        content_type = j["content_type"].get<std::string>();
    }
}

// UpdateContentRequest实现
nlohmann::json UpdateContentRequest::to_json() const {
    nlohmann::json j;
    if (title.has_value()) {
        j["title"] = title.value();
    }
    if (content.has_value()) {
        j["content"] = content.value();
    }
    if (tags.has_value()) {
        j["tags"] = tags.value();
    }
    if (content_type.has_value()) {
        j["content_type"] = content_type.value();
    }
    return j;
}

void UpdateContentRequest::from_json(const nlohmann::json& j) {
    if (j.contains("title")) {
        title = j["title"].get<std::string>();
    }
    if (j.contains("content")) {
        content = j["content"].get<std::string>();
    }
    if (j.contains("tags")) {
        tags = j["tags"].get<std::vector<std::string>>();
    }
    if (j.contains("content_type")) {
        content_type = j["content_type"].get<std::string>();
    }
}

// ContentStatistics实现
nlohmann::json ContentStatistics::to_json() const {
    nlohmann::json j;
    j["total_items"] = total_items;
    j["total_tags"] = total_tags;
    j["oldest_item_date"] = oldest_item_date;
    j["newest_item_date"] = newest_item_date;
    j["tag_counts"] = tag_counts;
    j["content_type_counts"] = content_type_counts;
    return j;
}

void ContentStatistics::from_json(const nlohmann::json& j) {
    if (j.contains("total_items")) {
        total_items = j["total_items"].get<int>();
    }
    if (j.contains("total_tags")) {
        total_tags = j["total_tags"].get<int>();
    }
    if (j.contains("oldest_item_date")) {
        oldest_item_date = j["oldest_item_date"].get<std::string>();
    }
    if (j.contains("newest_item_date")) {
        newest_item_date = j["newest_item_date"].get<std::string>();
    }
    if (j.contains("tag_counts")) {
        tag_counts = j["tag_counts"].get<std::map<std::string, int>>();
    }
    if (j.contains("content_type_counts")) {
        content_type_counts = j["content_type_counts"].get<std::map<std::string, int>>();
    }
}

// ClientStatistics实现
nlohmann::json ContentClient::ClientStatistics::to_json() const {
    nlohmann::json j;
    j["total_requests"] = total_requests;
    j["successful_requests"] = successful_requests;
    j["failed_requests"] = failed_requests;
    j["cache_hits"] = cache_hits;
    j["cache_misses"] = cache_misses;
    j["total_response_time_ms"] = total_response_time.count();
    return j;
}

void ContentClient::ClientStatistics::reset() {
    total_requests = 0;
    successful_requests = 0;
    failed_requests = 0;
    cache_hits = 0;
    cache_misses = 0;
    total_response_time = std::chrono::milliseconds{0};
}

// ContentClient::Impl类
class ContentClient::Impl {
public:
    std::shared_ptr<MCPClient> mcp_client;
    std::shared_ptr<HttpClient> http_client;
    std::string http_base_url;
    std::string preferred_protocol = "mcp"; // "mcp" or "rest"
    std::string last_error;
    ProgressCallback progress_callback;
    ClientStatistics stats;
    
    // 缓存相关
    bool cache_enabled = false;
    std::chrono::seconds cache_ttl{300}; // 5分钟
    struct CacheEntry {
        nlohmann::json data;
        std::chrono::steady_clock::time_point timestamp;
    };
    std::unordered_map<std::string, CacheEntry> cache;
    
    explicit Impl(const MCPClientConfig& config) {
        mcp_client = std::make_shared<MCPClient>(config);
    }
    
    explicit Impl(std::shared_ptr<MCPClient> client) 
        : mcp_client(std::move(client)) {
    }
    
    explicit Impl(std::shared_ptr<HttpClient> client, const std::string& base_url) 
        : http_client(std::move(client)), http_base_url(base_url), preferred_protocol("rest") {
    }
    
    bool is_cache_valid(const CacheEntry& entry) const {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - entry.timestamp);
        return elapsed < cache_ttl;
    }
    
    void update_progress(int current, int total, const std::string& operation) {
        if (progress_callback) {
            progress_callback(current, total, operation);
        }
    }
};

// ContentClient实现
ContentClient::ContentClient(const MCPClientConfig& config) 
    : pimpl_(std::make_unique<Impl>(config)) {
}

ContentClient::ContentClient(std::shared_ptr<MCPClient> mcp_client) 
    : pimpl_(std::make_unique<Impl>(std::move(mcp_client))) {
}

ContentClient::ContentClient(std::shared_ptr<HttpClient> http_client, const std::string& base_url) 
    : pimpl_(std::make_unique<Impl>(std::move(http_client), base_url)) {
}

ContentClient::~ContentClient() = default;

ContentClient::ContentClient(ContentClient&&) noexcept = default;
ContentClient& ContentClient::operator=(ContentClient&&) noexcept = default;

bool ContentClient::connect() {
    if (pimpl_->preferred_protocol == "mcp" && pimpl_->mcp_client) {
        return pimpl_->mcp_client->connect();
    }
    return true; // HTTP客户端不需要显式连接
}

void ContentClient::disconnect() {
    if (pimpl_->mcp_client) {
        pimpl_->mcp_client->disconnect();
    }
}

bool ContentClient::is_connected() const {
    if (pimpl_->preferred_protocol == "mcp" && pimpl_->mcp_client) {
        return pimpl_->mcp_client->is_connected();
    }
    return true; // HTTP客户端总是"连接"的
}

// MCP协议方式的内容管理
ContentResponse<ContentItem> ContentClient::create_content(const CreateContentRequest& request) {
    auto start_time = std::chrono::steady_clock::now();
    ContentResponse<ContentItem> response;
    
    try {
        if (!pimpl_->mcp_client) {
            response.success = false;
            response.error_message = "MCP client not available";
            return response;
        }
        
        auto mcp_response = pimpl_->mcp_client->call_tool("create_content", request.to_json());
        response = handle_mcp_response<ContentItem>(mcp_response);
        
        if (response.success) {
            response.data.from_json(mcp_response.data);
        }
        
    } catch (const std::exception& e) {
        response.success = false;
        response.error_message = "Create content failed: " + std::string(e.what());
        handle_error(response.error_message);
    }
    
    auto end_time = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    update_statistics(response.success, duration);
    
    return response;
}

ContentResponse<ContentItem> ContentClient::get_content(int64_t id) {
    auto start_time = std::chrono::steady_clock::now();
    ContentResponse<ContentItem> response;
    
    try {
        // 检查缓存
        std::string cache_key = build_cache_key("get_content", std::to_string(id));
        if (auto cached = get_from_cache<ContentItem>(cache_key)) {
            response.success = true;
            response.data = cached.value();
            pimpl_->stats.cache_hits++;
            return response;
        }
        pimpl_->stats.cache_misses++;
        
        if (!pimpl_->mcp_client) {
            response.success = false;
            response.error_message = "MCP client not available";
            return response;
        }
        
        nlohmann::json args;
        args["id"] = id;
        
        auto mcp_response = pimpl_->mcp_client->call_tool("get_content", args);
        response = handle_mcp_response<ContentItem>(mcp_response);
        
        if (response.success) {
            response.data.from_json(mcp_response.data);
            
            // 缓存结果
            put_to_cache(cache_key, response.data);
        }
        
    } catch (const std::exception& e) {
        response.success = false;
        response.error_message = "Get content failed: " + std::string(e.what());
        handle_error(response.error_message);
    }
    
    auto end_time = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    update_statistics(response.success, duration);
    
    return response;
}

ContentResponse<ContentItem> ContentClient::update_content(int64_t id, const UpdateContentRequest& request) {
    auto start_time = std::chrono::steady_clock::now();
    ContentResponse<ContentItem> response;
    
    try {
        if (!pimpl_->mcp_client) {
            response.success = false;
            response.error_message = "MCP client not available";
            return response;
        }
        
        auto args = request.to_json();
        args["id"] = id;
        
        auto mcp_response = pimpl_->mcp_client->call_tool("update_content", args);
        response = handle_mcp_response<ContentItem>(mcp_response);
        
        if (response.success) {
            response.data.from_json(mcp_response.data);
            
            // 清除相关缓存
            std::string cache_key = build_cache_key("get_content", std::to_string(id));
            pimpl_->cache.erase(cache_key);
        }
        
    } catch (const std::exception& e) {
        response.success = false;
        response.error_message = "Update content failed: " + std::string(e.what());
        handle_error(response.error_message);
    }
    
    auto end_time = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    update_statistics(response.success, duration);
    
    return response;
}

ContentResponse<bool> ContentClient::delete_content(int64_t id) {
    auto start_time = std::chrono::steady_clock::now();
    ContentResponse<bool> response;
    
    try {
        if (!pimpl_->mcp_client) {
            response.success = false;
            response.error_message = "MCP client not available";
            return response;
        }
        
        nlohmann::json args;
        args["id"] = id;
        
        auto mcp_response = pimpl_->mcp_client->call_tool("delete_content", args);
        response = handle_mcp_response<bool>(mcp_response);
        
        if (response.success) {
            response.data = true;
            
            // 清除相关缓存
            std::string cache_key = build_cache_key("get_content", std::to_string(id));
            pimpl_->cache.erase(cache_key);
        }
        
    } catch (const std::exception& e) {
        response.success = false;
        response.error_message = "Delete content failed: " + std::string(e.what());
        handle_error(response.error_message);
    }
    
    auto end_time = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    update_statistics(response.success, duration);
    
    return response;
}

ContentResponse<PagedResult<ContentItem>> ContentClient::search_content(const SearchOptions& options) {
    auto start_time = std::chrono::steady_clock::now();
    ContentResponse<PagedResult<ContentItem>> response;
    
    try {
        if (!pimpl_->mcp_client) {
            response.success = false;
            response.error_message = "MCP client not available";
            return response;
        }
        
        auto mcp_response = pimpl_->mcp_client->call_tool("search_content", options.to_json());
        response = handle_mcp_response<PagedResult<ContentItem>>(mcp_response);
        
        if (response.success) {
            response.data.from_json(mcp_response.data);
        }
        
    } catch (const std::exception& e) {
        response.success = false;
        response.error_message = "Search content failed: " + std::string(e.what());
        handle_error(response.error_message);
    }
    
    auto end_time = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    update_statistics(response.success, duration);
    
    return response;
}

ContentResponse<PagedResult<ContentItem>> ContentClient::list_content(int page, int page_size) {
    auto start_time = std::chrono::steady_clock::now();
    ContentResponse<PagedResult<ContentItem>> response;
    
    try {
        if (!pimpl_->mcp_client) {
            response.success = false;
            response.error_message = "MCP client not available";
            return response;
        }
        
        nlohmann::json args;
        args["page"] = page;
        args["page_size"] = page_size;
        
        auto mcp_response = pimpl_->mcp_client->call_tool("list_content", args);
        response = handle_mcp_response<PagedResult<ContentItem>>(mcp_response);
        
        if (response.success) {
            response.data.from_json(mcp_response.data);
        }
        
    } catch (const std::exception& e) {
        response.success = false;
        response.error_message = "List content failed: " + std::string(e.what());
        handle_error(response.error_message);
    }
    
    auto end_time = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    update_statistics(response.success, duration);
    
    return response;
}

ContentResponse<std::vector<std::string>> ContentClient::get_tags() {
    auto start_time = std::chrono::steady_clock::now();
    ContentResponse<std::vector<std::string>> response;
    
    try {
        if (!pimpl_->mcp_client) {
            response.success = false;
            response.error_message = "MCP client not available";
            return response;
        }
        
        auto mcp_response = pimpl_->mcp_client->call_tool("get_tags", nlohmann::json::object());
        response = handle_mcp_response<std::vector<std::string>>(mcp_response);
        
        if (response.success) {
            response.data = mcp_response.data.get<std::vector<std::string>>();
        }
        
    } catch (const std::exception& e) {
        response.success = false;
        response.error_message = "Get tags failed: " + std::string(e.what());
        handle_error(response.error_message);
    }
    
    auto end_time = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    update_statistics(response.success, duration);
    
    return response;
}

ContentResponse<ContentStatistics> ContentClient::get_statistics() {
    auto start_time = std::chrono::steady_clock::now();
    ContentResponse<ContentStatistics> response;
    
    try {
        if (!pimpl_->mcp_client) {
            response.success = false;
            response.error_message = "MCP client not available";
            return response;
        }
        
        auto mcp_response = pimpl_->mcp_client->call_tool("get_statistics", nlohmann::json::object());
        response = handle_mcp_response<ContentStatistics>(mcp_response);
        
        if (response.success) {
            response.data.from_json(mcp_response.data);
        }
        
    } catch (const std::exception& e) {
        response.success = false;
        response.error_message = "Get statistics failed: " + std::string(e.what());
        handle_error(response.error_message);
    }
    
    auto end_time = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    update_statistics(response.success, duration);
    
    return response;
}

// REST API方式的实现（简化版，主要调用HTTP客户端）
ContentResponse<ContentItem> ContentClient::create_content_rest(const CreateContentRequest& request) {
    ContentResponse<ContentItem> response;
    
    if (!pimpl_->http_client) {
        response.success = false;
        response.error_message = "HTTP client not available";
        return response;
    }
    
    try {
        std::string url = pimpl_->http_base_url + "/api/content";
        auto http_response = pimpl_->http_client->post(url, request.to_json());
        response = handle_http_response<ContentItem>(http_response);
        
        if (response.success && http_response.is_json()) {
            response.data.from_json(http_response.get_json());
        }
        
    } catch (const std::exception& e) {
        response.success = false;
        response.error_message = "REST create content failed: " + std::string(e.what());
        handle_error(response.error_message);
    }
    
    return response;
}

// 其他REST API方法的实现类似...

// 配置管理
void ContentClient::set_mcp_config(const MCPClientConfig& config) {
    if (pimpl_->mcp_client) {
        pimpl_->mcp_client->set_config(config);
    }
}

void ContentClient::set_http_base_url(const std::string& base_url) {
    pimpl_->http_base_url = base_url;
}

void ContentClient::set_preferred_protocol(const std::string& protocol) {
    pimpl_->preferred_protocol = protocol;
}

std::string ContentClient::get_last_error() const {
    return pimpl_->last_error;
}

void ContentClient::clear_error() {
    pimpl_->last_error.clear();
}

void ContentClient::set_progress_callback(ProgressCallback callback) {
    pimpl_->progress_callback = std::move(callback);
}

void ContentClient::enable_cache(bool enable) {
    pimpl_->cache_enabled = enable;
    if (!enable) {
        clear_cache();
    }
}

void ContentClient::clear_cache() {
    pimpl_->cache.clear();
}

void ContentClient::set_cache_ttl(std::chrono::seconds ttl) {
    pimpl_->cache_ttl = ttl;
}

const ContentClient::ClientStatistics& ContentClient::get_client_statistics() const {
    return pimpl_->stats;
}

void ContentClient::reset_client_statistics() {
    pimpl_->stats.reset();
}

// 内部方法实现
template<typename T>
ContentResponse<T> ContentClient::handle_mcp_response(const MCPResponse& response) {
    ContentResponse<T> content_response;
    content_response.success = response.success;
    content_response.error_message = response.error_message;
    content_response.error_code = response.error_code;
    return content_response;
}

template<typename T>
ContentResponse<T> ContentClient::handle_http_response(const HttpResponse& response) {
    ContentResponse<T> content_response;
    content_response.success = response.is_success();
    content_response.error_message = response.error_message;
    content_response.error_code = response.status_code;
    return content_response;
}

void ContentClient::handle_error(const std::string& error) {
    pimpl_->last_error = error;
    spdlog::error("Content Client Error: {}", error);
}

void ContentClient::update_statistics(bool success, std::chrono::milliseconds response_time) {
    pimpl_->stats.total_requests++;
    if (success) {
        pimpl_->stats.successful_requests++;
    } else {
        pimpl_->stats.failed_requests++;
    }
    pimpl_->stats.total_response_time += response_time;
}

template<typename T>
std::optional<T> ContentClient::get_from_cache(const std::string& key) {
    if (!pimpl_->cache_enabled) {
        return std::nullopt;
    }
    
    auto it = pimpl_->cache.find(key);
    if (it != pimpl_->cache.end() && pimpl_->is_cache_valid(it->second)) {
        try {
            T value;
            // 这里需要根据T的类型进行适当的反序列化
            // 简化实现，假设T有from_json方法
            return value;
        } catch (const std::exception&) {
            pimpl_->cache.erase(it);
        }
    }
    
    return std::nullopt;
}

template<typename T>
void ContentClient::put_to_cache(const std::string& key, const T& value) {
    if (!pimpl_->cache_enabled) {
        return;
    }
    
    try {
        Impl::CacheEntry entry;
        // 这里需要根据T的类型进行适当的序列化
        // 简化实现，假设T有to_json方法
        (void)value; // 避免未使用参数警告
        entry.timestamp = std::chrono::steady_clock::now();
        pimpl_->cache[key] = entry;
    } catch (const std::exception& e) {
        spdlog::warn("Failed to cache value: {}", e.what());
    }
}

std::string ContentClient::build_cache_key(const std::string& operation, const std::string& params) {
    return operation + ":" + params;
}

// content_utils命名空间实现
namespace content_utils {

bool validate_content_item(const ContentItem& item, std::string& error_message) {
    if (item.title.empty()) {
        error_message = "Title cannot be empty";
        return false;
    }
    
    if (item.content.empty()) {
        error_message = "Content cannot be empty";
        return false;
    }
    
    if (item.title.length() > 200) {
        error_message = "Title too long (max 200 characters)";
        return false;
    }
    
    if (item.content.length() > 1000000) {
        error_message = "Content too long (max 1MB)";
        return false;
    }
    
    return true;
}

bool validate_create_request(const CreateContentRequest& request, std::string& error_message) {
    if (request.title.empty()) {
        error_message = "Title cannot be empty";
        return false;
    }
    
    if (request.content.empty()) {
        error_message = "Content cannot be empty";
        return false;
    }
    
    if (request.tags.size() > 20) {
        error_message = "Too many tags (max 20)";
        return false;
    }
    
    return true;
}

bool validate_update_request(const UpdateContentRequest& request, std::string& error_message) {
    if (request.title.has_value() && request.title.value().empty()) {
        error_message = "Title cannot be empty";
        return false;
    }
    
    if (request.content.has_value() && request.content.value().empty()) {
        error_message = "Content cannot be empty";
        return false;
    }
    
    if (request.tags.has_value() && request.tags.value().size() > 20) {
        error_message = "Too many tags (max 20)";
        return false;
    }
    
    return true;
}

CreateContentRequest content_item_to_create_request(const ContentItem& item) {
    CreateContentRequest request;
    request.title = item.title;
    request.content = item.content;
    // 将 tags 字符串解析为 vector
    if (!item.tags.empty()) {
        request.tags = parse_tags(item.tags);
    }
    request.content_type = item.content_type;
    return request;
}

UpdateContentRequest content_item_to_update_request(const ContentItem& item) {
    UpdateContentRequest request;
    request.title = item.title;
    request.content = item.content;
    // 将 tags 字符串解析为 vector
    if (!item.tags.empty()) {
        request.tags = parse_tags(item.tags);
    }
    request.content_type = item.content_type;
    return request;
}

SearchOptions build_search_options(const std::string& query, 
                                  const std::vector<std::string>& tags,
                                  int page, int page_size) {
    SearchOptions options;
    options.query = query;
    options.tags = tags;
    options.page = page;
    options.page_size = page_size;
    return options;
}

std::vector<std::string> parse_tags(const std::string& tags_string, char delimiter) {
    std::vector<std::string> tags;
    std::stringstream ss(tags_string);
    std::string tag;
    
    while (std::getline(ss, tag, delimiter)) {
        // 去除前后空格
        tag.erase(0, tag.find_first_not_of(" \t"));
        tag.erase(tag.find_last_not_of(" \t") + 1);
        
        if (!tag.empty()) {
            tags.push_back(tag);
        }
    }
    
    return tags;
}

std::string format_tags(const std::vector<std::string>& tags, char delimiter) {
    std::ostringstream oss;
    for (size_t i = 0; i < tags.size(); ++i) {
        if (i > 0) {
            oss << delimiter << " ";
        }
        oss << tags[i];
    }
    return oss.str();
}

std::vector<std::string> normalize_tags(const std::vector<std::string>& tags) {
    std::vector<std::string> normalized;
    
    for (const auto& tag : tags) {
        std::string normalized_tag = tag;
        
        // 转换为小写
        std::transform(normalized_tag.begin(), normalized_tag.end(), 
                      normalized_tag.begin(), ::tolower);
        
        // 去除前后空格
        normalized_tag.erase(0, normalized_tag.find_first_not_of(" \t"));
        normalized_tag.erase(normalized_tag.find_last_not_of(" \t") + 1);
        
        if (!normalized_tag.empty()) {
            normalized.push_back(normalized_tag);
        }
    }
    
    // 去重
    std::sort(normalized.begin(), normalized.end());
    normalized.erase(std::unique(normalized.begin(), normalized.end()), normalized.end());
    
    return normalized;
}

std::string format_content_summary(const ContentItem& item, size_t max_length) {
    std::string summary = item.content;
    if (summary.length() > max_length) {
        summary = summary.substr(0, max_length - 3) + "...";
    }
    return summary;
}

std::string format_content_title(const ContentItem& item, size_t max_length) {
    std::string title = item.title;
    if (title.length() > max_length) {
        title = title.substr(0, max_length - 3) + "...";
    }
    return title;
}

std::string format_timestamp(const std::string& timestamp) {
    // 简化实现，返回原始时间戳
    return timestamp;
}

std::string get_relative_time(const std::string& timestamp) {
    // 简化实现，返回"some time ago"
    (void)timestamp; // 避免未使用参数警告
    return "some time ago";
}

nlohmann::json export_content_to_json(const std::vector<ContentItem>& items) {
    nlohmann::json j;
    j["version"] = "1.0";
    j["export_time"] = std::time(nullptr);
    
    nlohmann::json items_json = nlohmann::json::array();
    for (const auto& item : items) {
        items_json.push_back(item.to_json());
    }
    j["items"] = items_json;
    
    return j;
}

std::vector<ContentItem> import_content_from_json(const nlohmann::json& json) {
    std::vector<ContentItem> items;
    
    if (json.contains("items") && json["items"].is_array()) {
        for (const auto& item_json : json["items"]) {
            items.push_back(ContentItem::from_json(item_json));
        }
    }
    
    return items;
}

bool export_content_to_file(const std::vector<ContentItem>& items, const std::string& file_path) {
    try {
        auto json = export_content_to_json(items);
        std::ofstream file(file_path);
        if (file.is_open()) {
            file << json.dump(2);
            return true;
        }
    } catch (const std::exception& e) {
        spdlog::error("Failed to export content to file {}: {}", file_path, e.what());
    }
    
    return false;
}

std::vector<ContentItem> import_content_from_file(const std::string& file_path) {
    std::vector<ContentItem> items;
    
    try {
        std::ifstream file(file_path);
        if (file.is_open()) {
            nlohmann::json json;
            file >> json;
            items = import_content_from_json(json);
        }
    } catch (const std::exception& e) {
        spdlog::error("Failed to import content from file {}: {}", file_path, e.what());
    }
    
    return items;
}

ContentStatistics analyze_content(const std::vector<ContentItem>& items) {
    ContentStatistics stats;
    stats.total_items = items.size();
    
    if (items.empty()) {
        return stats;
    }
    
    // 分析标签和内容类型
    stats.tag_counts = count_tags(items);
    stats.content_type_counts = count_content_types(items);
    stats.total_tags = stats.tag_counts.size();
    
    // 找到最早和最晚的项目
    auto oldest = std::min_element(items.begin(), items.end(), 
        [](const ContentItem& a, const ContentItem& b) {
            return a.created_at < b.created_at;
        });
    
    auto newest = std::max_element(items.begin(), items.end(), 
        [](const ContentItem& a, const ContentItem& b) {
            return a.created_at < b.created_at;
        });
    
    if (oldest != items.end()) {
        stats.oldest_item_date = oldest->created_at;
    }
    
    if (newest != items.end()) {
        stats.newest_item_date = newest->created_at;
    }
    
    return stats;
}

std::map<std::string, int> count_tags(const std::vector<ContentItem>& items) {
    std::map<std::string, int> tag_counts;
    
    for (const auto& item : items) {
        // Parse tags string (assuming comma-separated)
        auto tags = parse_tags(item.tags, ',');
        for (const auto& tag : tags) {
            if (!tag.empty()) {
                tag_counts[tag]++;
            }
        }
    }
    
    return tag_counts;
}

std::map<std::string, int> count_content_types(const std::vector<ContentItem>& items) {
    std::map<std::string, int> type_counts;
    
    for (const auto& item : items) {
        type_counts[item.content_type]++;
    }
    
    return type_counts;
}

std::string format_content_error(const std::string& operation, const std::string& details) {
    return "Content operation '" + operation + "' failed: " + details;
}

MCPClientConfig load_content_client_config(const std::string& file_path) {
    return client_utils::load_config_from_file(file_path);
}

bool save_content_client_config(const MCPClientConfig& config, const std::string& file_path) {
    return client_utils::save_config_to_file(config, file_path);
}

} // namespace content_utils

} // namespace mcp