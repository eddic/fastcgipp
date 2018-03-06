/*!
 * @file       sockets.cpp
 * @brief      Defines everything for interfaces with OS level sockets.
 * @author     Eddie Carle &lt;eddie@isatec.ca&gt;
 * @date       May 3, 2017
 * @copyright  Copyright &copy; 2017 Eddie Carle. This project is released under
 *             the GNU Lesser General Public License Version 3.
 *
 * It is this file, along with sockets.hpp, that must be modified to make
 * fastcgi++ work on Windows.
 */

/*******************************************************************************
* Copyright (C) 2017 Eddie Carle [eddie@isatec.ca]                             *
*                                                                              *
* This file is part of fastcgi++.                                              *
*                                                                              *
* fastcgi++ is free software: you can redistribute it and/or modify it under   *
* the terms of the GNU Lesser General Public License as  published by the Free *
* Software Foundation, either version 3 of the License, or (at your option)    *
* any later version.                                                           *
*                                                                              *
* fastcgi++ is distributed in the hope that it will be useful, but WITHOUT ANY *
* WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS    *
* FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for     *
* more details.                                                                *
*                                                                              *
* You should have received a copy of the GNU Lesser General Public License     *
* along with fastcgi++.  If not, see <http://www.gnu.org/licenses/>.           *
*******************************************************************************/

#include "fastcgi++/sockets.hpp"
#include "fastcgi++/log.hpp"

#ifdef FASTCGIPP_LINUX
#include <sys/epoll.h>
#elif defined FASTCGIPP_UNIX
#include <algorithm>
#endif

#include <sys/socket.h>
#include <sys/un.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <pwd.h>
#include <grp.h>
#include <cstring>

Fastcgipp::Socket::Socket(
        const socket_t& socket,
        SocketGroup& group,
        bool valid):
    m_data(new Data(socket, valid, group)),
    m_original(true)
{
    if(!group.pollAdd(socket))
    {
        ERROR_LOG("Unable to add socket " << socket << " to poll list: " \
                << std::strerror(errno))
        close();
    }
}

ssize_t Fastcgipp::Socket::read(char* buffer, size_t size) const
{
    if(!valid())
        return -1;

    const ssize_t count = ::read(m_data->m_socket, buffer, size);
    if(count<0)
    {
        WARNING_LOG("Socket read() error on fd " \
                << m_data->m_socket << ": " << std::strerror(errno))
        if(errno == EAGAIN)
            return 0;
        close();
        return -1;
    }
    if(count == 0 && m_data->m_closing)
    {
#if FASTCGIPP_LOG_LEVEL > 3
        ++m_data->m_group.m_connectionRDHupCount;
#endif
        close();
        return -1;
    }

#if FASTCGIPP_LOG_LEVEL > 3
    m_data->m_group.m_bytesReceived += count;
#endif

    return count;
}

ssize_t Fastcgipp::Socket::write(const char* buffer, size_t size) const
{
    if(!valid() || m_data->m_closing)
        return -1;

    const ssize_t count = ::send(m_data->m_socket, buffer, size, MSG_NOSIGNAL);
    if(count<0)
    {
        WARNING_LOG("Socket write() error on fd " \
                << m_data->m_socket << ": " << strerror(errno))
        if(errno == EAGAIN)
            return 0;
        close();
        return -1;
    }

#if FASTCGIPP_LOG_LEVEL > 3
    m_data->m_group.m_bytesSent += count;
#endif

    return count;
}

void Fastcgipp::Socket::close() const
{
    if(valid())
    {
        ::shutdown(m_data->m_socket, SHUT_RDWR);
        ::close(m_data->m_socket);
        m_data->m_valid = false;
        m_data->m_group.pollDel(m_data->m_socket);
        m_data->m_group.m_sockets.erase(m_data->m_socket);
#if FASTCGIPP_LOG_LEVEL > 3
        if(!m_data->m_closing)
            ++m_data->m_group.m_connectionKillCount;
#endif
    }
}

