/*!
 * @file       mailer.hpp
 * @brief      Declares types for sending emails
 * @author     Eddie Carle &lt;eddie@isatec.ca&gt;
 * @date       May 12, 2019
 * @copyright  Copyright &copy; 2019 Eddie Carle. This project is released under
 *             the GNU Lesser General Public License Version 3.
 */
/*******************************************************************************
* Copyright (C) 2019 Eddie Carle [eddie@isatec.ca]                             *
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

#ifndef FASTCGIPP_MAILER_HPP
#define FASTCGIPP_MAILER_HPP

#include <queue>
#include <atomic>
#include <mutex>
#include <thread>
#include <condition_variable>

#include "fastcgi++/sockets.hpp"
#include "fastcgi++/email.hpp"

//! Topmost namespace for the fastcgi++ library
namespace Fastcgipp
{
    //! Contains the stuff for composing and sending emails
    namespace Mail
    {
        //! Handles low level communication with an SMTP server
        /*!
         * This class handles connections to the SMTP and it's queue of emails.
         * It can safely be constructed in the global space. Once constructed
         * call init() to initialize it. Call start() to start the handling
         * thread. Once you're done, call stop() or terminate() to finish things
         * and then call join() to wait for the thread to complete.
         *
         * Queue up emails for sending with queue().
         *
         * @date    May 12, 2019
         * @author  Eddie Carle &lt;eddie@isatec.ca&gt;
         */
        class Mailer
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

            //! Queue up an email
            void queue(Email_base& email);

            //! Initialize the mailer
            /*!
             * Note that this function can only be called _once_.
             *
             * @param [in] host Hostname/address of the SMTP server.
             * @param [in] origin Hostname/address of origin.
             * @param [in] port Server port number
             * @param [in] retryInterval How many seconds before retrying a bad
             *                           connection to the SMTP server?
             */
            void init(
                    const char* host,
                    const char* origin,
                    const unsigned short port=25,
                    unsigned retryInterval=30);

            virtual ~Mailer();

            Mailer():
                m_initialized(false),
                m_state(DISCONNECTED)
            {}

        private:
            //! General mailer handler
            void handler();

            //! True if we're initialized
            bool m_initialized;

            //! %Buffer for transmitting data
            std::queue<Email_base::Data> m_queue;

            //! True when handler() should be terminating
            std::atomic_bool m_terminate;

            //! True when handler() should be stopping
            std::atomic_bool m_stop;

            //! Thread our handler is running in
            std::thread m_thread;

            //! Always practice safe threading
            std::mutex m_mutex;

            //! We'll use this to wake up from the retry interval sleep
            std::condition_variable m_wake;

            //! Hostname/address of server
            std::string m_host;

            //! Origin hostname
            std::string m_origin;

            //! Server port
            std::string m_port;

            //! %Mailer retry interval
            unsigned m_retry;

            //! The socket group
            SocketGroup m_socketGroup;

            //! The connection socket
            Socket m_socket;

            //! Current email
            Email_base::Data m_email;

            //! Are we currently working on an email?
            bool inEmail() const
            {
                return !m_email.to.empty();
            }

            //! Purge current email
            void purgeEmail()
            {
                m_email.body.clear();
                m_email.to.clear();
                m_email.from.clear();
            }

            //! State enumeration
            enum State
            {
                DISCONNECTED,
                CONNECTED,
                EHLO,
                EIGHTBIT,
                MAIL,
                RCPT,
                DATA,
                DUMP,
                QUIT,
                ERROR
            };

            //! Current state
            State m_state;

            //! Current line being read from SMTP server
            std::string m_line;
        };
    }
}

#endif
