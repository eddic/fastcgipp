/*!
 * @file       sockets.hpp
 * @brief      Declares everything for interfaces with OS level sockets.
 * @author     Eddie Carle &lt;eddie@isatec.ca&gt;
 * @date       April 27, 2017
 * @copyright  Copyright &copy; 2017 Eddie Carle. This project is released under
 *             the GNU Lesser General Public License Version 3.
 *
 * It is this file, along with sockets.cpp, that must be modified to make
 * fastcgi++ work on Windows. Although not completely sure, I believe all that
 * needs to be modified in this file is Fastcgipp::socket_t and
 * Fastcgipp::poll_t.
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

#ifndef FASTCGIPP_SOCKETS_HPP
#define FASTCGIPP_SOCKETS_HPP

#include <memory>
#include <map>
#include <mutex>
#include <set>
#include <atomic>
#include <deque>
#include <string>

#include "fastcgi++/config.hpp"

#ifdef FASTCGIPP_UNIX
#include <vector>
#include <poll.h>
#endif

//! Topmost namespace for the fastcgi++ library
namespace Fastcgipp
{
    //! Our socket identifier type in GNU/Linux is simply an int
    typedef int socket_t;

#ifdef FASTCGIPP_LINUX
    //! Our polling type using the Linux kernel is simply an int
    typedef const int poll_t;
#elif defined FASTCGIPP_UNIX
    //! Our polling type using generic Unix is an array of pollfds
    typedef std::vector<pollfd> poll_t;
#endif

    class SocketGroup;

    //! Class for representing an OS level I/O socket.
    /*!
     * It works together with the SocketGroup class to establish all the
     * interfacing between the OS and the Transceiver class to communicate with
     * the outside world. The objects of this class represent individual
     * connections to the FastCGI server. They are consolidated and managed
     * within the SocketGroup class.
     *
     * <em>No non-const member functions are thread safe. This means you can
     * only use valid() and the comparison operators across multiple threads.
     * </em>
     *
     * @date    April 24, 2016
     * @author  Eddie Carle &lt;eddie@isatec.ca&gt;
     */
    class Socket
    {
    private:
        //! Our respective SocketGroup needs private access.
        friend class SocketGroup;

        //! Data structure to hold the shared socket data.
        struct Data
        {
            //! OS level socket identifier.
            const socket_t m_socket;

            //! Indicates whether or not the socket is dead/invalid.
            bool m_valid;

            //! Indicates whether or not the connection is closing
            /*!
             * If this is set to true that once we read zero bytes out of the
             * socket it has become invalid.
             */
            bool m_closing;

            //! SocketGroup object this socket is tied to.
            SocketGroup& m_group;

            //! Sole constructor
            /*!
             * @param [inout] socket The OS level socket identifier to associate
             *                       with this.
             * @param [inout] group The SocketGroup object that created and is
             *                      consolidating this socket and it's peers.
             * @param [in] valid Set to false if this is a dead socket.
             *                   Sometimes it may be needed to create a socket
             *                   that is invalid off the start.
             */
            Data(
                    const socket_t& socket,
                    bool valid,
                    SocketGroup& group):
                m_socket(socket),
                m_valid(valid),
                m_closing(false),
                m_group(group)
            {}

            Data() =delete;
            Data(const Data&) =delete;
        };

        //! Shared pointer to hold the socket data.
        std::shared_ptr<Data> m_data;

        //! This is only true for a non-copy constructed object.
        bool m_original;

        //! Sole non-copy/move constructor
        /*!
         * This constructor is only accessible to the SocketGroup class to create
         * new "original" sockets as they are accepted. Only sockets created
         * with this constructor will have m_original set to true.
         *
         * @param [inout] socket The OS level socket identifier to associate
         *                       with this.
         * @param [inout] group The SocketGroup object that created and is
         *                      consolidating this socket and it's peers.
         * @param [in] valid Set to false if this is a dead socket. Sometimes it
         *                   may be needed to create a socket that is invalid
         *                   off the start.
         */
        Socket(
                const socket_t& socket,
                SocketGroup& group,
                bool valid=true);

    public:
        //! Try and read a chunk of data out of the socket.
        /*!
         * This function will attempt to read the requested amount of data out
         * of the socket into the buffer. The return value indicates both how
         * many bytes were read and socket validity.
         *
         * If an error occurs during the read operation the socket is
         * closed/destroyed, marked invalid and -1 is returned. If the other end
         * has hung up, you can continue reading until the internal buffer is
         * empty.  At that point -1 is returned and the socket is
         * closed/destroyed and marked invalid.
         *
         * @param [out] buffer Pointer to memory location to which data should
         *                     be read into.
         * @param [in] size Maximum amount of data to read into the buffer.
         *                  Obviously this should be less than or equal to the
         *                  actual amount of memory allocated in the buffer.
         * @return Actual number of bytes read into the buffer. If -1 is
         *         returned the socket is no longer valid and all data has been
         *         received.
         */
        ssize_t read(char* buffer, size_t size) const;

        //! Try and write a chunk of data into the socket.
        /*!
         * This function will attempt to write the requested amount of data into
         * the socket from the buffer. The return value indicates either how
         * many bytes were actually written or socket validity.
         *
         * If an error occurs during the write operation the socket is
         * closed/destroyed, marked invalid and -1 is returned. If the other end
         * has hung up, -1 will be returned but this does \a not mean the socket
         * has been closed/destroyed yet. There may yet be data waiting to be
         * read(). The socket will not be automatically shut down until said
         * data is read.
         *
         * @param [out] buffer Pointer to memory location to which data should
         *                     be written from.
         * @param [in] size Maximum amount of data to write from the buffer.
         * @return Actual number of bytes written from the buffer. A -1 means
         *         you can't actually write data to the socket anymore.
         */
        ssize_t write(const char* buffer, size_t size) const;

        //! We need this to allow the socket objects to be in sorted containers.
        bool operator<(const Socket& x) const
        {
            return m_data < x.m_data;
        }

        //! We need this to allow the socket objects to be in sorted containers.
        bool operator==(const Socket& x) const
        {
            return m_data == x.m_data;
        }

        //! Copy constructor
        /*!
         * Any sockets built using the copy constructor will not be marked
         * original.
         */
        Socket(const Socket& x):
            m_data(x.m_data),
            m_original(false)
        {}

        //! Assignment
        /*!
         * Any sockets assigned with this will not be marked original.
         */
        Socket& operator=(const Socket& x)
        {
            m_data = x.m_data;
            m_original = false;
            return *this;
        }

        //! Move constructor
        /*!
         * This constructor serves the purposes of moving "original" sockets
         * into containers. The source socket has it's originality stripped and
         * moved to the destination.
         */
        Socket(Socket&& x):
            m_data(x.m_data),
            m_original(x.m_original)
        {
            x.m_original=false;
        }

        //! Calls close() on the socket if we are destructing the original
        ~Socket();

        //! Returns true if this socket is still open and capable of read/write.
        bool valid() const
        {
            return m_data && m_data->m_valid;
        }

        //! Call this to close the socket
        /*!
         * If the socket is valid, this will do the following:
         *  - Close/hangup the OS level socket.
         *  - Remove the socket from the associated SocketGroup polling set.
         *  - Fully disassociate the socket with the SocketGroup.
         *  - Mark the socket as invalid.
         *
         * If the socket is already invalid, calling this does nothing.
         */
        void close() const;

        //! Creates an invalid socket with no original.
        Socket();
    };

    //! Class for representing an OS level socket that listens for connections.
    /*!
     * It works together with the Socket class to establish all the interfacing
     * between the OS and the Transceiver class to communicate with the outside
     * world. The object of this class represents the socket that listens for
     * incoming connections to the FastCGI server. This class will create and
     * manage an array of Socket objects as new connections are initiated.
     *
     * <em>The only part of this class that is safe to call from multiple
     * threads is the wake() function.</em>
     *
     * @date    April 27, 2017
     * @author  Eddie Carle &lt;eddie@isatec.ca&gt;
     */
    class SocketGroup
    {
    public:
        SocketGroup();

        ~SocketGroup();

        //! Listen to the default Fastcgi socket
        /*!
         * Calling this simply adds the default socket used on FastCGI
         * applications that are initialized from HTTP servers.
         *
         * @return True on success. False on failure.
         */
        bool listen();

        //! Listen to a named socket
        /*!
         * Listen on a named socket. In the Unix world this would be a path. In
         * the Windows world I have no idea what this would be.
         *
         * @param [in] name Name of socket (path in Unix world).
         * @param [in] permissions Permissions of socket. If you do not wish to
         *                         set the permissions, leave it as it's default
         *                         value of 0xffffffffUL.
         * @param [in] owner Owner (username) of socket. Leave as nullptr if you
         *                   do not wish to set it.
         * @param [in] group Group (group name) of socket. Leave as nullptr if
         *                   you do not wish to set it.
         * @return True on success. False on failure.
         */
        bool listen(
                const char* name,
                uint32_t permissions = 0xffffffffUL,
                const char* owner = nullptr,
                const char* group = nullptr);

        //! Listen to a TCP port
        /*!
         * Listen on a specific interface and TCP port.
         *
         * @param [in] interface Interface to listen on. This could be an IP
         *                       address or a hostname. If you don't want to
         *                       specify the interface, pass nullptr.
         * @param [in] service Port or service to listen on. This could be a
         *                     service name, or a string representation of a
         *                     port number.
         * @return True on success. False on failure.
         */
        bool listen(
                const char* interface,
                const char* service);

        //! Connect to a named socket
        /*!
         * Connect to a named socket. In the Unix world this would be a path.
         * In the Windows world I have no idea what this would be.
         *
         * @param [in] name Name of socket (path in Unix world).
         * @return Socket associated with the new connection. If the connection
         *         failed, you'll get an invalid socket (it will evaluate to
         *         false).
         */
        Socket connect(const char* name);

        //! Connect to a host on a TCP port/serivce
        /*!
         * @param [in] host Host to connect to. This could be an IP address
         *                       address or a hostname.
         * @param [in] service Port or service to connect to. This could be a
         *                     service name, or a string representation of a
         *                     port number.
         * @return Socket associated with the new connection. If the connection
         *         failed, you'll get an invalid socket (it will evaluate to
         *         false).
         */
        Socket connect(
                const char* host,
                const char* service);

        //! Poll socket set for new incoming connections and data.
        /*!
         * Calling this will initiate a poll for both new connections and new
         * incoming data within the set of sockets that this object has created.
         * It will also keep an eye out for dead sockets and tear them down
         * accordingly.
         *
         * Its behaviour is as follows:
         *  - If a new connection arrives, it will establish the new connection.
         *  - If a socket is dead, it destroys and marks the socket as invalid.
         *    It will not return anything regarding the dead socket.
         *  - If new data has arrived in a currently active connection it will
         *    return the socket.
         *  - If the call has been set to non-blocking and no new data awaits, a
         *    generic invalid socket is returned.
         *
         * This function can be either blocking or non-blocking depending on the
         * boolean value passed to it. If the call is blocking it can be awoken
         * in a thread safe manner with a call to wake().
         *
         * @param[in] block Set \em true to make the call sleep and wait for new
         *                  data to arrive.
         * @return The socket for which there is new data waiting. Make sure to
         *         Socket::valid() on it to ensure validity.
         */
        Socket poll(bool block);

        //! Wake up from a nap inside poll()
        /*!
         * Calling this simply wakes up the execution thread that poll() is
         * blocking in. If there is no nap taking place, calling this does
         * nothing.
         *
         * This function is thread safe and can be called from anywhere as often
         * as is desired.
         */
        void wake();

        //! How many active sockets (not counting listeners) are in the group
        size_t size() const
        {
            return m_sockets.size();
        }

        //! Should we accept new connections?
        /*!
         * @param [in] status Set to false if you want to start refusing new
         *                    connections. True otherwise (default).
         */
        void accept(bool status);

    private:
        //! Our sockets need access to our private data
        friend class Socket;

        //! These are the sockets we listen for connections on
        std::set<socket_t> m_listeners;

        //! Our poll object
        poll_t m_poll;

        //! A pair of sockets for wakeup purposes
        socket_t m_wakeSockets[2];

        //! Set to true while there is a pending wake
        bool m_waking;

        //! Set to true if we should be accepting new connections
        std::atomic_bool m_accept;

        //! Set to true if we should refresh the listeners in the poll
        std::atomic_bool m_refreshListeners;

        //! We need this mutex to thread safe the wake() function.
        std::mutex m_wakingMutex;

        //! All the sockets
        std::map<socket_t, Socket> m_sockets;

        //! Accept a new connection and create it's socket
        inline void createSocket(const socket_t listener);

        //! Add a socket identifier to the poll list
        bool pollAdd(const socket_t socket);

        //! Remove a socket identifier to the poll list
        bool pollDel(const socket_t socket);

        //! Filenames to cleanup when we're done
        std::deque<std::string> m_filenames;

#if FASTCGIPP_LOG_LEVEL > 3
        //! Debug counter for incoming connections
        std::atomic_ullong m_incomingConnectionCount;

        //! Debug counter for outgoing connections
        std::atomic_ullong m_outgoingConnectionCount;

        //! Debug counter for locally killed sockets
        std::atomic_ullong m_connectionKillCount;

        //! Debug counter for remotely hung up sockets
        std::atomic_ullong m_connectionRDHupCount;

        //! Debug counter for bytes sent
        std::atomic_ullong m_bytesSent;

        //! Debug counter for bytes received
        std::atomic_ullong m_bytesReceived;
#endif
    };
}

#endif
