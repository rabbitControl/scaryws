/* A websocket client using Boost.Beast
 *
 * (C) Copyright Ingo Randolf 2025.
 *
 * Distributed under the Boost Software License, Version 1.0. (See
 * accompanying file LICENSE or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 */

#ifndef SCARYWS_CLIENT_SESSION_BASE_H
#define SCARYWS_CLIENT_SESSION_BASE_H

#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio/strand.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/url.hpp>

#include "IClientSessionListener.h"

// #define WSLIB_CLIENT_SESSION_VERBOSE

#ifdef WSLIB_CLIENT_SESSION_VERBOSE
#include <iostream>
#endif

namespace beast = boost::beast;
namespace websocket = beast::websocket;
namespace net = boost::asio;
using tcp = boost::asio::ip::tcp;

namespace scaryws
{

class ClientSessionBase
{
public:
    explicit ClientSessionBase(net::io_context& ioc);

    void setListener(IClientSessionListener* listener);

public:
    virtual bool isConnected() const = 0;
    virtual void send(const std::string& str) = 0;
    virtual void send(const std::vector<char>& data) = 0;

protected:
    void on_write(beast::error_code ec, std::size_t bytes_transferred);
    virtual void sendNext() = 0;

protected:
    void receivedData(beast::error_code ec,
                      std::size_t bytes_transferred,
                      bool binary);

    void fail(beast::error_code ec, const std::string& what);

protected:
    IClientSessionListener* m_listener{nullptr};

    tcp::resolver m_resolver;
    beast::flat_buffer m_buffer;
    boost::urls::url m_url;

    std::vector<std::shared_ptr<std::vector<char>>> m_queue;
    std::mutex m_mutex;
};

} // namespace scaryws

#endif // SCARYWS_CLIENT_SESSION_BASE_H
