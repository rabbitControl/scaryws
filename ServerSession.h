/* A websocket server using Boost.Beast
 *
 * (C) Copyright Ingo Randolf 2025.
 *
 * Distributed under the Boost Software License, Version 1.0. (See
 * accompanying file LICENSE or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 */

#ifndef SCARYWS_SERVER_SESSION_H
#define SCARYWS_SERVER_SESSION_H

#include "IServerSessionListener.h"

#include <memory>
#include <iostream>

#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>

namespace beast = boost::beast;
namespace http = beast::http;
namespace websocket = beast::websocket;
namespace net = boost::asio;
using tcp = boost::asio::ip::tcp;


namespace scaryws
{

class ServerSession
    : public std::enable_shared_from_this<ServerSession>
{
public:
    ServerSession(tcp::socket&& socket, bool binary);

    void run(std::function<void(ServerSession*)>&& cb = [](ServerSession*){});

    void send(const std::string& str);
    void send(const std::vector<char>& data);

    void setListener(IServerSessionListener* listener)
    {
        m_listener = listener;
    }

private:
    void sendNext();
    void on_run();
    void on_accept(beast::error_code ec);
    void do_read();
    void on_read(beast::error_code ec, std::size_t bytes_transferred);
    void on_write(beast::error_code ec, std::size_t bytes_transferred);

    void fail(beast::error_code ec, char const* what)
    {
        std::cerr << "ServerSession: " << what << ": " << ec.message() << "\n";
    }

private:
    websocket::stream<beast::tcp_stream> m_socket;
    beast::flat_buffer m_buffer;

    std::vector<std::shared_ptr<std::vector<char>>> m_queue;
    std::mutex m_mutex;

    IServerSessionListener* m_listener{nullptr};

    std::function<void(ServerSession*)> m_closedCb;
};

} // namespace scaryws


#endif // SCARYWS_SERVER_SESSION_H