Fastcgipp::Socket::~Socket()
{
    if(m_original && valid())
    {
        ::shutdown(m_data->m_socket, SHUT_RDWR);
        ::close(m_data->m_socket);
        m_data->m_valid = false;
        m_data->m_group.pollDel(m_data->m_socket);
    }
}

Fastcgipp::SocketGroup::SocketGroup():
#ifdef FASTCGIPP_LINUX
    m_poll(epoll_create1(0)),
#endif
    m_waking(false),
    m_accept(true),
    m_refreshListeners(false)
#if FASTCGIPP_LOG_LEVEL > 3
    ,m_incomingConnectionCount(0),
    m_outgoingConnectionCount(0),
    m_connectionKillCount(0),
    m_connectionRDHupCount(0),
    m_bytesSent(0),
    m_bytesReceived(0)
#endif
{
    // Add our wakeup socket into the poll list
    socketpair(AF_UNIX, SOCK_STREAM, 0, m_wakeSockets);
    pollAdd(m_wakeSockets[1]);
    DIAG_LOG("SocketGroup::SocketGroup(): Initialized ")
}

Fastcgipp::SocketGroup::~SocketGroup()
{
#ifdef FASTCGIPP_LINUX
    close(m_poll);
#endif
    close(m_wakeSockets[0]);
    close(m_wakeSockets[1]);
    for(const auto& listener: m_listeners)
    {
        ::shutdown(listener, SHUT_RDWR);
        ::close(listener);
    }
    for(const auto& filename: m_filenames)
        std::remove(filename.c_str());

    DIAG_LOG("SocketGroup::~SocketGroup(): Incoming sockets ======== " \
            << m_incomingConnectionCount)
    DIAG_LOG("SocketGroup::~SocketGroup(): Outgoing sockets ======== " \
            << m_outgoingConnectionCount)
    DIAG_LOG("SocketGroup::~SocketGroup(): Locally closed sockets == " \
            << m_connectionKillCount)
    DIAG_LOG("SocketGroup::~SocketGroup(): Remotely closed sockets = " \
            << m_connectionRDHupCount)
    DIAG_LOG("SocketGroup::~SocketGroup(): Remaining sockets ======= " \
            << m_sockets.size())
    DIAG_LOG("SocketGroup::~SocketGroup(): Bytes sent ===== " << m_bytesSent)
    DIAG_LOG("SocketGroup::~SocketGroup(): Bytes received = " \
            << m_bytesReceived)
}

bool Fastcgipp::SocketGroup::listen()
{
    const int listen=0;

    fcntl(listen, F_SETFL, fcntl(listen, F_GETFL)|O_NONBLOCK);

    if(m_listeners.find(listen) == m_listeners.end())
    {
        if(::listen(listen, 100) < 0)
        {
            ERROR_LOG("Unable to listen on default FastCGI socket: "\
                    << std::strerror(errno));
            return false;
        }
        m_listeners.insert(listen);
        m_refreshListeners = true;
        return true;
    }
    else
    {
        ERROR_LOG("Socket " << listen << " already being listened to")
        return false;
    }
}

bool Fastcgipp::SocketGroup::listen(
        const char* name,
        uint32_t permissions,
        const char* owner,
        const char* group)
{
    if(std::remove(name) != 0 && errno != ENOENT)
    {
        ERROR_LOG("Unable to delete file \"" << name << "\": " \
                << std::strerror(errno))
        return false;
    }

    const auto fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if(fd == -1)
    {
        ERROR_LOG("Unable to create unix socket: " << std::strerror(errno))
        return false;

    }

    struct sockaddr_un address;
    std::memset(&address, 0, sizeof(address));
    address.sun_family = AF_UNIX;
    std::strncpy(address.sun_path, name, sizeof(address.sun_path) - 1);

    if(bind(
                fd,
                reinterpret_cast<struct sockaddr*>(&address),
                sizeof(address)) < 0)
    {
        ERROR_LOG("Unable to bind to unix socket \"" << name << "\": " \
                << std::strerror(errno));
        close(fd);
        std::remove(name);
        return false;
    }

    // Set the user and group of the socket
    if(owner!=nullptr && group!=nullptr)
    {
        struct passwd* passwd = getpwnam(owner);
        struct group* grp = getgrnam(group);
        if(chown(name, passwd->pw_uid, grp->gr_gid)==-1)
        {
            ERROR_LOG("Unable to chown " << owner << ":" << group \
                    << " on the unix socket \"" << name << "\": " \
                    << std::strerror(errno));
            close(fd);
            return false;
        }
    }

    // Set the user and group of the socket
    if(permissions != 0xffffffffUL)
    {
        if(chmod(name, permissions)<0)
        {
            ERROR_LOG("Unable to set permissions 0" << std::oct << permissions \
                    << std::dec << " on \"" << name << "\": " \
                    << std::strerror(errno));
            close(fd);
            return false;
        }
    }

    if(::listen(fd, 100) < 0)
    {
        ERROR_LOG("Unable to listen on unix socket :\"" << name << "\": "\
                << std::strerror(errno));
        close(fd);
        return false;
    }

    m_filenames.emplace_back(name);
    m_listeners.insert(fd);
    m_refreshListeners = true;
    return true;
}

