/* A websocket client using Boost.Beast
 *
 * (C) Copyright Ingo Randolf 2025.
 *
 * Distributed under the Boost Software License, Version 1.0. (See
 * accompanying file LICENSE or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 */

#include "WebsocketClient.h"

#include <cstdlib>
#include <memory>
#include <thread>
#include <iostream>


#include <boost/certify/extensions.hpp>
#include <boost/certify/https_verification.hpp>

namespace scaryws
{

WebsocketClient::WebsocketClient()
{
}

WebsocketClient::~WebsocketClient()
{
    disconnect();

    if (m_thread)
    {
        m_thread->join();
        delete m_thread;
        m_thread = nullptr;
    }
}

void WebsocketClient::connect(const std::string& url)
{
    auto url_view = boost::urls::parse_uri(url);
    if (!url_view.has_error())
    {
        m_url = url_view.value();
    }

    if (m_url.empty())
    {
        // cout << "no url\n";
        return;
    }

    if (m_thread)
    {
        disconnect();
        m_thread->join();
        delete m_thread;
        m_thread = nullptr;
    }

    // cout << "host: " << m_url.host() << endl;
    // cout << "port: " << m_url.port() << endl;
    // cout << "path: " << m_url.path() << endl;
    // cout << "query: " << m_url.query() << endl;

    if (m_url.scheme().find("wss", 0) == 0)
    {
        m_thread = new std::thread(&WebsocketClient::runSSL, this, m_url);
    }
    else
    {
        m_thread = new std::thread(&WebsocketClient::run, this, m_url);
    }
}

void WebsocketClient::reconnect()
{
    if (!m_url.empty())
    {
        connect(m_url.c_str());
    }
    else
    {
        std::cout << "no url\n";
    }
}

std::string WebsocketClient::url() const
{
    return m_url.c_str();
}

void WebsocketClient::disconnect()
{
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_session)
    {
        m_session->close();
    }
    else if (m_sslSession)
    {
        m_sslSession->close();
    }
    else
    {
        // cout << "close: no session" << endl;
    }

    m_session.reset();
    m_sslSession.reset();
}


void WebsocketClient::send(const std::string& str)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_session)
    {
        m_session->send(str);
    }
    else if (m_sslSession)
    {
        m_sslSession->send(str);
    }
    else
    {
        // cout << "send: no session" << endl;
    }
}

void WebsocketClient::send(const std::vector<char>& data)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_session)
    {
        m_session->send(data);
    }
    else if (m_sslSession)
    {
        m_sslSession->send(data);
    }
    else
    {
        // cout << "send: no session" << endl;
    }
}

bool WebsocketClient::isConnected() const
{
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_session)
    {
        return m_session->isConnected();
    }
    else if (m_sslSession)
    {
        return m_sslSession->isConnected();
    }

    return false;
}

bool WebsocketClient::binary() const
{
    return m_binary;
}

void WebsocketClient::binary(bool newBinary)
{
    m_binary = newBinary;
}

void WebsocketClient::verifyPeer(bool verify)
{
    m_verifyPeer = verify;
}

bool WebsocketClient::verifyPeer() const
{
    return m_verifyPeer;
}


// threaded functions

void WebsocketClient::connected()
{
    // client connected
}

void WebsocketClient::error(int code, const std::string& message)
{
    disconnect();
}

void WebsocketClient::disconnected(uint16_t code)
{
    // client disconnected
}

void WebsocketClient::received(const char* data, size_t size)
{
    // received binary data
}

void WebsocketClient::received(const std::string& msg)
{
    // received text data
}


void WebsocketClient::run(const boost::urls::url& url)
{
    net::io_context ioc;

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_session = std::make_shared<ClientSession>(ioc, m_binary);
        m_session->setListener(this);
        m_session->run(url);
    }

    try
    {
        ioc.run();
    }
    catch(std::exception& ex)
    {
        std::cerr << "execption running ws-client io:" << ex.what() << "\n";
    }

    // call closed
    disconnected(0);

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_session.reset();
    }

    // cout << "client session ended" << endl;
}

void WebsocketClient::runSSL(const boost::urls::url& url)
{
    net::io_context ioc;

    ssl::context ctx{ssl::context::tls_client};

    if (m_verifyPeer)
    {
        ctx.set_verify_mode(net::ssl::verify_peer |
                            net::ssl::verify_fail_if_no_peer_cert);

        ctx.set_default_verify_paths();

        boost::certify::enable_native_https_server_verification(ctx);
    }
    else
    {
        ctx.set_verify_mode(net::ssl::verify_none);
    }


    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_sslSession = std::make_shared<ClientSessionSSL>(ioc, ctx, m_binary);
        m_sslSession->setListener(this);
        m_sslSession->run(url);
    }

    try
    {
        ioc.run();
    }
    catch(std::exception& ex)
    {
        std::cerr << "execption running ws-client ssl-io:" << ex.what() << "\n";
    }

    // call closed
    disconnected(0);

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_sslSession.reset();
    }

    // cout << "client ssl session ended" << endl;
}

} // namespace scaryws
