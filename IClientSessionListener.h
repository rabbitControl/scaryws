/* A websocket client using Boost.Beast
 *
 * (C) Copyright Ingo Randolf 2025.
 *
 * Distributed under the Boost Software License, Version 1.0. (See
 * accompanying file LICENSE or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 */

#ifndef SCARYWS_I_CLIENT_SESSION_LISTENER_H
#define SCARYWS_I_CLIENT_SESSION_LISTENER_H

#include <cstdint>
#include <string>

namespace scaryws
{

class IClientSessionListener
{
public:
    virtual void connected() = 0;
    virtual void error(int code, const std::string& message) = 0;
    virtual void disconnected(uint16_t code) = 0;
    virtual void received(const char* data, size_t size) = 0;
    virtual void received(const std::string& msg) = 0;
};

} // namespace scaryws

#endif // SCARYWS_I_CLIENT_SESSION_LISTENER_H
