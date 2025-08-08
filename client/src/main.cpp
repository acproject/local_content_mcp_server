#include "mcp_client.hpp"
#include "content_client.hpp"
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <chrono>
#include <thread>

using namespace mcp;

// 命令行参数解析
struct CommandLineArgs {
    std::string command;
    std::string server_url = "http://localhost:8080";
    std::string config_file;
    bool verbose = false;
    bool use_mcp = true;
    std::vector<std::string> args;
};

// 解析命令行参数
CommandLineArgs parse_args(int argc, char* argv[]) {
    CommandLineArgs args;
    
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        
        if (arg == "--help" || arg == "-h") {
            args.command = "help";
            return args;
        } else if (arg == "--verbose" || arg == "-v") {
            args.verbose = true;
        } else if (arg == "--rest") {
            args.use_mcp = false;
        } else if (arg == "--server" || arg == "-s") {
            if (i + 1 < argc) {
                args.server_url = argv[++i];
            }
        } else if (arg == "--config" || arg == "-c") {
            if (i + 1 < argc) {
                args.config_file = argv[++i];
            }
        } else if (args.command.empty()) {
            args.command = arg;
        } else {
            args.args.push_back(arg);
        }
    }
    
    return args;
}

// 打印帮助信息
void print_help() {
    std::cout << "Local Content MCP Client\n\n";
    std::cout << "Usage: mcp_client [OPTIONS] COMMAND [ARGS...]\n\n";
    std::cout << "Commands:\n";
    std::cout << "  create <title> <content> [tags...]  Create new content\n";
    std::cout << "  get <id>                           Get content by ID\n";
    std::cout << "  update <id> [options]              Update content\n";
    std::cout << "  delete <id>                        Delete content\n";
    std::cout << "  search <query> [tags...]           Search content\n";
    std::cout << "  list [page] [page_size]            List all content\n";
    std::cout << "  tags                               Get all tags\n";
    std::cout << "  stats                              Get statistics\n";
    std::cout << "  test                               Test connection\n";
    std::cout << "  interactive                        Interactive mode\n";
    std::cout << "\nOptions:\n";
    std::cout << "  -h, --help                         Show this help\n";
    std::cout << "  -v, --verbose                      Verbose output\n";
    std::cout << "  -s, --server <url>                 Server URL (default: http://localhost:8080)\n";
    std::cout << "  -c, --config <file>                Config file path\n";
    std::cout << "  --rest                             Use REST API instead of MCP\n";
    std::cout << "\nExamples:\n";
    std::cout << "  mcp_client create \"My Note\" \"This is content\" tag1 tag2\n";
    std::cout << "  mcp_client search \"keyword\" tag1\n";
    std::cout << "  mcp_client list 1 10\n";
    std::cout << "  mcp_client --rest get 123\n";
}

// 创建内容客户端
std::unique_ptr<ContentClient> create_client(const CommandLineArgs& args) {
    if (args.use_mcp) {
        // 使用MCP协议
        MCPClientConfig config;
        config.server_host = args.server_url;
        config.timeout_seconds = 30;
        config.max_retries = 3;
        config.retry_delay_ms = 1000;
        
        if (!args.config_file.empty()) {
            try {
                config = content_utils::load_content_client_config(args.config_file);
            } catch (const std::exception& e) {
                spdlog::warn("Failed to load config from {}: {}", args.config_file, e.what());
            }
        }
        
        return std::make_unique<ContentClient>(config);
    } else {
        // 使用REST API
        HttpRequestConfig http_config;
        http_config.timeout = std::chrono::seconds(30);
        http_config.max_retries = 3;
        http_config.retry_delay = std::chrono::milliseconds(1000);
        
        auto http_client = std::make_shared<HttpClient>(http_config);
        return std::make_unique<ContentClient>(http_client, args.server_url);
    }
}