bool Fastcgipp::SocketGroup::listen(
        const char* interface,
        const char* service)
{
    if(service == nullptr)
    {
        ERROR_LOG("Cannot call listen(interface, service) with service=nullptr.")
        return false;
    }

    addrinfo hints;
    std::memset(&hints, 0, sizeof(addrinfo));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_canonname = nullptr;
    hints.ai_addr = nullptr;
    hints.ai_next = nullptr;

    addrinfo* result;

    if(getaddrinfo(interface, service, &hints, &result))
    {
        ERROR_LOG("Unable to use getaddrinfo() on " \
                << (interface==nullptr?"0.0.0.0":interface) << ":" << service << ". " \
                << std::strerror(errno))
        return false;
    }

    int fd=-1;
    for(auto i=result; i!=nullptr; i=result->ai_next)
    {
        fd = socket(i->ai_family, i->ai_socktype, i->ai_protocol);
        if(fd == -1)
            continue;
        if(
                bind(fd, i->ai_addr, i->ai_addrlen) == 0
                && ::listen(fd, 100) == 0)
            break;
        close(fd);
        fd = -1;
    }
    freeaddrinfo(result);

    if(fd==-1)
    {
        ERROR_LOG("Unable to bind/listen on " \
                << (interface==nullptr?"0.0.0.0":interface) << ":" << service)
        return false;
    }

    m_listeners.insert(fd);
    m_refreshListeners = true;
    return true;
}

Fastcgipp::Socket Fastcgipp::SocketGroup::connect(const char* name)
{
    const auto fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if(fd == -1)
    {
        ERROR_LOG("Unable to create unix socket: " << std::strerror(errno))
        return Socket();
    }

    sockaddr_un address;
    std::memset(&address, 0, sizeof(address));
    address.sun_family = AF_UNIX;
    std::strncpy(address.sun_path, name, sizeof(address.sun_path) - 1);

    if(::connect(
                fd,
                reinterpret_cast<struct sockaddr*>(&address),
                sizeof(address))==-1)
    {
        ERROR_LOG("Unable to connect to unix socket \"" << name << "\": " \
                << std::strerror(errno));
        close(fd);
        return Socket();
    }

#if FASTCGIPP_LOG_LEVEL > 3
    ++m_outgoingConnectionCount;
#endif

    return m_sockets.emplace(
            fd,
            Socket(fd, *this)).first->second;
}

