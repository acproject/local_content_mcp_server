#include <gtest/gtest.h>
#include "handler.hpp"
#include <nlohmann/json.hpp>

using json = nlohmann::json;

class HandlerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 设置测试环境
    }
    
    void TearDown() override {
        // 清理测试环境
    }
};

TEST_F(HandlerTest, ParseValidMessage) {
    json test_msg;
    test_msg["cmd"] = "test_command";
    test_msg["data"] = "test_payload";
    
    std::string raw = test_msg.dump();
    std::string cmd;
    std::string payload;
    
    EXPECT_TRUE(handler::parse_msg(raw, cmd, payload));
    EXPECT_EQ(cmd, "test_command");
}

TEST_F(HandlerTest, ParseInvalidMessage) {
    std::string raw = "invalid json";
    std::string cmd;
    std::string payload;
    
    EXPECT_FALSE(handler::parse_msg(raw, cmd, payload));
}

TEST_F(HandlerTest, ParseEmptyMessage) {
    std::string raw = "{}";
    std::string cmd;
    std::string payload;
    
    EXPECT_FALSE(handler::parse_msg(raw, cmd, payload));
}