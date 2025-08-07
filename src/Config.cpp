//
// Created by acpro on 25-8-7.
//

#include "config.hpp"
#include <fstream>
#include <spdlog/spdlog.h>

Config& Config::instance() {
    static Config instance;
    return instance;
}

void Config::load(const std::string& path) {
    std::ifstream ifs(path);
    if (!ifs.is_open()) {
        spdlog::error("open config file {} failed", path);
        throw std::runtime_error("Failed to open config file: " + path);
    }
    ifs >> j_;
    spdlog::info("Config loaded successfully from {}", path);
}

std::string Config::get(const std::string& key) const {
    return j_.at(key).get<std::string>();
}
