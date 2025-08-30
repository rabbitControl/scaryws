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
    : ioc(ioc)
    , acceptor(ioc)
    , m_binary(binary)
{
    beast::error_code ec;

    acceptor.open(endpoint.protocol(), ec);
    if(ec)
    {
        fail(ec, "open");
        return;
    }

    acceptor.set_option(net::socket_base::reuse_address(true), ec);
    if(ec)
    {
        fail(ec, "set_option");
        return;
    }

    acceptor.bind(endpoint, ec);
    if(ec)
    {
        fail(ec, "bind");
        return;
    }    

    acceptor.listen(net::socket_base::max_listen_connections, ec);
    if(ec)
    {
        fail(ec, "listen");
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

    acceptor.cancel();
    acceptor.close();
    ioc.stop();

    m_sessions.clear();
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
    return acceptor.is_open();
}

size_t ServerListener::sessionCount() const
{
    return m_sessions.size();
}


void ServerListener::do_accept()
{
    // The new connection gets its own strand
    acceptor.async_accept(
        net::make_strand(ioc),
        beast::bind_front_handler(&ServerListener::on_accept,
                                  shared_from_this()));
}

void ServerListener::on_accept(beast::error_code ec,
                               tcp::socket socket)
{
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
                    break;
                }

                it++;
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
    std::cerr << "Listener: " << what << ": " << ec.message() << "\n";
}

} // namespace scaryws