// 执行创建内容命令
int cmd_create(ContentClient& client, const std::vector<std::string>& args) {
    if (args.size() < 2) {
        std::cerr << "Error: create command requires title and content\n";
        return 1;
    }
    
    CreateContentRequest request;
    request.title = args[0];
    request.content = args[1];
    
    // 添加标签
    for (size_t i = 2; i < args.size(); ++i) {
        request.tags.push_back(args[i]);
    }
    
    auto response = client.create_content(request);
    
    if (response.success) {
        std::cout << "Content created successfully:\n";
        std::cout << "ID: " << response.data.id << "\n";
        std::cout << "Title: " << response.data.title << "\n";
        std::cout << "Created: " << response.data.created_at << "\n";
        return 0;
    } else {
        std::cerr << "Error: " << response.error_message << "\n";
        return 1;
    }
}

// 执行获取内容命令
int cmd_get(ContentClient& client, const std::vector<std::string>& args) {
    if (args.empty()) {
        std::cerr << "Error: get command requires content ID\n";
        return 1;
    }
    
    try {
        int64_t id = std::stoll(args[0]);
        auto response = client.get_content(id);
        
        if (response.success) {
            const auto& item = response.data;
            std::cout << "ID: " << item.id << "\n";
            std::cout << "Title: " << item.title << "\n";
            std::cout << "Content: " << item.content << "\n";
            std::cout << "Tags: ";
            for (size_t i = 0; i < item.tags.size(); ++i) {
                if (i > 0) std::cout << ", ";
                std::cout << item.tags[i];
            }
            std::cout << "\n";
            std::cout << "Type: " << item.content_type << "\n";
            std::cout << "Created: " << item.created_at << "\n";
            std::cout << "Updated: " << item.updated_at << "\n";
            return 0;
        } else {
            std::cerr << "Error: " << response.error_message << "\n";
            return 1;
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: Invalid ID format\n";
        return 1;
    }
}

// 执行删除内容命令
int cmd_delete(ContentClient& client, const std::vector<std::string>& args) {
    if (args.empty()) {
        std::cerr << "Error: delete command requires content ID\n";
        return 1;
    }
    
    try {
        int64_t id = std::stoll(args[0]);
        auto response = client.delete_content(id);
        
        if (response.success) {
            std::cout << "Content deleted successfully\n";
            return 0;
        } else {
            std::cerr << "Error: " << response.error_message << "\n";
            return 1;
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: Invalid ID format\n";
        return 1;
    }
}

// 执行搜索内容命令
int cmd_search(ContentClient& client, const std::vector<std::string>& args) {
    if (args.empty()) {
        std::cerr << "Error: search command requires query\n";
        return 1;
    }
    
    SearchOptions options;
    options.query = args[0];
    options.page = 1;
    options.page_size = 10;
    
    // 添加标签过滤
    for (size_t i = 1; i < args.size(); ++i) {
        options.tags.push_back(args[i]);
    }
    
    auto response = client.search_content(options);
    
    if (response.success) {
        const auto& result = response.data;
        std::cout << "Found " << result.total_count << " items:\n\n";
        
        for (const auto& item : result.items) {
            std::cout << "ID: " << item.id << "\n";
            std::cout << "Title: " << item.title << "\n";
            std::cout << "Summary: " << content_utils::format_content_summary(item, 100) << "\n";
            if (!item.tags.empty()) {
                auto tags_vector = content_utils::parse_tags(item.tags);
                std::cout << "Tags: " << content_utils::format_tags(tags_vector, ',') << "\n";
            }
            std::cout << "Created: " << item.created_at << "\n";
            std::cout << "\n";
        }
        
        if (result.has_next) {
            std::cout << "Use 'list' command with page number to see more results\n";
        }
        
        return 0;
    } else {
        std::cerr << "Error: " << response.error_message << "\n";
        return 1;
    }
}

// 执行列表内容命令
int cmd_list(ContentClient& client, const std::vector<std::string>& args) {
    int page = 1;
    int page_size = 10;
    
    if (args.size() >= 1) {
        try {
            page = std::stoi(args[0]);
        } catch (const std::exception&) {
            std::cerr << "Error: Invalid page number\n";
            return 1;
        }
    }
    
    if (args.size() >= 2) {
        try {
            page_size = std::stoi(args[1]);
        } catch (const std::exception&) {
            std::cerr << "Error: Invalid page size\n";
            return 1;
        }
    }
    
    auto response = client.list_content(page, page_size);
    
    if (response.success) {
        const auto& result = response.data;
        std::cout << "Page " << result.page << " of " << result.total_pages 
                  << " (" << result.total_count << " total items):\n\n";
        
        for (const auto& item : result.items) {
            std::cout << "ID: " << item.id << "\n";
            std::cout << "Title: " << content_utils::format_content_title(item, 50) << "\n";
            if (!item.tags.empty()) {
                auto tags_vector = content_utils::parse_tags(item.tags);
                std::cout << "Tags: " << content_utils::format_tags(tags_vector, ',') << "\n";
            }
            std::cout << "Created: " << item.created_at << "\n";
            std::cout << "\n";
        }
        
        return 0;
    } else {
        std::cerr << "Error: " << response.error_message << "\n";
        return 1;
    }
}

// 执行获取标签命令
int cmd_tags(ContentClient& client, const std::vector<std::string>& args) {
    (void)args; // 避免未使用参数警告
    auto response = client.get_tags();
    
    if (response.success) {
        std::cout << "Available tags (" << response.data.size() << "):\n";
        for (const auto& tag : response.data) {
            std::cout << "  " << tag << "\n";
        }
        return 0;
    } else {
        std::cerr << "Error: " << response.error_message << "\n";
        return 1;
    }
}

// 执行统计命令
int cmd_stats(ContentClient& client, const std::vector<std::string>& args) {
    (void)args; // 避免未使用参数警告
    auto response = client.get_statistics();
    
    if (response.success) {
        const auto& stats = response.data;
        std::cout << "Content Statistics:\n";
        std::cout << "  Total Items: " << stats.total_items << "\n";
        std::cout << "  Total Tags: " << stats.total_tags << "\n";
        std::cout << "  Oldest Item: " << stats.oldest_item_date << "\n";
        std::cout << "  Newest Item: " << stats.newest_item_date << "\n";
        
        if (!stats.tag_counts.empty()) {
            std::cout << "\n  Top Tags:\n";
            // 按计数排序
            std::vector<std::pair<std::string, int>> sorted_tags(stats.tag_counts.begin(), stats.tag_counts.end());
            std::sort(sorted_tags.begin(), sorted_tags.end(), 
                     [](const auto& a, const auto& b) { return a.second > b.second; });
            
            for (size_t i = 0; i < std::min(size_t(10), sorted_tags.size()); ++i) {
                std::cout << "    " << sorted_tags[i].first << ": " << sorted_tags[i].second << "\n";
            }
        }
        
        if (!stats.content_type_counts.empty()) {
            std::cout << "\n  Content Types:\n";
            for (const auto& [type, count] : stats.content_type_counts) {
                std::cout << "    " << type << ": " << count << "\n";
            }
        }
        
        return 0;
    } else {
        std::cerr << "Error: " << response.error_message << "\n";
        return 1;
    }
}

// 执行测试连接命令
int cmd_test(ContentClient& client, const std::vector<std::string>& args) {
    (void)args; // 避免未使用参数警告
    std::cout << "Testing connection...\n";
    
    if (!client.connect()) {
        std::cerr << "Error: Failed to connect to server\n";
        return 1;
    }
    
    std::cout << "Connected successfully\n";
    
    // 测试基本功能
    auto response = client.get_statistics();
    if (response.success) {
        std::cout << "Server is responding (" << response.data.total_items << " items)\n";
        
        // 显示客户端统计
        const auto& client_stats = client.get_client_statistics();
        std::cout << "Client Statistics:\n";
        std::cout << "  Total Requests: " << client_stats.total_requests << "\n";
        std::cout << "  Successful: " << client_stats.successful_requests << "\n";
        std::cout << "  Failed: " << client_stats.failed_requests << "\n";
        std::cout << "  Cache Hits: " << client_stats.cache_hits << "\n";
        std::cout << "  Cache Misses: " << client_stats.cache_misses << "\n";
        
        return 0;
    } else {
        std::cerr << "Error: Server not responding properly: " << response.error_message << "\n";
        return 1;
    }
}

// 交互模式
int cmd_interactive(ContentClient& client, const std::vector<std::string>& args) {
    (void)args; // 避免未使用参数警告
    std::cout << "Entering interactive mode. Type 'help' for commands, 'quit' to exit.\n";
    
    if (!client.connect()) {
        std::cerr << "Error: Failed to connect to server\n";
        return 1;
    }
    
    std::string line;
    while (true) {
        std::cout << "mcp> ";
        if (!std::getline(std::cin, line)) {
            break;
        }
        
        if (line.empty()) {
            continue;
        }
        
        if (line == "quit" || line == "exit") {
            break;
        }
        
        if (line == "help") {
            std::cout << "Available commands:\n";
            std::cout << "  create <title> <content> [tags...]\n";
            std::cout << "  get <id>\n";
            std::cout << "  delete <id>\n";
            std::cout << "  search <query> [tags...]\n";
            std::cout << "  list [page] [page_size]\n";
            std::cout << "  tags\n";
            std::cout << "  stats\n";
            std::cout << "  test\n";
            std::cout << "  clear (clear screen)\n";
            std::cout << "  quit/exit\n";
            continue;
        }
        
        if (line == "clear") {
            std::cout << "\033[2J\033[H"; // ANSI清屏
            continue;
        }
        
        // 解析命令
        std::istringstream iss(line);
        std::vector<std::string> tokens;
        std::string token;
        while (iss >> token) {
            tokens.push_back(token);
        }
        
        if (tokens.empty()) {
            continue;
        }
        
        std::string command = tokens[0];
        std::vector<std::string> cmd_args(tokens.begin() + 1, tokens.end());
        
        try {
            if (command == "create") {
                cmd_create(client, cmd_args);
            } else if (command == "get") {
                cmd_get(client, cmd_args);
            } else if (command == "delete") {
                cmd_delete(client, cmd_args);
            } else if (command == "search") {
                cmd_search(client, cmd_args);
            } else if (command == "list") {
                cmd_list(client, cmd_args);
            } else if (command == "tags") {
                cmd_tags(client, cmd_args);
            } else if (command == "stats") {
                cmd_stats(client, cmd_args);
            } else if (command == "test") {
                cmd_test(client, cmd_args);
            } else {
                std::cout << "Unknown command: " << command << "\n";
            }
        } catch (const std::exception& e) {
            std::cerr << "Error: " << e.what() << "\n";
        }
        
        std::cout << "\n";
    }
    
    std::cout << "Goodbye!\n";
    return 0;
}

int main(int argc, char* argv[]) {
    // 设置日志
    auto console = spdlog::stdout_color_mt("console");
    spdlog::set_default_logger(console);
    
    // 解析命令行参数
    auto args = parse_args(argc, argv);
    
    if (args.verbose) {
        spdlog::set_level(spdlog::level::debug);
    } else {
        spdlog::set_level(spdlog::level::info);
    }
    
    if (args.command.empty() || args.command == "help") {
        print_help();
        return 0;
    }
    
    try {
        // 创建客户端
        auto client = create_client(args);
        if (!client) {
            std::cerr << "Error: Failed to create client\n";
            return 1;
        }
        
        // 设置进度回调（如果需要）
        if (args.verbose) {
            client->set_progress_callback([](int current, int total, const std::string& operation) {
                std::cout << "Progress: " << operation << " (" << current << "/" << total << ")\n";
            });
        }
        
        // 启用缓存
        client->enable_cache(true);
        
        // 执行命令
        if (args.command == "create") {
            return cmd_create(*client, args.args);
        } else if (args.command == "get") {
            return cmd_get(*client, args.args);
        } else if (args.command == "delete") {
            return cmd_delete(*client, args.args);
        } else if (args.command == "search") {
            return cmd_search(*client, args.args);
        } else if (args.command == "list") {
            return cmd_list(*client, args.args);
        } else if (args.command == "tags") {
            return cmd_tags(*client, args.args);
        } else if (args.command == "stats") {
            return cmd_stats(*client, args.args);
        } else if (args.command == "test") {
            return cmd_test(*client, args.args);
        } else if (args.command == "interactive") {
            return cmd_interactive(*client, args.args);
        } else {
            std::cerr << "Error: Unknown command: " << args.command << "\n";
            std::cerr << "Use --help for usage information\n";
            return 1;
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    
    return 0;
}