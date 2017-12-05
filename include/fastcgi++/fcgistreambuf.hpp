/*!
 * @file       fcgistreambuf.hpp
 * @brief      Declares the FcgiStreambuf class
 * @author     Eddie Carle &lt;eddie@isatec.ca&gt;
 * @date       June 3, 2017
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

#ifndef FASTCGIPP_FCGISTREAMBUF_HPP
#define FASTCGIPP_FCGISTREAMBUF_HPP

#include "fastcgi++/protocol.hpp"
#include "fastcgi++/webstreambuf.hpp"
#include "fastcgi++/block.hpp"

#include <istream>
#include <functional>

//! Topmost namespace for the fastcgi++ library
namespace Fastcgipp
{
    //! Stream buffer class for output of client data through FastCGI
    /*!
     * This class is derived from WebStreambuf<charT, traits>. It acts
     * just the same with the added feature of the dump() function but properly
     * flushes into FastCGI records.
     *
     * @tparam charT Character type (char or wchar_t)
     * @tparam traits Character traits
     *
     * @date    May 3, 2017
     * @author  Eddie Carle &lt;eddie@isatec.ca&gt;
     */
    template <class charT, class traits = std::char_traits<charT>>
    class FcgiStreambuf: public WebStreambuf<charT, traits>
    {
    public:
        FcgiStreambuf()
        {
            this->setp(m_buffer, m_buffer+s_buffSize);
        }

        //! Configure the stream buffer
        /*!
         * Sets FastCGI related member data necessary for operation of the
         * stream buffer.
         *
         * @param[in] id Complete ID associated with the request
         * @param[in] type Type of output stream (ERR or OUT)
         * @param[in] send_ Function to send record with
         */
        void configure(
                const Protocol::RequestId& id,
                const Protocol::RecordType& type,
                const std::function<void(const Socket&, Block&&)>
                    send_)
        {
            m_id = id;
            m_type = type;
            send = send_;
        }

        virtual ~FcgiStreambuf()
        {
            sync();
        }

        //! Dumps raw data directly into the FastCGI protocol
        /*!
         * This function exists as a mechanism to dump raw data out the stream
         * bypassing the stream buffer or any code conversion mechanisms. If the
         * user has any binary data to send, this is the function to do it with.
         *
         * @param[in] data Pointer to first byte of data to send
         * @param[in] size Size in bytes of data to be sent
         */
        void dump(const char* data, size_t size);

        //! Dumps an input stream directly into the FastCGI protocol
        /*!
         * This function exists as a mechanism to dump a raw input stream out
         * this stream bypassing the stream buffer or any code conversion
         * mechanisms. Typically this would be a filestream associated with an
         * image or something. The stream is transmitted until an EOF.
         *
         * @param[in] stream Reference to input stream that should be
         *                   transmitted.
         */
        void dump(std::basic_istream<char>& stream);

    private:
        typedef typename std::basic_streambuf<charT, traits>::int_type int_type;
        typedef typename std::basic_streambuf<charT, traits>::traits_type traits_type;
        typedef typename std::basic_streambuf<charT, traits>::char_type char_type;

        int_type overflow(int_type c = traits_type::eof());

        int sync()
        {
            return emptyBuffer()?0:-1;
        }

        //! Code converts, packages and transmits all data in the stream buffer
        bool emptyBuffer();

        //! Size of the internal stream buffer
        static const int s_buffSize = 8192;

        //! The buffer
        char_type m_buffer[s_buffSize];

        //! ID associated with the request
        Protocol::RequestId m_id;

        //! Type of output stream (ERR or OUT)
        Protocol::RecordType m_type;

        //! Function to actually send the record
        std::function<void(const Socket&, Block&&)> send;
    };
}

#endif
