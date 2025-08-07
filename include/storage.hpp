// storage.hpp
#pragma once
#include <string>
#include <unordered_map>
#include <memory>
#include <hiredis/hiredis.h>

class Redis {
public:
    static Redis& instance();
    bool set(const std::string& key, const std::string& val);
    std::string get(const std::string& key);
    // 其它辅助接口…

private:
    Redis();
    redisContext* ctx_;
};
