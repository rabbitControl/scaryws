/* A websocket client using Boost.Beast
 *
 * (C) Copyright Ingo Randolf 2025.
 *
 * Distributed under the Boost Software License, Version 1.0. (See
 * accompanying file LICENSE or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 */

#include "ClientSessionBase.h"

#ifdef WSLIB_CLIENT_SESSION_VERBOSE
#include <iostream>
#endif

namespace scaryws
{

ClientSessionBase::ClientSessionBase(net::io_context& ioc)
    : m_resolver(net::make_strand(ioc))
{
}

void ClientSessionBase::setListener(IClientSessionListener* listener)
{
    m_listener = listener;
}

void ClientSessionBase::on_write(beast::error_code ec, std::size_t bytes_transferred)
{
    boost::ignore_unused(bytes_transferred);

    if(ec)
    {
        fail(ec, "write");
    }

    std::lock_guard<std::mutex> lock(m_mutex);

    // Remove the string from the queue
    m_queue.erase(m_queue.begin());

    sendNext();
}

void ClientSessionBase::receivedData(beast::error_code ec,
                                   std::size_t bytes_transferred,
                                   bool binary)
{
    if (m_listener)
    {
        if (binary)
        {
            m_listener->received(static_cast<const char*>(m_buffer.data().data()), m_buffer.data().size());
        }
        else
        {
            m_listener->received(std::string(static_cast<const char*>(m_buffer.data().data()), m_buffer.data().size()));
        }
    }
    else
    {
        // no listener
#ifdef WSLIB_CLIENT_SESSION_VERBOSE
        cout << "no listener, but received received: " << bytes_transferred << " bytes" << endl;
#endif
    }

    // clear buffer
    m_buffer.consume(m_buffer.size());
}

void ClientSessionBase::fail(beast::error_code ec, const std::string& what)
{
#ifdef WSLIB_CLIENT_SESSION_VERBOSE
    cerr << "fail: " << what << ": " << ec.message() << endl;
#endif

    if (m_listener)
    {
        m_listener->error(ec.value(), ec.message());
    }
}

} // namespace scaryws
