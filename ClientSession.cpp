/* A websocket client using Boost.Beast
 *
 * (C) Copyright Ingo Randolf 2025.
 *
 * Distributed under the Boost Software License, Version 1.0. (See
 * accompanying file LICENSE or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 */

#include "ClientSession.h"

#ifdef WSLIB_CLIENT_SESSION_VERBOSE
#include <iostream>
#endif

namespace scaryws
{

ClientSession::ClientSession(net::io_context& ioc, bool binary)
    : ClientSessionBase(ioc)
    , m_socket(net::make_strand(ioc))
{
    m_socket.binary(binary);

    // m_socket.control_callback(
    //     [](websocket::frame_type kind, beast::string_view payload)
    // {
    //     cout << "control callback: " << int(kind) << ":" << payload << "\n";
    // });

    // auto fragmenent?
    // m_socket.auto_fragment(false);
    // m_socket.write_buffer_bytes(16384);
}

void ClientSession::send(const std::string& str)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_queue.push_back(std::make_shared<std::vector<char>>(str.begin(), str.end()));

    if (m_queue.size() > 1)
    {
        return;
    }

    sendNext();
}

void ClientSession::send(const std::vector<char>& data)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_queue.push_back(std::make_shared<std::vector<char>>(data));

    if (m_queue.size() > 1)
    {
        return;
    }

    sendNext();
}

void ClientSession::sendNext()
{
    if (!m_queue.empty())
    {
        m_socket.async_write(
            net::buffer(*m_queue.front()),
            beast::bind_front_handler(&ClientSession::on_write,
                                      shared_from_this()));
    }
}

void ClientSession::close()
{
    m_socket.async_close(
        websocket::close_code::normal,
        beast::bind_front_handler(&ClientSession::on_close,
                                  shared_from_this()));
}

bool ClientSession::isConnected() const
{
    return m_socket.is_open();
}


void ClientSession::run(const boost::urls::url& url)
{
    m_url = url;

    if (m_url.port().empty())
    {
        m_url.set_port("80");
    }

    m_resolver.async_resolve(
        m_url.host(),
        m_url.port(),
        beast::bind_front_handler(&ClientSession::on_resolve,
                                  shared_from_this()));

}

void ClientSession::on_resolve(beast::error_code ec,
                tcp::resolver::results_type results)
{
    if (ec)
    {
        return fail(ec, "resolve");
    }

    // // Set a timeout on the operation
    beast::get_lowest_layer(m_socket).expires_after(std::chrono::seconds(30));

    // Make the connection on the IP address we get from a lookup
    beast::get_lowest_layer(m_socket).async_connect(
        results,
        beast::bind_front_handler(&ClientSession::on_connect,
                                  shared_from_this()));
}

void
ClientSession::on_connect(beast::error_code ec, tcp::resolver::results_type::endpoint_type ep)
{
    if (ec)
    {
        return fail(ec, "connect");
    }

    // no delay
    beast::get_lowest_layer(m_socket).socket().set_option(tcp::no_delay(true));

    // set timeout
    beast::get_lowest_layer(m_socket).expires_after(std::chrono::seconds(30));

    // m_socket.set_option(websocket::stream_base::decorator(
    //     [](websocket::request_type& req)
    // {
    //     req.set(http::field::user_agent,
    //             string("rcp-test-client"));
    // }));


    // Update the host_ string. This will provide the value of the
    // Host HTTP header during the WebSocket handshake.
    // See https://tools.ietf.org/html/rfc7230#section-5.4
    std::string host_port = m_url.host() + ":" + std::to_string(ep.port());
    std::string path_query = (m_url.path().empty() ? "/" : m_url.path()) + (m_url.query().empty() ? "" : ("?" + m_url.query()));

    m_socket.async_handshake(
        host_port,
        path_query,
        beast::bind_front_handler(&ClientSession::on_handshake,
                                  shared_from_this()));
}

void
ClientSession::on_handshake(beast::error_code ec)
{
    if (ec)
    {
        return fail(ec, "handshake");
    }

    beast::get_lowest_layer(m_socket).expires_never();

    if (m_listener)
    {
        m_listener->connected();
    }

    m_socket.async_read(
        m_buffer,
        beast::bind_front_handler(&ClientSession::on_read,
                                  shared_from_this()));
}

void ClientSession::on_read(beast::error_code ec,
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
        beast::bind_front_handler(&ClientSession::on_read,
                                  shared_from_this()));

}

void ClientSession::on_close(beast::error_code ec)
{
    if (ec)
    {
        return fail(ec, "close");
    }

    // If we get here then the connection is closed gracefully
#ifdef WSLIB_CLIENT_SESSION_VERBOSE
    auto reason = m_socket.reason();
    std::cout << "closed non-ssl (" << reason.code << "): " << reason.reason << std::endl;
#endif
}

} // namespace scaryws
