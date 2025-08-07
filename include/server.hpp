#pragma once
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <memory>
#include <unordered_map>
#include <functional>

using tcp = boost::asio::ip::tcp;
namespace http = boost::beast::http;
namespace websocket = boost::beast::websocket;

class Connection;
class Server;

class Connection; // 前向声明

class Server {
public:
    Server(boost::asio::io_context& io,
        const std::string& host,
        unsigned short port
    );
    void run();
    void add_handler(const std::string& cmd,
        std::function<void(Connection& , const std::string&)>);
    
    // 添加友元类声明
    friend class Connection;
private:
    void do_accept();
    boost::asio::io_context& io_;
    tcp::acceptor acceptor_;
    std::unordered_map<std::string,
    std::function<void(Connection&, const std::string&)>> handlers_;
};

class Connection : public std::enable_shared_from_this<Connection> {
public:
    Connection(tcp::socket socket,
               Server& server,
               boost::asio::io_context& io);
    void start();

    void send(const std::string& msg);
    void close();

    // 供 handler 访问
    Server& server() { return server_; }
    boost::asio::io_context& io() { return io_; }

private:
    void do_read();

    tcp::socket socket_;
    Server& server_;
    boost::asio::io_context& io_;
    boost::asio::streambuf buffer_;
};