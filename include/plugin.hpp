// plugin.hpp
#pragma once
#include <string>
#include <memory>
#include <functional>
#include "handler.hpp"
#include "server.hpp"

class Plugin {
public:
    virtual ~Plugin() = default;
    virtual void init(Server& server) = 0;
};

using PluginPtr = std::unique_ptr<Plugin>;
using PluginCreateFn = PluginPtr(*)();

struct PluginLoader {
    std::string path;
    void* handle = nullptr;
    PluginCreateFn create = nullptr;
};

std::vector<PluginLoader> load_plugins(const std::string& dir);
void unload_plugins(std::vector<PluginLoader>& plugins);
