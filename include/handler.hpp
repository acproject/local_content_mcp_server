// handler.hpp
#pragma once
#include <string>
#include <unordered_map>
#include <functional>

namespace handler {
    // parse raw string -> command + payload
    bool parse_msg(const std::string& raw,
                   std::string& cmd,
                   std::string& payload);

    using CmdFunc = std::function<void(class Connection&, const std::string&)>;
}
