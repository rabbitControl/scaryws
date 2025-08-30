/* A websocket server using Boost.Beast
 *
 * (C) Copyright Ingo Randolf 2025.
 *
 * Distributed under the Boost Software License, Version 1.0. (See
 * accompanying file LICENSE or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 */

#include "WebsocketServer.h"

#include <iostream>

#include <boost/beast/core.hpp>
#include <boost/asio/ip/tcp.hpp>

#include "ServerListener.h"

namespace beast = boost::beast;
namespace net = boost::asio;
using tcp = boost::asio::ip::tcp;


namespace scaryws
{

WebsocketServer::WebsocketServer()
    : m_address(net::ip::address_v4::any())
{}

WebsocketServer::~WebsocketServer()
{
    close();
}

uint16_t WebsocketServer::port() const
{
    return m_port;
}

std::string WebsocketServer::address() const
{
    return m_address.to_string();
}

void WebsocketServer::binary(bool binary)
{
    m_binary = binary;
}

bool WebsocketServer::binary() const
{
    return m_binary;
}

void WebsocketServer::listen(uint16_t port, const std::string& address)
{
    close();

    if (address.empty())
    {
        m_address = net::ip::make_address("0.0.0.0");
    }
    else
    {
        try
        {
            m_address = net::ip::make_address(address);
        }
        catch (boost::system::error_code ec)
        {
            std::cerr << "error parsing address: " << ec.what() << std::endl;
        }
    }

    m_port = port;

    if (m_port > 0)
    {
        m_thread = new std::thread(&WebsocketServer::run, this);
    }
}

void WebsocketServer::close()
{
    if (m_listener)
    {
        m_listener->cancel();
    }

    if (m_thread)
    {
        m_thread->join();
        delete m_thread;
        m_thread = nullptr;
    }

    m_port = 0;
}

bool WebsocketServer::isListening() const
{
    return m_listener && m_listener->isListening();
}

size_t WebsocketServer::clientCount() const
{
    if (m_listener)
    {
        return m_listener->sessionCount();
    }

    return 0;
}

void WebsocketServer::sendToAll(const std::string& str, void* except)
{
    if (m_listener)
    {
        m_listener->sendToAll(str, except);
    }
}

void WebsocketServer::sendToAll(const std::vector<char>& data, void* except)
{
    if (m_listener)
    {
        m_listener->sendToAll(data, except);
    }
}

void WebsocketServer::sendTo(const std::string& str, void* client)
{
    if (m_listener)
    {
        m_listener->sendTo(str, client);
    }
}

void WebsocketServer::sendTo(const std::vector<char>& data, void* client)
{
    if (m_listener)
    {
        m_listener->sendTo(data, client);
    }
}


// threaded functions

void WebsocketServer::listening()
{

}
void WebsocketServer::closed()
{

}

void WebsocketServer::clientConnected(void* client)
{
}

void WebsocketServer::clientDisconnected(void* client)
{
}

void WebsocketServer::received(const char* data, size_t size, void* client)
{
    // received binary message
}

void WebsocketServer::received(const std::string& msg, void* client)
{
    // received text message
}


void WebsocketServer::run()
{
    // int const threads = 1;

    net::io_context ioc; // threads

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_listener = std::make_shared<ServerListener>(ioc,
                                                tcp::endpoint{m_address, m_port},
                                                m_binary);
        m_listener->setListener(this);
        m_listener->run();
    }

    //
    listening();

    try
    {
        ioc.run();
    }
    catch(std::exception& ex)
    {
        std::cout << "execption running ws-server io:" << ex.what() << "\n";
    }

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_listener.reset();
    }

    closed();
}

} // namespace scaryws

