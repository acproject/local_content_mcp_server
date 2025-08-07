#include "config.hpp"
#include "server.hpp"
#include "storage.hpp"
#include "plugin.hpp"
#include <boost/asio.hpp>
#include <spdlog/spdlog.h>
#include <iostream>

int main() {
    try {
        std::cout << "Starting MCP server..." << std::endl;
        Config::instance().load("resources/config.json");

        boost::asio::io_context io;
        Server srv(io,
                   Config::instance().get("host"),
                   static_cast<unsigned short>(std::stoi(Config::instance().get("port"))));

        // 注册核心命令（示例）
        srv.add_handler("login", [](Connection& conn, const std::string& payload){
            // 解析 token
            std::string token = payload; // 这里简化
            if (Redis::instance().set("sess:" + token, "valid")) {
                conn.send("login: ok\n");
            } else {
                conn.send("login: fail\n");
            }
        });

        // 加载插件
        auto plugins = load_plugins("plugins");
        for (auto& p : plugins) {
            auto plugin = p.create();
            plugin->init(srv);
        }

        spdlog::info("MCP server starting on {}:{}", Config::instance().get("host"),
                     Config::instance().get("port"));
        srv.run();

        unload_plugins(plugins);
    } catch (const std::exception& e) {
        spdlog::critical("Fatal error: {}", e.what());
        return EXIT_FAILURE;
    }
}