Fastcgipp::Socket Fastcgipp::SocketGroup::connect(
        const char* host,
        const char* service)
{
    if(service == nullptr)
    {
        ERROR_LOG("Cannot call connect(host, service) with service=nullptr.")
        return Socket();
    }

    if(host == nullptr)
    {
        ERROR_LOG("Cannot call host(host, service) with host=nullptr.")
        return Socket();
    }

    addrinfo hints;
    std::memset(&hints, 0, sizeof(addrinfo));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = 0;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_canonname = nullptr;
    hints.ai_addr = nullptr;
    hints.ai_next = nullptr;

    addrinfo* result;

    if(getaddrinfo(host, service, &hints, &result))
    {
        ERROR_LOG("Unable to use getaddrinfo() on " << host << ":" << service  \
                << ". " << std::strerror(errno))
        return Socket();
    }

    int fd=-1;
    for(auto i=result; i!=nullptr; i=result->ai_next)
    {
        fd = socket(i->ai_family, i->ai_socktype, i->ai_protocol);
        if(fd == -1)
            continue;
        if(::connect(fd, i->ai_addr, i->ai_addrlen) != -1)
            break;
        close(fd);
        fd = -1;
    }
    freeaddrinfo(result);

    if(fd==-1)
    {
        ERROR_LOG("Unable to connect to " << host << ":" << service)
        return Socket();
    }

#if FASTCGIPP_LOG_LEVEL > 3
    ++m_outgoingConnectionCount;
#endif

    return m_sockets.emplace(
            fd,
            Socket(fd, *this)).first->second;
}

Fastcgipp::Socket Fastcgipp::SocketGroup::poll(bool block)
{
    int pollResult;

#ifdef FASTCGIPP_LINUX
    epoll_event epollEvent;
    const auto& pollIn = EPOLLIN;
    const auto& pollErr = EPOLLERR;
    const auto& pollHup = EPOLLHUP;
    const auto& pollRdHup = EPOLLRDHUP;
#elif defined FASTCGIPP_UNIX
    const auto& pollIn = POLLIN;
    const auto& pollErr = POLLERR;
    const auto& pollHup = POLLHUP;
    const auto& pollRdHup = POLLRDHUP;
#endif

    while(m_listeners.size()+m_sockets.size() > 0)
    {
        if(m_refreshListeners)
        {
            for(auto& listener: m_listeners)
            {
                pollDel(listener);
                if(m_accept && !pollAdd(listener))
                    FAIL_LOG("Unable to add listen socket " << listener \
                            << " to the poll list: " << std::strerror(errno))
            }
            m_refreshListeners=false;
        }
#ifdef FASTCGIPP_LINUX
        pollResult = epoll_wait(
                m_poll,
                &epollEvent,
                1,
                block?-1:0);
#elif defined FASTCGIPP_UNIX
        pollResult = ::poll(
                m_poll.data(),
                m_poll.size(),
                block?-1:0);
#endif

        if(pollResult<0)
        {
            if(errno == EINTR)
                continue;
            FAIL_LOG("Error on poll: " << std::strerror(errno))
        }
        else if(pollResult>0)
        {
#ifdef FASTCGIPP_LINUX
            const auto& socketId = epollEvent.data.fd;
            const auto& events = epollEvent.events;
#elif defined FASTCGIPP_UNIX
            const auto fd = std::find_if(
                    m_poll.begin(),
                    m_poll.end(),
                    [] (const pollfd& x)
                    {
                        return x.revents != 0;
                    });
            if(fd == m_poll.end())
                FAIL_LOG("poll() gave a result >0 but no revents are non-zero")
            const auto& socketId = fd->fd;
            const auto& events = fd->revents;
#endif

            if(m_listeners.find(socketId) != m_listeners.end())
            {
                if(events == pollIn)
                {
                    createSocket(socketId);
                    continue;
                }
                else if(events & pollErr)
                    FAIL_LOG("Error in listen socket.")
                else if(events & (pollHup | pollRdHup))
                    FAIL_LOG("The listen socket hung up.")
                else
                    FAIL_LOG("Got a weird event 0x" << std::hex << events\
                            << " on listen poll." )
            }
            else if(socketId == m_wakeSockets[1])
            {
                if(events == pollIn)
                {
                    std::lock_guard<std::mutex> lock(m_wakingMutex);
                    char x[256];
                    if(read(m_wakeSockets[1], x, 256)<1)
                        FAIL_LOG("Unable to read out of wakeup socket: " << \
                                std::strerror(errno))
                    m_waking=false;
                    block=false;
                    continue;
                }
                else if(events & (pollHup | pollRdHup))
                    FAIL_LOG("The wakeup socket hung up.")
                else if(events & pollErr)
                    FAIL_LOG("Error in the wakeup socket.")
            }
            else
            {
                const auto socket = m_sockets.find(socketId);
                if(socket == m_sockets.end())
                {
                    ERROR_LOG("Poll gave fd " << socketId \
                            << " which isn't in m_sockets.")
                    pollDel(socketId);
                    close(socketId);
                    continue;
                }

                if(events & pollRdHup)
                    socket->second.m_data->m_closing=true;
                else if(events & pollHup)
                {
                    WARNING_LOG("Socket " << socketId << " hung up")
                    socket->second.m_data->m_closing=true;
                }
                else if(events & pollErr)
                {
                    ERROR_LOG("Error in socket " << socketId)
                    socket->second.m_data->m_closing=true;
                }
                else if((events & pollIn) == 0)
                    FAIL_LOG("Got a weird event 0x" << std::hex << events\
                            << " on socket poll." )
                return socket->second;
            }
        }
        break;
    }
    return Socket();
}

