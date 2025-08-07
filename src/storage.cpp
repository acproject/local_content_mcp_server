// storage.cpp
#include "storage.hpp"
#include <spdlog/spdlog.h>

Redis& Redis::instance() {
    static Redis r;
    return r;
}

Redis::Redis() {
    const char* host = "localhost";
    int port = 6379;
    ctx_ = redisConnect(host, port);
    if (ctx_ == nullptr || ctx_->err) {
        throw std::runtime_error("Redis connection failed");
    }
}

bool Redis::set(const std::string& key, const std::string& val) {
    redisReply* reply = (redisReply*)redisCommand(ctx_, "SET %s %s", key.c_str(), val.c_str());
    bool ok = reply && reply->type == REDIS_REPLY_STATUS && std::string(reply->str) == "OK";
    freeReplyObject(reply);
    return ok;
}

std::string Redis::get(const std::string& key) {
    redisReply* reply = (redisReply*)redisCommand(ctx_, "GET %s", key.c_str());
    std::string result;
    if (reply && reply->type == REDIS_REPLY_STRING) result = reply->str;
    freeReplyObject(reply);
    return result;
}
