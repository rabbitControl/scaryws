/* A websocket server using Boost.Beast
 *
 * (C) Copyright Ingo Randolf 2025.
 *
 * Distributed under the Boost Software License, Version 1.0. (See
 * accompanying file LICENSE or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 */

#ifndef SCARYWS_WEBSOCKET_SERVER_H
#define SCARYWS_WEBSOCKET_SERVER_H

#include <thread>

#include <boost/beast/core.hpp>
#include <boost/asio/ip/tcp.hpp>

#include "IServerSessionListener.h"

namespace beast = boost::beast;
namespace net = boost::asio;
using tcp = boost::asio::ip::tcp;

namespace scaryws
{

class ServerListener;

class WebsocketServer
    : public IServerSessionListener
{
public:
    WebsocketServer();
    ~WebsocketServer();

    uint16_t port() const;
    std::string address() const;

    // send binary data - default: true
    void binary(bool binary);
    bool binary() const;

    void listen(uint16_t port, const std::string& address = "");
    bool isListening() const;
    void close();

    size_t clientCount() const;

    // send text data
    void sendToAll(const std::string& str, void* except = nullptr);
    void sendTo(const std::string& str, void* client);

    // send binary data
    void sendToAll(const std::vector<char>& str, void* except = nullptr);
    void sendTo(const std::vector<char>& data, void* client);

public:
    // IServerSessionListener
    virtual void listening() override;
    virtual void closed() override;
    virtual void clientConnected(void* client) override;
    virtual void clientDisconnected(void* client) override;
    virtual void received(const char* data, size_t size, void* client) override;
    virtual void received(const std::string& msg, void* client) override;

private:
    void run();

private:
    bool m_binary{true};

    net::ip::address m_address;
    uint16_t m_port{0};

    std::shared_ptr<ServerListener> m_listener;

    std::thread* m_thread{nullptr};
    mutable std::mutex m_mutex;
};

} // namespace scaryws


#endif // SCARYWS_WEBSOCKET_SERVER_H
