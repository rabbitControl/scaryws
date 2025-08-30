/* A websocket server using Boost.Beast
 *
 * (C) Copyright Ingo Randolf 2025.
 *
 * Distributed under the Boost Software License, Version 1.0. (See
 * accompanying file LICENSE or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 */

#ifndef SCARYWS_SERVER_LISTENER_H
#define SCARYWS_SERVER_LISTENER_H

#include <memory>

#include <boost/beast/core.hpp>
#include <boost/asio/strand.hpp>

#include "IServerSessionListener.h"
#include "ServerSession.h"

namespace beast = boost::beast;
namespace net = boost::asio;
using tcp = boost::asio::ip::tcp;

namespace scaryws
{

class ServerListener
    : public std::enable_shared_from_this<ServerListener>
{
public:
    ServerListener(net::io_context& ioc, tcp::endpoint endpoint, bool binary);

    void run();
    void setListener(IServerSessionListener* listener);

    void cancel();
    void sendToAll(const std::string& msg, void* except = nullptr);
    void sendToAll(const std::vector<char>& data, void* except = nullptr);
    void sendTo(const std::string& msg, void* client);
    void sendTo(const std::vector<char>& data, void* client);

    bool isListening() const;
    size_t sessionCount() const;

private:
    void fail(beast::error_code ec, char const* what);
    void do_accept();
    void on_accept(beast::error_code ec, tcp::socket socket);

private:
    net::io_context& ioc;
    tcp::acceptor acceptor;

    mutable std::recursive_mutex m_mutex;
    std::vector<std::shared_ptr<ServerSession>> m_sessions;
    bool m_binary{true};

    IServerSessionListener* m_listener{nullptr};
};

} // namespace scaryws

#endif // SCARYWS_SERVER_LISTENER_H
