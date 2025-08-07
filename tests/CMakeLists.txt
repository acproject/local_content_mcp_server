add_executable(server_test server_test.cpp handler_test.cpp)

target_link_libraries(server_test PRIVATE 
    GTest::GTest
    GTest::Main
    core
)

add_test(NAME ServerTest COMMAND server_test)