/*!
 * @file       curler.hpp
 * @brief      Declares types for sending ecurls
 * @author     Eddie Carle &lt;eddie@isatec.ca&gt;
 * @date       May 7, 2020
 * @copyright  Copyright &copy; 2020 Eddie Carle. This project is released under
 *             the GNU Lesser General Public License Version 3.
 */
/*******************************************************************************
* Copyright (C) 2020 Eddie Carle [eddie@isatec.ca]                             *
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

#ifndef FASTCGIPP_CURLER_HPP
#define FASTCGIPP_CURLER_HPP

#include <queue>
#include <atomic>
#include <mutex>
#include <thread>
#include <map>

#include "fastcgi++/poll.hpp"
#include "fastcgi++/curl.hpp"

//! Topmost namespace for the fastcgi++ library
namespace Fastcgipp
{
    //! Handles multiple asynchronous curl requests
    /*!
     * This class handles outgoing HTTP requests via curl.  It can safely be
     * constructed in the global space. Once constructed call start() to start
     * the handling thread. Once you're done, call stop() or terminate() to
     * finish things and then call join() to wait for the thread to complete.
     *
     * Queue up requests for sending with queue().
     *
     * @date    May 7, 2020
     * @author  Eddie Carle &lt;eddie@isatec.ca&gt;
     */
    class Curler
    {
    public:
        //! Call from any thread to stop the handler() thread
        /*!
         * Calling this thread will signal the handler() thread to
         * gracefully stop itself. This means it waits until all
         * queued queries are finished
         *
         * @sa join()
         */
        void stop();

        //! Call from any thread to terminate the handler() thread
        /*!
         * Calling this thread will signal the handler() thread to
         * immediately terminate itself. This means it doesn't wait until
         * the currently queued queries are finished.
         *
         * @sa join()
         */
        void terminate();

        //! Call from any thread to start the handler() thread
        /*!
         * If the thread is already running this will do nothing.
         */
        void start();

        //! %Block until a stop() or terminate() is completed
        void join();

        //! Queue up an curl
        void queue(Curl_base& curl);

        virtual ~Curler();

        //! Construct a Curler object
        /*!
         * @param concurrency Maximum amount of current requests to be handled
         */
        Curler(unsigned concurrency=1);

    private:
        //! General curler handler
        void handler();

        //! How many concurrent can we do?
        const unsigned m_concurrency;

        //! %Buffer for transmitting data
        std::queue<Curl_base> m_queue;

        //! True when handler() should be terminating
        std::atomic_bool m_terminate;

        //! True when handler() should be stopping
        std::atomic_bool m_stop;

        //! Thread our handler is running in
        std::thread m_thread;

        //! Always practice safe threading
        std::mutex m_mutex;

        //! A pair of sockets for wakeup purposes
        socket_t m_wakeSockets[2];

        //! Set to true while there is a pending wake
        bool m_waking;

        //! The polling object
        Poll m_poll;

        //! Call to wake up
        void wake();

        //! Associative array linked sockets to handles
        std::map<void*, Curl_base> m_handles;

        //! Curl multi handle
        void* const m_multiHandle;

        //! Curl socket action callback function
        static int socketCallback(
                void* handle,
                int socket,
                int action,
                void *userp,
                void *socketp);
    };
}

#endif
