/* A websocket server using Boost.Beast
 *
 * (C) Copyright Ingo Randolf 2025.
 *
 * Distributed under the Boost Software License, Version 1.0. (See
 * accompanying file LICENSE or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 */

#include "ServerSession.h"

#include <iostream>

namespace scaryws
{

ServerSession::ServerSession(tcp::socket&& socket, bool binary)
    : m_socket(std::move(socket))
{
    m_socket.binary(binary);
}

void ServerSession::send(const std::string& str)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_queue.push_back(std::make_shared<std::vector<char>>(str.begin(), str.end()));

    if (m_queue.size() > 1)
    {
        return;
    }

    sendNext();
}

void ServerSession::send(const std::vector<char>& data)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_queue.push_back(std::make_shared<std::vector<char>>(data));

    if (m_queue.size() > 1)
    {
        return;
    }

    sendNext();
}

void ServerSession::setListener(IServerSessionListener* listener)
{
    m_listener = listener;
}

void ServerSession::sendNext()
{
    if (!m_queue.empty())
    {
        m_socket.async_write(
            net::buffer(*m_queue.front()),
            beast::bind_front_handler(&ServerSession::on_write,
                                      shared_from_this()));
    }
}

void ServerSession::close()
{
    auto self(shared_from_this());
    net::dispatch(m_socket.get_executor(), [self]
    {
        // TODO: gracefully close this connection
        // self->m_socket.close(boost::beast::websocket::normal);
        self->m_socket.next_layer().cancel();
    });
}

// Get on the correct executor
void ServerSession::run(std::function<void(ServerSession*)>&& cb)
{
    m_closedCb = cb;

    // We need to be executing within a strand to perform async operations
    // on the I/O objects in this session. Although not strictly necessary
    // for single-threaded contexts, this example code is written to be
    // thread-safe by default.
    net::dispatch(m_socket.get_executor(),
                  beast::bind_front_handler(&ServerSession::on_run,
                                            shared_from_this()));
}

// Start the asynchronous operation
void ServerSession::on_run()
{
    // Set suggested timeout settings for the websocket
    m_socket.set_option(
        websocket::stream_base::timeout::suggested(
            beast::role_type::server));

    // Set a decorator to change the Server of the handshake
    // m_socket.set_option(websocket::stream_base::decorator(
    //     [](websocket::response_type& res)
    // {
    //     res.set(http::field::server,
    //             std::string(BOOST_BEAST_VERSION_STRING) +
    //                 " websocket-server-async");
    // }));

    // Accept the websocket handshake
    m_socket.async_accept(
        beast::bind_front_handler(&ServerSession::on_accept,
                                  shared_from_this()));
}

void ServerSession::on_accept(beast::error_code ec)
{
    if (ec)
    {
        return fail(ec, "accept");
    }

    if (m_listener)
    {
        m_listener->clientConnected(this);
    }

    do_read();
}

void ServerSession::do_read()
{
    m_socket.async_read(
        m_buffer,
        beast::bind_front_handler(&ServerSession::on_read,
                                  shared_from_this()));
}

void ServerSession::on_read(beast::error_code ec,
                            std::size_t bytes_transferred)
{
    boost::ignore_unused(bytes_transferred);

    if (ec == websocket::error::closed ||
        ec == boost::asio::error::operation_aborted)
    {
        do_close();

        if (m_closedCb)
        {
            m_closedCb(this);
        }

        return;
    }

    if (ec)
    {
        return fail(ec, "read");
    }

    if (m_listener)
    {
        if (m_socket.got_binary())
        {
            m_listener->received(static_cast<const char*>(m_buffer.data().data()),
                                 m_buffer.data().size(),
                                 this);
        }
        else
        {
            m_listener->received(std::string(static_cast<const char*>(m_buffer.data().data()),
                                             m_buffer.data().size()),
                                 this);
        }
    }

    // Clear the buffer
    m_buffer.consume(m_buffer.size());

    do_read();
}

void ServerSession::on_write(beast::error_code ec,
                             std::size_t bytes_transferred)
{
    boost::ignore_unused(bytes_transferred);

    if (ec == websocket::error::closed ||
        ec == boost::asio::error::operation_aborted)
    {
        do_close();

        if (m_closedCb)
        {
            m_closedCb(this);
        }

        return;
    }


    if (ec)
    {
        fail(ec, "write");
    }

    std::lock_guard<std::mutex> lock(m_mutex);

    m_queue.erase(m_queue.begin());

    sendNext();
}

void ServerSession::do_close()
{
    beast::error_code ec;
    beast::get_lowest_layer(m_socket).socket().shutdown(tcp::socket::shutdown_both, ec);
}

void ServerSession::fail(beast::error_code ec, char const* what)
{
    std::cerr << "ServerSession: " << what << ": " << ec.message() << "\n";
}

} // namespace scaryws
