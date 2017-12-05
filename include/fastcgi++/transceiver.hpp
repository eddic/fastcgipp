/*!
 * @file       transceiver.hpp
 * @brief      Declares the Fastcgipp::Transceiver class
 * @author     Eddie Carle &lt;eddie@isatec.ca&gt;
 * @date       May 3, 2017
 * @copyright  Copyright &copy; 2017 Eddie Carle. This project is released under
 *             the GNU Lesser General Public License Version 3.
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

#ifndef FASTCGIPP_TRANSCEIVER_HPP
#define FASTCGIPP_TRANSCEIVER_HPP

#include <map>
#include <list>
#include <queue>
#include <algorithm>
#include <map>
#include <functional>
#include <memory>
#include <atomic>
#include <mutex>
#include <thread>

#include <fastcgi++/protocol.hpp>
#include "fastcgi++/block.hpp"

//! Topmost namespace for the fastcgi++ library
namespace Fastcgipp
{
    //! Handles low level communication with "the other side"
    /*!
     * This class handles the sending/receiving/buffering of data through the OS
     * level sockets and also the creation/destruction of the sockets
     * themselves.
     *
     * @date    May 4, 2017
     * @author  Eddie Carle &lt;eddie@isatec.ca&gt;
     */
    class Transceiver
    {
    public:
        //! General transceiver handler
        /*!
         * This function is called by Manager::handler() to both transmit data
         * passed to it from requests and relay received data back to them as a
         * Message. The function will return true if there is nothing at all for
         * it to do.
         */
        void handler();

        //! Call from any thread to stop the handler() thread
        /*!
         * Calling this thread will signal the handler() thread to cleanly stop
         * itself. This means it keeps going until all connections are closed.
         * No new connections are accepted.
         *
         * @sa join()
         */
        void stop();

        //! Call from any thread to terminate the handler() thread
        /*!
         * Calling this thread will signal the handler() thread to immediately
         * terminate itself. This means it doesn't wait until all connections
         * are closed.
         *
         * @sa join()
         */
        void terminate();

        //! Call from any thread to start the handler() thread
        /*!
         * If the thread is already running this will do nothing.
         */
        void start();

        //! Block until a stop() or terminate() is called and completed
        void join();

        //! Queue up a block of data for transmission
        /*!
         * @param[in] socket Socket to write the data out
         * @param[in] data Block of data to send out
         * @param[in] kill True if the socket should be closed once everything
         *                 is sent.
         */
        void send(const Socket& socket, Block&& data, bool kill);

        //! Constructor
        /*!
         * Construct a transceiver object based on an initial file descriptor to
         * listen on and a function to pass messages on to.
         *
         * @param[in] sendMessage Function to call to pass messages to requests
         */
        Transceiver(
                const std::function<void(Protocol::RequestId, Message&&)>
                sendMessage);

        ~Transceiver();

        //! Listen to the default Fastcgi socket
        /*!
         * Calling this simply adds the default socket used on FastCGI
         * applications that are initialized from HTTP servers.
         *
         * @return True on success. False on failure.
         */
        bool listen()
        {
            return m_sockets.listen();
        }

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
                const char* group = nullptr)
        {
            return m_sockets.listen(name, permissions, owner, group);
        }

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
                const char* service)
        {
            return m_sockets.listen(interface, service);
        }

    private:
        //! Container associating sockets with their receive buffers
        std::map<Socket, Block> m_receiveBuffers;

        //! Simple FastCGI record to queue up for transmission
        struct Record
        {
            const Socket socket;
            const Block data;
            const char* read;
            const bool kill;

            Record(
                    const Socket& socket_,
                    Block&& data_,
                    bool kill_):
                socket(socket_),
                data(std::move(data_)),
                read(data.begin()),
                kill(kill_)
            {}
        };

        //! %Buffer for transmitting data
        std::deque<std::unique_ptr<Record>> m_sendBuffer;

        //! Thread safe the send buffer
        std::mutex m_sendBufferMutex;

        //! Function to call to pass messages to requests
        const std::function<void(Protocol::RequestId, Message&&)> m_sendMessage;

        //! Listen for connections with this
        SocketGroup m_sockets;

        //! Transmit all buffered data possible
        /*!
         * @return True if we successfully sent all data that was queued up.
         */
        inline bool transmit();

        //! Receive data on the specified socket.
        inline void receive(Socket& socket);

        //! True when handler() should be terminating
        std::atomic_bool m_terminate;

        //! True when handler() should be stopping
        std::atomic_bool m_stop;

        //! Thread our handler is running in
        std::thread m_thread;

        //! Cleanup a dead socket
        void cleanupSocket(const Socket& socket);

#if FASTCGIPP_LOG_LEVEL > 3
        //! Debug counter for locally killed sockets
        std::atomic_ullong m_connectionKillCount;

        //! Debug counter for remotely hung up sockets
        std::atomic_ullong m_connectionRDHupCount;

        //! Debug counter for records sent
        std::atomic_ullong m_recordsSent;

        //! Debug counter for records queued for sending
        std::atomic_ullong m_recordsQueued;

        //! Debug counter for bytes received
        std::atomic_ullong m_recordsReceived;
#endif
    };
}

#endif
