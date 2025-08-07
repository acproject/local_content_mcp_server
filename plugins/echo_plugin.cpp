// echo_plugin.cpp
#include "plugin.hpp"
#include "handler.hpp"
#include "server.hpp"

class EchoPlugin : public Plugin {
public:
    void init(Server& server) override {
        server.add_handler("echo",
            [](Connection& conn, const std::string& payload) {
                // 直接原样返回
                conn.send("echo: " + payload + "\n");
            });
    }
};

extern "C" PluginPtr create_plugin() {
    return std::make_unique<EchoPlugin>();
}
