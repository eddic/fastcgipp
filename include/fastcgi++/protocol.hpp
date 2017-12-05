/*!
 * @file       protocol.hpp
 * @brief      Declares everything for relating to the FastCGI protocol itself.
 * @author     Eddie Carle &lt;eddie@isatec.ca&gt;
 * @date       May 10, 2017
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

#ifndef FASTCGIPP_PROTOCOL_HPP
#define FASTCGIPP_PROTOCOL_HPP

#include <memory>
#include <cstdint>
#include <algorithm>
#include <map>
#include <vector>

#include "fastcgi++/message.hpp"
#include "fastcgi++/sockets.hpp"

//! Topmost namespace for the fastcgi++ library
namespace Fastcgipp
{
    //! Defines the fastcgi++ version
    extern const char version[];

    //! Defines aspects of the FastCGI %Protocol
    /*!
     * The %Protocol namespace defines the data structures and constants used
     * by the FastCGI protocol version 1. All data has been modelled after the
     * official FastCGI protocol specification located at
     * http://www.fastcgi.com/devkit/doc/fcgi-spec.html
     */
    namespace Protocol
    {
        //! The internal ID of a FastCGI request
        typedef uint16_t FcgiId;

        //! Constant that defines a bad/special FcgiId
        const uint16_t badFcgiId = 0xffffUL;

        //! A unique identifier for each FastCGI request
        /*!
         * Because each FastCGI request has both a RequestID and a Socket
         * associated with it, this class defines an ID value that encompasses
         * both.
         *
         * @date    May 6, 2016
         * @author  Eddie Carle &lt;eddie@isatec.ca&gt;
         */
        struct RequestId
        {
            //! Construct from an FcgiId and a Socket
            /*!
             * The constructor builds upon a RequestID and the Socket it is
             * communicating through.
             *
             * @param [in] id The internal FastCGI request ID.
             * @param [in] socket The associated socket.
             */
            RequestId(
                    FcgiId id,
                    const Socket& socket):
                m_socket(socket),
                m_id(id)
            {}

            RequestId(const RequestId& x):
                m_socket(x.m_socket),
                m_id(x.m_id)
            {}

            RequestId():
                m_socket(Socket()),
                m_id(badFcgiId)
            {}

            RequestId& operator=(const RequestId& x)
            {
                m_socket = x.m_socket;
                m_id = x.m_id;
                return *this;
            }

            //! Associated socket
            Socket m_socket;

            //! Internal FastCGI request ID
            FcgiId m_id;

            //! We need this uglyness to find ranges based purely on the socket
            struct Less
            {
                using is_transparent = std::true_type;

                bool operator()(const RequestId& x, const RequestId& y) const
                {
                    if(x.m_socket == y.m_socket)
                        return x.m_id < y.m_id;
                    else
                        return x.m_socket < y.m_socket;
                }

                bool operator()(const RequestId& id, const Socket& socket) const
                {
                    return id.m_socket < socket;
                }

                bool operator()(const Socket& socket, const RequestId& id) const
                {
                    return socket < id.m_socket;
                }
            };
        };

        //! A simple associative container that indexes with RequestId
        template<class T>
        using Requests = std::map<RequestId, T, RequestId::Less>;

        //! Defines the types of records within the FastCGI protocol
        enum class RecordType: uint8_t
        {
            BEGIN_REQUEST=1,
            ABORT_REQUEST=2,
            END_REQUEST=3,
            PARAMS=4,
            IN=5,
            OUT=6,
            ERR=7,
            DATA=8,
            GET_VALUES=9,
            GET_VALUES_RESULT=10,
            UNKNOWN_TYPE=11
        };

        //! The version of the FastCGI protocol that this adheres to
        const int version=1;

        //! All FastCGI records will be a multiple of this many bytes
        const int chunkSize=8;

        //! Defines the possible roles a FastCGI application may play
        enum class Role: uint16_t
        {
            RESPONDER=1,
            AUTHORIZER=2,
            FILTER=3
        };

        //! Possible statuses a request may declare when complete
        enum class ProtocolStatus: uint8_t
        {
            REQUEST_COMPLETE=0,
            CANT_MPX_CONN=1,
            OVERLOADED=2,
            UNKNOWN_ROLE=3
        };

        //! Get unsigned integral type from size
        template<size_t Size> struct Unsigned;
        template<> struct Unsigned<2>
        {
            typedef uint16_t Type;
        };
        template<> struct Unsigned<4>
        {
            typedef uint32_t Type;
        };
        template<> struct Unsigned<8>
        {
            typedef uint64_t Type;
        };

        //! Allows raw storage of types in big endian format
        /*!
         * This templated class allows any integral based (enumerations
         * included) type to be stored in big endian format but maintain type
         * compatibility.
         *
         * @tparam T Type to emulate. Must be either an integral type or an
         *           enumeration who's underlying type is integral.
         * @date    May 10, 2017
         * @author  Eddie Carle &lt;eddie@isatec.ca&gt;
         */
        template<typename T> class BigEndian
        {
        private:
            static const size_t size = sizeof(T);
            static_assert(size==2 || size==4 || size==8, "Fastcgipp::Protocol::"
                    "BigEndian can only work with types of size 2, 4 or 8.");

            //! Underlying unsigned integral type.
            typedef typename Unsigned<size>::Type BaseType;

            //! The raw data of the big endian integer
            unsigned char m_data[size];

            //! Set the internal data to the passed parameter.
            void set(T x)
            {
                union
                {
                    BaseType base;
                    T actual;
                };
                actual = x;
                for(unsigned int i=0; i<size; ++i)
                    m_data[i] = static_cast<unsigned char>(
                            0xff & base>>8*(size-1-i));
            }

        public:
            BigEndian& operator=(T x)
            {
                set(x);
                return *this;
            }

            BigEndian(T x)
            {
                set(x);
            }

            constexpr BigEndian()
            {}

            operator T() const
            {
                return read(m_data);
            }

            //! Static function for reading the value out of a data array.
            /*!
             * This will read the value out of an unsigned char array in big
             * endian format and cast it into type T.
             *
             * @param [in] source Pointer to start of data. This data should of
             *                    course be at minimum size.
             */
            static T read(const unsigned char* source)
            {
                union
                {
                    BaseType base;
                    T actual;
                };
                base = 0;

                for(unsigned int i=0; i<size; ++i)
                    base |= static_cast<BaseType>(*(source+i))
                        << 8*(size-1-i);

                return actual;
            }

            //! Simply casts char to unsigned char.
            static T read(const char* source)
            {
                return read(reinterpret_cast<const unsigned char*>(source));
            }
        };

        //! Data structure used as the header for FastCGI records
        /*!
         * This structure defines the header used in FastCGI records. It can be
         * casted to and from raw 8 byte blocks of data and transmitted/received
         * as is.
         *
         * @date    March 6, 2016
         * @author  Eddie Carle &lt;eddie@isatec.ca&gt;
         */
        struct Header
        {
            //! FastCGI version number
            uint8_t version;

            //! Record type
            RecordType type;

            //! Request ID
            BigEndian<FcgiId> fcgiId;

            //! Content length
            BigEndian<uint16_t> contentLength;

            //! Length of record padding
            uint8_t paddingLength;

            //! Reseved for future use and header padding
            uint8_t reserved;
        };

        //! The body for FastCGI records with a RecordType of BEGIN_REQUEST
        /*!
         * This structure defines the body used in FastCGI BEGIN_REQUEST
         * records. It can be casted from raw 8 byte blocks of data and received
         * as is. A BEGIN_REQUEST record is received when the other side wished
         * to make a new request.
         *
         * @date    May 11, 2016
         * @author  Eddie Carle &lt;eddie@isatec.ca&gt;
         */
        struct BeginRequest
        {
            //! Flag bit representing the keep alive value
            static const int keepConnBit = 1;

            //! Get keep alive value from the record body
            /*!
             * If this value is false, the socket should be closed on our side
             * when the request is complete.  If true, the other side will close
             * the socket when done and potentially reuse the socket and
             * multiplex other requests on it.
             *
             * @return Boolean value as to whether or not the connection is kept
             *         alive.
             */
            bool kill() const
            {
                return !(flags & keepConnBit);
            }

            //! Role
            BigEndian<Role> role;

            //! Flag value
            uint8_t flags;

            //! Reserved for future use and body padding
            uint8_t reserved[5];
        };

        //! The body for FastCGI records with a RecordType of UNKNOWN_TYPE
        /*!
         * This structure defines the body used in FastCGI UNKNOWN_TYPE records.
         * It can be casted to raw 8 byte blocks of data and transmitted as is.
         * An UNKNOWN_TYPE record is sent as a reply to record types that are
         * not recognized.
         *
         * @date    March 6, 2016
         * @author  Eddie Carle &lt;eddie@isatec.ca&gt;
         */
        struct UnknownType
        {
            //! Unknown record type
            RecordType type;

            //! Reseved for future use and body padding
            uint8_t reserved[7];
        };

        //! The body for FastCGI records of type RecordType::END_REQUEST
        /*!
         * This structure defines the body used in FastCGI END_REQUEST records.
         * It can be casted to raw 8 byte blocks of data and transmitted as is.
         * An END_REQUEST record is sent when this side wishes to terminate a
         * request. This can be simply because it is complete or because of a
         * problem.
         *
         * @date    March 6, 2016
         * @author  Eddie Carle &lt;eddie@isatec.ca&gt;
         */
        struct EndRequest
        {
            //! Return value
            BigEndian<int32_t> appStatus;

            //! Requests Status
            ProtocolStatus protocolStatus;

            //! Reseved for future use and body padding
            uint8_t reserved[3];
        };

        //! Process the body of a FastCGI record of type RecordType::PARAMS
        /*!
         * Takes the body of a FastCGI record of type RecordType::PARAMS and
         * parses it. You end up with iterators giving you both the name and
         * value of the parameter.
         *
         * The return value indicates whether or not there is actually
         * sufficient data in the array to to read both the sizes and the values
         * themselves.
         *
         * @param[in] data Iterator to the first byte of the record body
         * @param[in] dataEnd Iterator to 1+ the last byte of the record body
         * @param[out] name Reference to an iterator that will be pointed to the
         *                  first byte of the parameter name. If false is
         *                  returned this value is undefined.
         * @param[out] value Reference to an iterator that will be pointed to
         *                   the first byte of the parameter value. If false is
         *                   returned this value is undefined.
         * @param[out] end Reference to an iterator that will be pointed to 1+
         *                 the last byte of the parameter value. If false is
         *                 returned this value is undefined.
         * @return False if out of bounds. True otherwise.
         */
        bool processParamHeader(
                const char* data,
                const char* const dataEnd,
                const char*& name,
                const char*& value,
                const char*& end);

        //! For the reply of FastCGI management records
        /*!
         * This class template is an efficient tool for replying to
         * RecordType::GET_VALUES management records. The structure represents a
         * complete record (body+header) of a name-value pair to be sent as a
         * reply to a management value query. The templating allows the
         * structure to be exactly the size that is needed so it can be casted
         * to raw data and transmitted as is. Note that the name and value
         * lengths are left as single bytes so they are limited in range from
         * 0-127.
         *
         * @tparam NAMELENGTH Length of name in bytes (0-127). Null terminator
         *                    not included.
         * @tparam VALUELENGTH Length of value in bytes (0-127). Null terminator
         *                     not included.
         *
         * @date    March 6, 2016
         * @author  Eddie Carle &lt;eddie@isatec.ca&gt;
         */
        template<int NAMELENGTH, int VALUELENGTH> struct ManagementReply
        {
        private:
            static const int paddingLength = (chunkSize-1)-(
                    sizeof(Header)
                    +2
                    +NAMELENGTH
                    +VALUELENGTH
                    -1)%chunkSize;

            //! Management records header
            Header header;

            //! Length in bytes of name
            uint8_t nameLength;

            //! Length in bytes of value
            uint8_t valueLength;

            //! Name data
            uint8_t name[NAMELENGTH];

            //! Value data
            uint8_t value[VALUELENGTH];

            //! Padding data
            uint8_t padding[paddingLength];

        public:
            //! Construct the record based on the name data and value data
            /*!
             * A full record is constructed from the name-value data. After
             * construction the structure can be casted to raw data and
             * transmitted as is. The size of the data arrays pointed to by
             * name_ and value_ are assumed to correspond with the NAMELENGTH
             * and VALUELENGTH template parameters passed to the class.
             *
             * @param[in] name_ Pointer to name data
             * @param[in] value_ Pointer to value data
             */
            ManagementReply(
                    const char* name_,
                    const char* value_):
                nameLength(NAMELENGTH),
                valueLength(VALUELENGTH)
            {
                std::copy(name_, name_+NAMELENGTH, name);
                std::copy(value_, value_+VALUELENGTH, value);
                header.version = version;
                header.type = RecordType::GET_VALUES_RESULT;
                header.fcgiId = 0;
                header.contentLength = NAMELENGTH+VALUELENGTH;
                header.paddingLength= paddingLength;
            }
        };

        //! The maximum allowed file descriptors open at a time
        extern const ManagementReply<14, 2> maxConnsReply;

        //! The maximum allowed requests at a time
        extern const ManagementReply<13, 2> maxReqsReply;

        //! Where or not requests can be multiplexed over a single connections
        extern const ManagementReply<15, 1> mpxsConnsReply;
    }
}

#endif