void Fastcgipp::SocketGroup::wake()
{
    std::lock_guard<std::mutex> lock(m_wakingMutex);
    if(!m_waking)
    {
        m_waking=true;
        char x=0;
        if(write(m_wakeSockets[0], &x, 1) != 1)
            FAIL_LOG("Unable to write to wakeup socket: " \
                    << std::strerror(errno))
    }
}

void Fastcgipp::SocketGroup::createSocket(const socket_t listener)
{
    sockaddr_un addr;
    socklen_t addrlen=sizeof(sockaddr_un);
    const socket_t socket=::accept(
            listener,
            reinterpret_cast<sockaddr*>(&addr),
            &addrlen);
    if(socket<0)
        FAIL_LOG("Unable to accept() with fd " \
                << listener << ": " \
                << std::strerror(errno))
    if(fcntl(
            socket,
            F_SETFL,
            fcntl(socket, F_GETFL)|O_NONBLOCK)
            < 0)
    {
        ERROR_LOG("Unable to set NONBLOCK on fd " << socket \
                << " with fcntl(): " << std::strerror(errno))
        close(socket);
        return;
    }

    if(m_accept)
    {
        m_sockets.emplace(
                socket,
                Socket(socket, *this));
#if FASTCGIPP_LOG_LEVEL > 3
        ++m_incomingConnectionCount;
#endif
    }
    else
        close(socket);
}

Fastcgipp::Socket::Socket():
    m_data(nullptr),
    m_original(false)
{}

bool Fastcgipp::SocketGroup::pollAdd(const socket_t socket)
{
#ifdef FASTCGIPP_LINUX
    epoll_event event;
    event.data.fd = socket;
    event.events = EPOLLIN | EPOLLERR | EPOLLHUP | EPOLLRDHUP;
    return epoll_ctl(m_poll, EPOLL_CTL_ADD, socket, &event) != -1;
#elif defined FASTCGIPP_UNIX
    const auto fd = std::find_if(
            m_poll.begin(),
            m_poll.end(),
            [&socket] (const pollfd& x)
            {
                return x.fd == socket;
            });
    if(fd != m_poll.end())
        return false;

    m_poll.emplace_back();
    m_poll.back().fd = socket;
    m_poll.back().events = POLLIN | POLLRDHUP | POLLERR | POLLHUP;
    return true;
#endif
}

bool Fastcgipp::SocketGroup::pollDel(const socket_t socket)
{
#ifdef FASTCGIPP_LINUX
    return epoll_ctl(m_poll, EPOLL_CTL_DEL, socket, nullptr) != -1;
#elif defined FASTCGIPP_UNIX
    const auto fd = std::find_if(
            m_poll.begin(),
            m_poll.end(),
            [&socket] (const pollfd& x)
            {
                return x.fd == socket;
            });
    if(fd == m_poll.end())
        return false;

    m_poll.erase(fd);
    return true;
#endif
}

void Fastcgipp::SocketGroup::accept(bool status)
{
    if(status != m_accept)
    {
        m_refreshListeners = true;
        m_accept = status;
        wake();
    }
}
