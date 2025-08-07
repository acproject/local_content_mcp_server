// plugin.cpp
#include "plugin.hpp"
#include <spdlog/spdlog.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif

std::vector<PluginLoader> load_plugins(const std::string& /* dir */) {
    std::vector<PluginLoader> res;
    
#ifdef _WIN32
    std::vector<std::string> files = {"./plugins/echo_plugin.dll"};
#else
    std::vector<std::string> files = {"./plugins/echo_plugin.so"};
#endif
    
    for (auto& file : files) {
#ifdef _WIN32
        HMODULE h = LoadLibraryA(file.c_str());
        if (!h) {
            spdlog::error("LoadLibrary {} failed: {}", file, GetLastError());
            continue;
        }
        auto fn = (PluginCreateFn)GetProcAddress(h, "create_plugin");
        if (!fn) {
            spdlog::error("GetProcAddress create_plugin failed: {}", GetLastError());
            FreeLibrary(h);
            continue;
        }
        res.push_back({file, (void*)h, fn});
#else
        void* h = dlopen(file.c_str(), RTLD_NOW);
        if (!h) {
            spdlog::error("dlopen {} failed: {}", file, dlerror());
            continue;
        }
        auto fn = (PluginCreateFn)dlsym(h, "create_plugin");
        if (!fn) {
            spdlog::error("dlsym create_plugin failed: {}", dlerror());
            dlclose(h);
            continue;
        }
        res.push_back({file, h, fn});
#endif
    }
    return res;
}

void unload_plugins(std::vector<PluginLoader>& plugins) {
    for (auto& p : plugins) {
#ifdef _WIN32
        FreeLibrary((HMODULE)p.handle);
#else
        dlclose(p.handle);
#endif
    }
}
