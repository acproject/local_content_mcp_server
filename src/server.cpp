// server.cpp
#include "server.hpp"
#include "handler.hpp"
#include <spdlog/spdlog.h>
#include <boost/asio/dispatch.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast/core/buffers_to_string.hpp>

Server::Server(boost::asio::io_context& io,
               const std::string& host,
               unsigned short port)
    : io_(io),
      acceptor_(io, tcp::endpoint(boost::asio::ip::make_address(host), port)) {
    do_accept();
}

void Server::run() {
    io_.run();
}

void Server::add_handler(const std::string& cmd,
                         std::function<void(Connection&, const std::string&)> fn) {
    handlers_[cmd] = std::move(fn);
}

void Server::do_accept() {
    acceptor_.async_accept(
        [this](boost::system::error_code ec, tcp::socket socket) {
            if (!ec) {
                auto conn = std::make_shared<Connection>(std::move(socket), *this, io_);
                conn->start();
            } else {
                spdlog::error("accept error: {}", ec.message());
            }
            do_accept();
        });
}

Connection::Connection(tcp::socket socket,
                       Server& server,
                       boost::asio::io_context& io)
    : socket_(std::move(socket)),
      server_(server),
      io_(io) {}

void Connection::start() {
    do_read();
}

void Connection::send(const std::string& msg) {
    auto self = shared_from_this();
    boost::asio::post(io_,
        [self, msg] {
            boost::asio::async_write(self->socket_,
                boost::asio::buffer(msg),
                [self](boost::system::error_code ec, std::size_t) {
                    if (ec) {
                        spdlog::error("write error: {}", ec.message());
                        self->close();
                    }
                });
        });
}

void Connection::close() {
    boost::system::error_code ignored_ec;
    socket_.shutdown(tcp::socket::shutdown_both, ignored_ec);
    socket_.close(ignored_ec);
}

void Connection::do_read() {
    auto self = shared_from_this();
    boost::asio::async_read_until(socket_, buffer_, '\n',
        [self](boost::system::error_code ec, std::size_t bytes_transferred) {
            if (!ec) {
                std::string msg = boost::beast::buffers_to_string(self->buffer_.data());
                self->buffer_.consume(bytes_transferred);

                // 解析消息（JSON/Protobuf）
                std::string cmd;
                std::string payload;
                if (!handler::parse_msg(msg, cmd, payload)) {
                    self->send("error: malformed\n");
                } else {
                    if (auto it = self->server_.handlers_.find(cmd);
                        it != self->server_.handlers_.end()) {
                        it->second(*self, payload);
                    } else {
                        self->send("error: unknown command\n");
                    }
                }

                self->do_read();
            } else {
                spdlog::error("read error: {}", ec.message());
                self->close();
            }
        });
}
