/*!
 * @file       email.hpp
 * @brief      Declares types for composing emails
 * @author     Eddie Carle &lt;eddie@isatec.ca&gt;
 * @date       May 4, 2020
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

#ifndef FASTCGIPP_EMAIL_HPP
#define FASTCGIPP_EMAIL_HPP

#include <memory>
#include <list>
#include <ostream>

#include "fastcgi++/chunkstreambuf.hpp"

//! Topmost namespace for the fastcgi++ library
namespace Fastcgipp
{
    //! Contains the stuff for composing and sending emails
    namespace Mail
    {

        //! De-templated base class for Email
        /*!
         * @date    May 4, 2020
         * @author  Eddie Carle &lt;eddie@isatec.ca&gt;
         */
        class Email_base
        {
        public:
            //! %Email message data
            struct DataRef
            {
                //! Linked list of body chunks
                std::list<ChunkStreamBuf_base::Chunk>& body;

                //! Recipient email address
                std::string to;

                //! From email address
                std::string from;

                DataRef(std::list<ChunkStreamBuf_base::Chunk>& _body):
                    body(_body)
                {}
            };

            //! %Email message data
            struct Data
            {
                //! Linked list of body chunks
                std::list<ChunkStreamBuf_base::Chunk> body;

                //! Recipient email address
                std::string to;

                //! From email address
                std::string from;

                Data()
                {}

                Data(DataRef&& dataRef):
                    body(std::move(dataRef.body)),
                    to(std::move(dataRef.to)),
                    from(std::move(dataRef.from))
                {}
            };

            //! Retrieve email message data. Makes stream EOF.
            Data data()
            {
                close();
                return std::move(m_data);
            }
            virtual ~Email_base() = default;
        protected:
            //! %Email message data
            DataRef m_data;

            //! Flush the buffer and set the stream to EOF.
            virtual void close() =0;

            Email_base(std::list<ChunkStreamBuf_base::Chunk>& body):
                m_data(body)
            {}
        };

        //! Object for composing email messages
        /*!
         * Use this class to compose email messages from the template character
         * type. This class inherits from std::basic_ostream so the body can be
         * composed using stream insertion operators.
         *
         * Use the to() and from() member functions to set the to/from email
         * addresses.
         *
         * @tparam charT Character type for internal processing (wchar_t or
         *               char)
         * @date    December 22, 2018
         * @author  Eddie Carle &lt;eddie@isatec.ca&gt;
         */
        template <class charT>
        class Email: public std::basic_ostream<charT>, public Email_base
        {
        private:
            //! Our stream buffer object
            ChunkStreamBuf<charT> m_streamBuffer;

            void close();

        public:
            Email();
            ~Email() override = default;

            //! Set the to address
            void to(const std::basic_string<charT>& address);

            //! Set the to address with move semantics
            void to(std::basic_string<charT>&& address);

            //! Set the from address
            void from(const std::basic_string<charT>& address);

            //! Set the from address with move semantics
            void from(std::basic_string<charT>&& address);
        };

        template<> inline void Email<char>::to(const std::string& address)
        {
            m_data.to = address;
        }

        template<> inline void Email<char>::to(std::string&& address)
        {
            m_data.to = std::move(address);
        }

        template<> inline void Email<char>::from(const std::string& address)
        {
            m_data.from = address;
        }

        template<> inline void Email<char>::from(std::string&& address)
        {
            m_data.from = std::move(address);
        }

        template<> void Email<wchar_t>::to(const std::wstring& address);
        template<> inline void Email<wchar_t>::to(std::wstring&& address)
        {
            to(address);
        }

        template<> void Email<wchar_t>::from(const std::wstring& address);
        template<> inline void Email<wchar_t>::from(std::wstring&& address)
        {
            from(address);
        }
    }
}

#endif
