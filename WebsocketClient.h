/* A websocket client using Boost.Beast
 *
 * (C) Copyright Ingo Randolf 2025.
 *
 * Distributed under the Boost Software License, Version 1.0. (See
 * accompanying file LICENSE or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 */

#ifndef SCARYWS_WEBSOCKET_CLIENT_H
#define SCARYWS_WEBSOCKET_CLIENT_H

#include <thread>

#include <boost/url.hpp>

#include "ClientSession.h"
#include "ClientSessionSSL.h"
#include "IClientSessionListener.h"

namespace scaryws
{

class WebsocketClient
    : public IClientSessionListener
{
public:
    WebsocketClient();
    ~WebsocketClient();

    // send binary data - default: true
    bool binary() const;
    void binary(bool newBinary);

    // verify peer - default: true
    void verifyPeer(bool verify);
    bool verifyPeer() const;

    std::string url() const;

    virtual void connect(const std::string& url);
    virtual void disconnect();
    virtual void send(const std::string& str);
    virtual void send(const std::vector<char>& data);
    virtual bool isConnected() const;
    virtual void reconnect();

public:
    // IClientSessionListener
    void connected() override;
    void error(int code, const std::string& message) override;
    void disconnected(uint16_t code) override;
    void received(const char* data, size_t size) override;
    void received(const std::string& msg) override;


private:
    void run(const boost::urls::url& url);
    void runSSL(const boost::urls::url& url);

private:
    boost::urls::url m_url;
    bool m_binary{true};
    bool m_verifyPeer{true};

    std::thread* m_thread{nullptr};
    mutable std::mutex m_mutex;

    std::shared_ptr<ClientSession> m_session;
    std::shared_ptr<ClientSessionSSL> m_sslSession;
};

} // namespace scaryws


#endif // SCARYWS_WEBSOCKET_CLIENT_H
