/* A websocket server using Boost.Beast
 *
 * (C) Copyright Ingo Randolf 2025.
 *
 * Distributed under the Boost Software License, Version 1.0. (See
 * accompanying file LICENSE or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 */

#include "ServerListener.h"

#include <iostream>

namespace scaryws
{

ServerListener::ServerListener(net::io_context& ioc, tcp::endpoint endpoint, bool binary)
    : m_ioc(ioc)
    , m_acceptor(net::make_strand(ioc))
    , m_binary(binary)
{
    beast::error_code ec;

    m_acceptor.open(endpoint.protocol(), ec);
    if (ec)
    {
        fail(ec, "acceptor open");
        return;
    }

    m_acceptor.set_option(net::socket_base::reuse_address(true), ec);
    if (ec)
    {
        fail(ec, "acceptor set_option");
        return;
    }

    m_acceptor.bind(endpoint, ec);
    if (ec)
    {
        fail(ec, "acceptor bind");
        return;
    }    

    m_acceptor.listen(net::socket_base::max_listen_connections, ec);
    if (ec)
    {
        fail(ec, "acceptor listen");
        return;
    }
}

void ServerListener::run()
{
    do_accept();
}

void ServerListener::setListener(IServerSessionListener* listener)
{
    m_listener = listener;
}


void ServerListener::cancel()
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);

    for (auto& session : m_sessions)
    {
        session->close();
    }

    m_acceptor.cancel();
    m_acceptor.close();
}

void ServerListener::sendToAll(const std::string& msg, void* except)
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);

    for (auto& session : m_sessions)
    {
        if (session.get() != except)
        {
            session->send(msg);
        }
    }
}

void ServerListener::sendToAll(const std::vector<char>& data, void* except)
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);

    for (auto& session : m_sessions)
    {
        if (session.get() != except)
        {
            session->send(data);
        }
    }
}

void ServerListener::sendTo(const std::string& msg, void* client)
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);

    for (auto& session : m_sessions)
    {
        if (session.get() == client)
        {
            session->send(msg);
            break;
        }
    }
}

void ServerListener::sendTo(const std::vector<char>& data, void* client)
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);

    for (auto& session : m_sessions)
    {
        if (session.get() == client)
        {
            session->send(data);
            break;
        }
    }
}

bool ServerListener::isListening() const
{
    return m_acceptor.is_open();
}

size_t ServerListener::sessionCount() const
{
    return m_sessions.size();
}


void ServerListener::do_accept()
{
    // The new connection gets its own strand
    m_acceptor.async_accept(
        net::make_strand(m_ioc),
        beast::bind_front_handler(&ServerListener::on_accept,
                                  shared_from_this()));
}

void ServerListener::on_accept(beast::error_code ec,
                               tcp::socket socket)
{
    // This can happen during exit
    if (!m_acceptor.is_open())
    {
        return;
    }

    // This can happen during exit
    if (ec == boost::asio::error::operation_aborted)
    {
        return;
    }


    if (ec)
    {
        fail(ec, "accept");
    }
    else
    {
        // Create the session and run it
        auto session = std::make_shared<ServerSession>(std::move(socket), m_binary);
        session->setListener(m_listener);
        session->run([this](ServerSession* session)
        {
            // closed callback
            // remove session
            std::lock_guard<std::recursive_mutex> lock(m_mutex);

            auto it = m_sessions.begin();
            while(it != m_sessions.end())
            {
                if (it->get() == session)
                {
                    m_sessions.erase(it);

                    if (m_listener)
                    {
                        m_listener->clientDisconnected(session);
                    }

                    break;
                }

                it++;
            }

            if (!m_acceptor.is_open() &&
                m_sessions.empty())
            {
                // all clients closed - stop the world
                m_ioc.stop();
            }
        });

        // add session
        {
            std::lock_guard<std::recursive_mutex> lock(m_mutex);
            m_sessions.push_back(session);
        }
    }


    // accept next
    do_accept();
}

void ServerListener::fail(beast::error_code ec, char const* what)
{
    if (ec == boost::asio::error::operation_aborted)
    {
        return;
    }

    std::cerr << "Listener: " << what << ": " << ec.message() << "\n";
}

} // namespace scaryws
