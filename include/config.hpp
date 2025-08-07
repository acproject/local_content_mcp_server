#pragma once
#include <string>
#include <unordered_map>
#include <nlohmann/json.hpp>

class Config {
public:
    static Config& instance();

    const std::string& get(const std::string& key) const;
    void load(const std::string& path);

private:
    Config() = default;
    nlohmann::json j_;
};
