#pragma once
#include <string>
#include <unordered_map>
#include <nlohmann/json.hpp>

class Config {
public:
    static Config& instance();

    std::string get(const std::string& key) const;
    void load(const std::string& path);

private:
    Config() = default;
    std::unordered_map<std::string, std::string> config_;
    nlohmann::json j_;
};
