/*!
 * @file       connection.hpp
 * @brief      Declares the Fastcgipp::SQL::Connection class
 * @author     Eddie Carle &lt;eddie@isatec.ca&gt;
 * @date       October 12, 2018
 * @copyright  Copyright &copy; 2018 Eddie Carle. This project is released under
 *             the GNU Lesser General Public License Version 3.
 */

/*******************************************************************************
* Copyright (C) 2018 Eddie Carle [eddie@isatec.ca]                             *
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

#ifndef FASTCGIPP_CONNECTION_HPP
#define FASTCGIPP_CONNECTION_HPP

#include <deque>
#include <map>
#include <functional>
#include <atomic>
#include <mutex>
#include <thread>

#include "fastcgi++/sockets.hpp"
#include "fastcgi++/message.hpp"
#include "fastcgi++/sql/parameters.hpp"
#include "fastcgi++/sql/results.hpp"

//! Topmost namespace for the fastcgi++ library
namespace Fastcgipp
{
    //! Contains all fastcgi++ %SQL facilities
    namespace SQL
    {
        //! Structure to hold %SQL query data
        struct Query
        {
            //! Statement
            const char* statement;
            
            //! %Parameters
            std::shared_ptr<Parameters_base> parameters;

            //! %Results
            std::shared_ptr<Results_base> results;

            //! Callback function to call when query is complete
            std::function<void(Message)> callback;
        };

        //! Handles low level communication with "the other side"
        /*!
         * This class handles connections to the database and its queries. It
         * can safely be constructed in the global space. Once constructed call
         * init() to initialize it. Call start() to start the handling thread.
         * Once you're done, call stop() or terminate() to finish things and
         * then call join() to wait for the thread to complete.
         *
         * Queue up queries with queue().
         *
         * @date    October 7, 2018
         * @author  Eddie Carle &lt;eddie@isatec.ca&gt;
         */
        class Connection
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

            //! Queue up a query
            bool queue(const Query& query);

            //! Initialize the connection
            /*!
             * Note that this function can only be called _once_.
             *
             * @param [in] host Hostname/address/socket of the SQL server.
             * @param [in] db Database name
             * @param [in] username Server username
             * @param [in] password Server password
             * @param [in] port Server port number
             * @param [in] concurrency How many connnections/simultaneous
             *                         queries?
             * @param [in] messageType Type value for Message sent via callback.
             * @param [in] retryInterval How many seconds before retrying a bad
             *                           connection to the SQL server?
             */
            void init(
                    const char* host,
                    const char* db,
                    const char* username,
                    const char* password,
                    const unsigned concurrency=1,
                    const unsigned short port=5432,
                    int messageType=5432,
                    unsigned retryInterval=30);

            virtual ~Connection();

            Connection():
                m_initialized(false)
            {}

        private:
            //! General connection handler
            void handler();

            //! Call this to wakeup the thread if it's sleeping
            void wake();

            //! Call this to initiate all connections with the server
            void connect();

            //! Are we fully connected?
            bool connected()
            {
                return m_connections.size() == m_concurrency;
            }

            //! True if we're initialized
            bool m_initialized;

            struct Conn
            {
                bool idle;
                void* connection;
                Query query;
            };

            //! Container associating sockets with their receive buffers
            std::map<socket_t, Conn> m_connections;

            //! Kill and destroy this specific connection
            void kill(std::map<socket_t, Conn>::iterator& conn);

            //! Kill and destroy everything!!
            void killAll();

            //! %Buffer for transmitting data
            std::deque<Query> m_queue;

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

            //! Hostname/address/socket of server
            std::string m_host;

            //! Database name
            std::string m_db;

            //! Server username
            std::string m_username;

            //! Server password
            std::string m_password;

            //! Server port
            std::string m_port;

            //! %Connection retry interval
            unsigned m_retry;

            //! How many concurrent queries shall we allow?
            unsigned m_concurrency;

            //! Callback message type ID
            unsigned m_messageType;

            //! The poll group
            Poll m_poll;
        };
    }
}

#endif
