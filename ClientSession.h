/* A websocket client using Boost.Beast
 *
 * (C) Copyright Ingo Randolf 2025.
 *
 * Distributed under the Boost Software License, Version 1.0. (See
 * accompanying file LICENSE or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 */

#ifndef SCARYWS_CLIENT_SESSION_H
#define SCARYWS_CLIENT_SESSION_H

#include <vector>

#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/websocket/ssl.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/asio/strand.hpp>
#include <boost/asio/local/stream_protocol.hpp>
#include <boost/url.hpp>

#include "ClientSessionBase.h"

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
namespace ssl = boost::asio::ssl;       // from <boost/asio/ssl.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

using stream_protocol = net::local::stream_protocol;

namespace scaryws
{

class ClientSession
    : public ClientSessionBase
    , public std::enable_shared_from_this<ClientSession>
{
public:
    // Resolver and socket require an io_context
    explicit ClientSession(net::io_context& ioc, bool binary = true);

    // Start the asynchronous operation
    void run(const boost::urls::url& url);

    void close();

public:
    // SessionBase
    bool isConnected() const override;    
    void send(const std::string& str) override;
    void send(const std::vector<char>& data) override;

private:
    void sendNext() override;

private:
    void on_resolve(beast::error_code ec, tcp::resolver::results_type results);
    void on_connect(beast::error_code ec, tcp::resolver::results_type::endpoint_type ep);
    void on_handshake(beast::error_code ec);
    void on_read(beast::error_code ec, std::size_t bytes_transferred);
    void on_close(beast::error_code ec);

private:
    websocket::stream<beast::tcp_stream> m_socket;
};

} // namespace scaryws

#endif // SCARYWS_CLIENT_SESSION_H
