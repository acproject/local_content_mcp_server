// handler.cpp
#include "handler.hpp"
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

using json = nlohmann::json;

bool handler::parse_msg(const std::string& raw,
                        std::string& cmd,
                        std::string& payload) {
    try {
        auto js = json::parse(raw);
        cmd = js.at("cmd").get<std::string>();
        payload = js.dump(); // 你可以直接把原文传给插件
        return true;
    } catch (const std::exception& e) {
        spdlog::error("parse_msg error: {}", e.what());
        return false;
    }
}
