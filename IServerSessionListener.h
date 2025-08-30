/* A websocket server using Boost.Beast
 *
 * (C) Copyright Ingo Randolf 2025.
 *
 * Distributed under the Boost Software License, Version 1.0. (See
 * accompanying file LICENSE or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 */

#ifndef SCARYWS_I_SERVER_SESSION_LISTENER_H
#define SCARYWS_I_SERVER_SESSION_LISTENER_H

#include <string>

namespace scaryws
{

class IServerSessionListener
{
public:
    virtual void listening() = 0;
    virtual void closed() = 0;
    virtual void clientConnected(void* client) = 0;
    virtual void clientDisconnected(void* client) = 0;

    // received binary data
    virtual void received(const char* data, size_t size, void* client) = 0;

    // received text data
    virtual void received(const std::string& msg, void* client) = 0;
};

} // namespace scaryws

#endif // SCARYWS_I_SERVER_SESSION_LISTENER_H
