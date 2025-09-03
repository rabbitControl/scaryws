/* A websocket client using Boost.Beast
 *
 * (C) Copyright Ingo Randolf 2025.
 *
 * Distributed under the Boost Software License, Version 1.0. (See
 * accompanying file LICENSE or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 */

#include "ClientSessionSSL.h"

#ifdef WSLIB_CLIENT_SESSION_VERBOSE
#include <iostream>
#endif

#include <boost/certify/extensions.hpp>
#include "boost/certify/https_verification.hpp"


namespace scaryws
{

ClientSessionSSL::ClientSessionSSL(net::io_context& ioc,
                         ssl::context& ctx,
                         bool binary)
    : ClientSessionBase(ioc)
    , m_socket(net::make_strand(ioc), ctx)
{
    m_socket.binary(binary);
}

void ClientSessionSSL::send(const std::string& str)
{
    m_queue.push_back(std::make_shared<std::vector<char>>(str.begin(), str.end()));

    if (m_queue.size() > 1)
    {
        return;
    }

    sendNext();
}

void ClientSessionSSL::send(const std::vector<char>& data)
{
    m_queue.push_back(std::make_shared<std::vector<char>>(data));

    if (m_queue.size() > 1)
    {
        return;
    }

    sendNext();
}

void ClientSessionSSL::sendNext()
{
    if (!m_queue.empty())
    {
        m_socket.async_write(
            net::buffer(*m_queue.front()),
            beast::bind_front_handler(&ClientSessionSSL::on_write,
                                      shared_from_this()));
    }
}

void ClientSessionSSL::close()
{
    m_socket.async_close(
        websocket::close_code::normal,
        beast::bind_front_handler(&ClientSessionSSL::on_close,
                                  shared_from_this()));
}

bool ClientSessionSSL::isConnected() const
{
    return m_socket.is_open();
}


void ClientSessionSSL::run(const boost::urls::url& url)
{
    m_url = url;

    boost::certify::set_server_hostname(m_socket.next_layer(), m_url.host());
    boost::certify::sni_hostname(m_socket.next_layer(), m_url.host());

    if (m_url.port().empty())
    {
        m_url.set_port("443");
    }

    // // Look up the domain name
    m_resolver.async_resolve(
        m_url.host(),
        m_url.port(),
        beast::bind_front_handler(&ClientSessionSSL::on_resolve,
                                  shared_from_this()));

}

void ClientSessionSSL::on_resolve(beast::error_code ec,
                                  tcp::resolver::results_type results)
{
    if (ec)
    {
        return fail(ec, std::string("resolve: ") + m_url.host());
    }

    // // Set a timeout on the operation
    beast::get_lowest_layer(m_socket).expires_after(std::chrono::seconds(30));

    // Make the connection on the IP address we get from a lookup
    beast::get_lowest_layer(m_socket).async_connect(
        results,
        beast::bind_front_handler(&ClientSessionSSL::on_connect,
                                  shared_from_this()));
}

void ClientSessionSSL::on_connect(beast::error_code ec,
                             tcp::resolver::results_type::endpoint_type ep)
{
    if (ec)
    {
        return fail(ec, "connect");
    }

    // no delay
    beast::get_lowest_layer(m_socket).socket().set_option(tcp::no_delay(true));

    // set timeout
    beast::get_lowest_layer(m_socket).expires_after(std::chrono::seconds(30));

    // handshake
    m_socket.next_layer().async_handshake(
        ssl::stream_base::client,
        beast::bind_front_handler(&ClientSessionSSL::on_ssl_handshake,
                                  shared_from_this()));
}

void
ClientSessionSSL::on_ssl_handshake(beast::error_code ec)
{
    if (ec)
    {
        return fail(ec, "ssl_handshake");
    }

    // Turn off the timeout on the tcp_stream, because
    // the websocket stream has its own timeout system.
    beast::get_lowest_layer(m_socket).expires_never();

    // Set suggested timeout settings for the websocket
    m_socket.set_option(
        websocket::stream_base::timeout::suggested(
            beast::role_type::client));

    // Set a decorator to change the User-Agent of the handshake
    // m_socket.set_option(websocket::stream_base::decorator(
    //     [](websocket::request_type& req)
    // {
    //     req.set(http::field::user_agent,
    //             std::string(BOOST_BEAST_VERSION_STRING) +
    //                 " rcp-websocket-client");
    // }));


    // Update the host_ string. This will provide the value of the
    // Host HTTP header during the WebSocket handshake.
    // See https://tools.ietf.org/html/rfc7230#section-5.4
    std::string host_port = m_url.host() + ":" + std::string(m_url.port());
    std::string path_query = (m_url.path().empty() ? "/" : m_url.path()) + (m_url.query().empty() ? "" : ("?" + m_url.query()));

    // websocket handshake
    m_socket.async_handshake(
        host_port,
        path_query,
        beast::bind_front_handler(&ClientSessionSSL::on_handshake,
                                  shared_from_this()));
}


void
ClientSessionSSL::on_handshake(beast::error_code ec)
{
    if (ec)
    {
        return fail(ec, "handshake");
    }

    if (m_listener)
    {
        m_listener->connected();
    }

    m_socket.async_read(
        m_buffer,
        beast::bind_front_handler(&ClientSessionSSL::on_read,
                                  shared_from_this()));
}

void ClientSessionSSL::on_read(beast::error_code ec,
                               std::size_t bytes_transferred)
{
    if (ec == websocket::error::closed ||
        ec == boost::asio::error::operation_aborted)
    {
        return;
    }

    if (ec)
    {
        return fail(ec, "read");
    }

    ClientSessionBase::receivedData(ec,
                              bytes_transferred,
                              m_socket.got_binary());

    // read more
    m_socket.async_read(
        m_buffer,
        beast::bind_front_handler(&ClientSessionSSL::on_read,
                                  shared_from_this()));
}

void
ClientSessionSSL::on_close(beast::error_code ec)
{
    if (ec)
    {
        return fail(ec, "close");
    }

    // If we get here then the connection is closed gracefully
#ifdef WSLIB_CLIENT_SESSION_VERBOSE
    auto reason = m_socket.reason();
    std::cout << "closed ssl (" << reason.code << "): " << reason.reason << std::endl;
#endif
}

} // namespace scaryws
