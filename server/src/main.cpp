#include <iostream>
#include <memory>
#include <signal.h>
#include <filesystem>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/rotating_file_sink.h>

#include "config.hpp"
#include "database.hpp"
#include "content_manager.hpp"
#include "mcp_server.hpp"
#include "http_handler.hpp"

using namespace mcp;

// 全局变量用于信号处理
std::unique_ptr<HttpHandler> g_http_handler;
bool g_running = true;

// 信号处理函数
void signal_handler(int signal) {
    spdlog::info("Received signal {}, shutting down...", signal);
    g_running = false;
    
    if (g_http_handler) {
        g_http_handler->stop();
    }
}

// 设置日志系统
void setup_logging(const Config& config) {
    std::vector<spdlog::sink_ptr> sinks;
    
    // 控制台输出 (默认启用)
    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    console_sink->set_level(spdlog::level::from_str(config.get_log_level()));
    sinks.push_back(console_sink);
    
    // 文件输出
    if (!config.get_log_file().empty()) {
        // 确保日志目录存在
        std::filesystem::path log_path(config.get_log_file());
        std::filesystem::create_directories(log_path.parent_path());
        
        auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
            config.get_log_file(),
            10 * 1024 * 1024, // 10MB
            5 // 5 files
        );
        file_sink->set_level(spdlog::level::from_str(config.get_log_level()));
        sinks.push_back(file_sink);
    }
    
    // 创建logger
    auto logger = std::make_shared<spdlog::logger>("mcp_server", sinks.begin(), sinks.end());
    logger->set_level(spdlog::level::from_str(config.get_log_level()));
    logger->flush_on(spdlog::level::warn);
    
    spdlog::set_default_logger(logger);
    spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] [%t] %v");
    
    spdlog::info("Logging system initialized");
}

// 创建必要的目录
void create_directories(const Config& config) {
    // 数据库目录
    std::filesystem::path db_path(config.get_database_path());
    std::filesystem::create_directories(db_path.parent_path());
    
    // 静态文件目录
    if (config.is_static_files_enabled() && !config.get_static_files_path().empty()) {
        std::filesystem::create_directories(config.get_static_files_path());
    }
    
    spdlog::info("Required directories created");
}

// 打印启动信息
void print_startup_info(const Config& config) {
    spdlog::info("========================================");
    spdlog::info("  Local Content MCP Server");
    spdlog::info("========================================");
    spdlog::info("Version: 1.0.0");
    spdlog::info("Build: {} {}", __DATE__, __TIME__);
    spdlog::info("Server: http://{}:{}", config.get_host(), config.get_port());
    spdlog::info("Database: {}", config.get_database_path());
    spdlog::info("Log Level: {}", config.get_log_level());
    spdlog::info("========================================");
}

int main(int argc, char* argv[]) {
    try {
        // 解析命令行参数
        std::string config_file = "config.json";
        if (argc > 1) {
            config_file = argv[1];
        }
        
        // 加载配置
        auto& config = Config::instance();
        if (std::filesystem::exists(config_file)) {
            if (!config.load_from_file(config_file)) {
                std::cerr << "Failed to load config from: " << config_file << std::endl;
                return 1;
            }
        } else {
            std::cout << "Config file not found: " << config_file << ", using defaults" << std::endl;
        }
        
        // 设置日志系统
        setup_logging(config);
        
        // 打印启动信息
        print_startup_info(config);
        
        // 创建必要的目录
        create_directories(config);
        
        // 初始化数据库
        spdlog::info("Initializing database...");
        auto database = std::make_shared<Database>(config.get_database_path());
        if (!database->initialize()) {
            spdlog::error("Failed to initialize database");
            return 1;
        }
        spdlog::info("Database initialized successfully");
        
        // 初始化内容管理器
        spdlog::info("Initializing content manager...");
        auto content_manager = std::make_shared<ContentManager>(database);
        spdlog::info("Content manager initialized successfully");
        
        // 初始化MCP服务器
        spdlog::info("Initializing MCP server...");
        auto mcp_server = std::make_shared<MCPServer>(content_manager);
        spdlog::info("MCP server initialized successfully");
        
        // 初始化HTTP处理器
        spdlog::info("Initializing HTTP handler...");
        g_http_handler = std::make_unique<HttpHandler>(mcp_server);
        
        // 初始化HTTP处理器的附加功能（文件上传和LLaMA）
        if (!g_http_handler->initialize()) {
            spdlog::error("Failed to initialize HTTP handler features");
            return 1;
        }
        spdlog::info("HTTP handler initialized successfully");
        
        // 设置信号处理
        signal(SIGINT, signal_handler);
        signal(SIGTERM, signal_handler);
        
        // 启动HTTP服务器
        spdlog::info("Starting HTTP server...");
        if (!g_http_handler->start(config.get_host(), config.get_port())) {
            spdlog::error("Failed to start HTTP server");
            return 1;
        }
        
        spdlog::info("Server started successfully!");
        spdlog::info("Server is running on http://{}:{}", config.get_host(), config.get_port());
        spdlog::info("Press Ctrl+C to stop the server");
        
        // 主循环
        while (g_running) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        
        // 清理资源
        spdlog::info("Shutting down server...");
        g_http_handler->stop();
        g_http_handler.reset();
        
        spdlog::info("Server shutdown complete");
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        spdlog::error("Fatal error: {}", e.what());
        return 1;
    } catch (...) {
        std::cerr << "Unknown fatal error occurred" << std::endl;
        spdlog::error("Unknown fatal error occurred");
        return 1;
    }
}