//
// Created by acpro on 25-8-7.
//

#pragma once
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
        return;
    }
    ifs >> j_;
}

const std::string& Config::get(const std::string& key) const {
    return j_.at(key).get<std::string>();
}
