#include <gtest/gtest.h>
#include "server.hpp"
#include "config.hpp"
#include <boost/asio.hpp>

class ServerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 设置测试环境
    }
    
    void TearDown() override {
        // 清理测试环境
    }
};

TEST_F(ServerTest, BasicServerCreation) {
    boost::asio::io_context io;
    EXPECT_NO_THROW({
        Server server(io, "127.0.0.1", 0); // 使用端口0让系统自动分配
    });
}

TEST_F(ServerTest, HandlerRegistration) {
    boost::asio::io_context io;
    Server server(io, "127.0.0.1", 0);
    
    EXPECT_NO_THROW({
        server.add_handler("test", [](Connection& conn, const std::string& payload) {
            conn.send("test response");
        });
    });
}