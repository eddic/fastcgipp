/*!
 * @file       poll.hpp
 * @brief      Declares everything for interfaces with OS level socket polling.
 * @author     Eddie Carle &lt;eddie@isatec.ca&gt;
 * @date       May 5, 2020
 * @copyright  Copyright &copy; 2020 Eddie Carle. This project is released under
 *             the GNU Lesser General Public License Version 3.
 *
 * It is this file, along with poll.cpp, that must be modified to make
 * fastcgi++ work on Windows. Although not completely sure, I believe all that
 * needs to be modified in this file is Fastcgipp::socket_t and
 * Fastcgipp::Poll.
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

#ifndef FASTCGIPP_POLL_HPP
#define FASTCGIPP_POLL_HPP

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

    //! Class for handling OS level socket polling
    /*!
     * This class introduces a layer of abstraction to the polling interface
     * used by the SocketGroup class and other facilities of within fastcgi++.
     * Cross-platform development will require modification of this class.
     *
     * @date    October 3, 2018
     * @author  Eddie Carle &lt;eddie@isatec.ca&gt;
     */
    class Poll
    {
    private:
#ifdef FASTCGIPP_LINUX
        //! Our polling type using the Linux kernel is simply an int
        typedef const int poll_t;
#elif defined FASTCGIPP_UNIX
        //! Our polling type using generic Unix is an array of pollfds
        typedef std::vector<pollfd> poll_t;
#endif

        //! The OS level polling object
        poll_t m_poll;

    public:
        //! Add a socket identifier to the poll list
        bool add(const socket_t socket);

        //! Remove a socket identifier to the poll list
        bool del(const socket_t socket);

        //! Type returned from a poll request
        class Result
        {
        private:
            //! Event: there is data to read
            static const unsigned pollIn;

            //! Event: there is an error in the socket
            static const unsigned pollErr;

            //! Event: the socket has been "hung up" on this end
            static const unsigned pollHup;

            //! Event: the socket has been "hung up" on the other end
            static const unsigned pollRdHup;

            //! Event register
            unsigned m_events;

            //! Associated socket
            socket_t m_socket;

            //! True if the poll actually returned a socket with an event
            bool m_data;

            friend class Poll;

            //! Only Poll should be able to construct these
            Result():
                m_events(0),
                m_data(false)
            {}

        public:
            //! Get socket id associated with poll event
            socket_t socket() const
            {
                return m_socket;
            }

            //! True if the poll actually returned a socket with an event
            explicit operator bool() const
            {
                return m_data;
            }

            //! Associated event register
            unsigned events() const
            {
                return m_events;
            }

            //! True if the socket has been "hung up" on this end
            bool hup() const
            {
                return m_events & pollHup;
            }

            //! True if the socket has been "hung up" on the other end
            bool rdHup() const
            {
                return m_events & pollRdHup;
            }

            //! True if the socket has an error
            bool err() const
            {
                return m_events & pollErr;
            }

            //! True if the socket has data to read
            bool in() const
            {
                return m_events & pollIn;
            }

            //! True if and only if the socket has data to read
            bool onlyIn() const
            {
                return m_events == pollIn;
            }
        };

        //! Initiate poll on group
        /*!
         * @param [in] timeout 0 means don't block at all. -1 means block
         *                     indefinitely. A positive integer is the number of
         *                     milliseconds before blocking times out.
         */
        Result poll(int timeout);

        Poll();
        virtual ~Poll();
    };
}

#endif
