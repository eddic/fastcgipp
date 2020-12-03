/*!
 * @file       manager.hpp
 * @brief      Declares the Manager class
 * @author     Eddie Carle &lt;eddie@isatec.ca&gt;
 * @date       May 18, 2016
 * @copyright  Copyright &copy; 2016 Eddie Carle. This project is released under
 *             the GNU Lesser General Public License Version 3.
 */

/*******************************************************************************
* Copyright (C) 2016 Eddie Carle [eddie@isatec.ca]                             *
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

#ifndef FASTCGIPP_MANAGER_HPP
#define FASTCGIPP_MANAGER_HPP

#include <map>
#include <list>
#include <algorithm>
#include <thread>
#include <mutex>
#include <shared_mutex>
#include <memory>
#include <functional>
#include <condition_variable>

#include "fastcgi++/protocol.hpp"
#include "fastcgi++/transceiver.hpp"
#include "fastcgi++/request.hpp"

//! Topmost namespace for the fastcgi++ library
namespace Fastcgipp
{
    //! General task and protocol management class base
    /*!
     * Handles all task and protocol management, creation/destruction of
     * requests and passing of messages to requests.
     *
     * To operate this class you need to do the following:
     *  - Construct a Manager object
     *  - Perhaps call setupSignals()
     *  - Call at least one of the listen() member functions.
     *  - Call start()
     *  - Call stop() or terminate() when you are done.
     *
     * @date    August 20, 2016
     * @author  Eddie Carle &lt;eddie@isatec.ca&gt;
     */
    class Manager_base
    {
    public:
        //! Sole constructor
        /*!
         * @param[in] threads Number of threads to use for request handling
         */
        Manager_base(unsigned threads);

        ~Manager_base();

        //! Call from any thread to terminate the Manager
        /*!
         * This function is intended to be called from a thread separate from
         * the Manager in order to terminate it. It should also be called by a
         * signal handler in the case of of a SIGTERM. It will force the manager
         * to terminate immediately.
         *
         * @sa join()
         * @sa setupSignals()
         * @sa signalHandler()
         */
        void terminate();

        //! Call from any thread to stop the Manager
        /*!
         * This function is intended to be called from a signal handler in the
         * case of of a SIGUSR1. It is similar to terminate() except the
         * Manager will wait until all requests are complete before halting.
         *
         * @sa join()
         * @sa setupSignals()
         * @sa signalHandler()
         */
        void stop();

        //! Call from any thread to start the Manager
        /*!
         * If the Manager is already running this will do nothing.
         */
        void start();

        //! Block until a stop() or terminate() is called and completed
        void join();

        //! Configure the handlers for POSIX signals
        /*!
         * By calling this function appropriate handlers will be set up for
         * SIGPIPE, SIGUSR1 and SIGTERM.
         *
         * @sa signalHandler()
         */
        static void setupSignals();

        //! Listen to the default Fastcgi socket
        /*!
         * Calling this simply adds the default socket used on FastCGI
         * applications that are initialized from HTTP servers.
         *
         * @return True on success. False on failure.
         */
        bool listen()
        {
            return m_transceiver.listen();
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
            return m_transceiver.listen(name, permissions, owner, group);
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
            return m_transceiver.listen(interface, service);
        }

        //! Pass a message to a request
        void push(Protocol::RequestId id, Message&& message);

        //! Should we set socket option to reuse address
        /*!
         * @param [in] status Set to true if you want to reuse address.
         *                    False otherwise (default).
         */
        void reuseAddress(bool value)
        {
            m_transceiver.reuseAddress(value);
        }

        //! Call before start to change the number of threads
        /*!
         * If the Manager is already running this will do nothing.
         *
         * @param[in] threads Number of threads to use for request handling
         *
         * @sa Manager_base()
         * @sa start()
         */
        void resizeThreads(unsigned threads);

    protected:
        //! Make a request object
        virtual std::unique_ptr<Request_base> makeRequest(
                const Protocol::RequestId& id,
                const Protocol::Role& role,
                bool kill) =0;

        //! Handles low level communication with the other side
        Transceiver m_transceiver;

    private:
        //! Queue for pending tasks
        std::queue<Protocol::RequestId> m_tasks;

        //! Thread safe our tasks
        std::mutex m_tasksMutex;

        //! An associative container for our requests
        Protocol::Requests<std::unique_ptr<Request_base>> m_requests;

        //! Thread safe our requests
        std::shared_timed_mutex m_requestsMutex;

        //! Local messages
        std::queue<std::pair<Message, Socket>> m_messages;

        //! Thread safe our local messages
        std::mutex m_messagesMutex;

        //! General handling function to have it's own thread
        void handler();

        //! Handles management messages
        /*!
         * This function is called by handler() in the case that a management
         * message is recieved.
         */
        inline void localHandler();

        //! True when the manager should be terminating
        bool m_terminate;

        //! True when the manager should be stopping
        bool m_stop;

        //! Thread safe starting and stopping
        std::mutex m_startStopMutex;

        //! Threads our manager is running in
        std::vector<std::thread> m_threads;

        //! Condition variable to wake handler() threads up
        std::condition_variable m_wake;

        //! General function to handler POSIX signals
        static void signalHandler(int signum);

        //! Pointer to the %Manager object
        static Manager_base* instance;

#if FASTCGIPP_LOG_LEVEL > 3
        //! Debug counter for new requests
        std::atomic_ullong m_requestCount;

        //! Debug counter for max requests
        size_t m_maxRequests;

        //! Debug counter for management records
        std::atomic_ullong m_managementRecordCount;

        //! Debug counter for bad socket messages
        std::atomic_ullong m_badSocketMessageCount;

        //! Debug counter for bad socket kills
        std::atomic_ullong m_badSocketKillCount;

        //! Debug counter for request messages received
        std::atomic_ullong m_messageCount;

        //! Debug counter currently active handler() threads
        unsigned m_activeThreads;

        //! Debug counter max active handler() threads
        unsigned m_maxActiveThreads;
#endif
    };

    //! General task and protocol management class
    /*!
     * Handles all task and protocol management, creation/destruction of
     * requests and passing of messages to requests. The template argument
     * should be a class type derived from the Request class with at least the
     * response() function defined.
     *
     * To operate this class you need to do the following:
     *  - Construct a Manager object
     *  - Perhaps call setupSignals()
     *  - Call at least one of the listen() member functions.
     *  - Call start()
     *  - Call stop() or terminate() when you are done.
     *
     * @tparam RequestT A class type derived from the Request class with at
     *                  least the Request::response() function defined.
     *
     * @date    May 13, 2016
     * @author  Eddie Carle &lt;eddie@isatec.ca&gt;
     */
    template<class RequestT> class Manager: public Manager_base
    {
    public:
        //! Sole constructor
        /*!
         * @param[in] threads Number of threads to use for request handling
         */
        Manager(unsigned threads = std::thread::hardware_concurrency()):
            Manager_base(threads)
        {}

    private:
        //! Make a request object
        std::unique_ptr<Request_base> makeRequest(
                const Protocol::RequestId& id,
                const Protocol::Role& role,
                bool kill)
        {
            using namespace std::placeholders;

            std::unique_ptr<RequestT> request(new RequestT);
            request->configure(
                    id,
                    role,
                    kill,
                    std::bind(&Transceiver::send, &m_transceiver, _1, _2, _3),
                    std::bind(&Manager_base::push, this, id, _1));
            return request;
        }

    };
}

#endif
