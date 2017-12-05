/*!
 * @file       http.hpp
 * @brief      Declares elements of the HTTP protocol
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

#ifndef FASTCGIPP_HTTP_HPP
#define FASTCGIPP_HTTP_HPP

#include <string>
#include <ostream>
#include <istream>
#include <iterator>
#include <cstring>
#include <algorithm>
#include <map>
#include <vector>
#include <memory>
#include <array>
#include <ctime>
#include <cstring>
#include <atomic>

#include <fastcgi++/protocol.hpp>

//! Topmost namespace for the fastcgi++ library
namespace Fastcgipp
{
    //! Defines classes and functions relating to the HTTP protocol
    namespace Http
    {
        //! Holds a file uploaded from the client
        /*!
         * The actual name associated with the file is omitted from the class
         * so it can be linked in an associative container.
         *
         * @tparam charT Type of character to use in the value string (char or
         *               wchar_t)
         *
         * @date    May 3, 2017
         * @author  Eddie Carle &lt;eddie@isatec.ca&gt;
         */
        template<class charT> struct File
        {
            //! Filename
            std::basic_string<charT> filename;

            //! Content Type
            std::basic_string<charT> contentType;

            //! Size of file
            size_t size;

            //! File data
            mutable std::unique_ptr<char[]> data;

            //! Move constructor
            File(File&& x):
                filename(std::move(x.filename)),
                contentType(std::move(x.contentType)),
                size(x.size),
                data(std::move(x.data))
            {}

            File() {}
        };

        //! The HTTP request method as an enumeration
        enum class RequestMethod
        {
            ERROR=0,
            HEAD=1,
            GET=2,
            POST=3,
            PUT=4,
            DELETE=5,
            TRACE=6,
            OPTIONS=7,
            CONNECT=8
        };

        //! Some textual labels for RequestMethod
        extern const std::array<const char* const, 9> requestMethodLabels;

        template<class charT, class Traits>
        inline std::basic_ostream<charT, Traits>& operator<<(
                std::basic_ostream<charT, Traits>& os,
                const RequestMethod requestMethod)
        {
            return os << requestMethodLabels[static_cast<int>(requestMethod)];
        }

        //! Efficiently stores IPv6 addresses
        /*!
         * This class stores IPv6 addresses as a 128 bit array. It does this as
         * opposed to storing the string itself to facilitate efficient logging
         * and processing of the address. The class possesses full IO and
         * comparison capabilities as well as allowing bitwise AND operations
         * for netmask calculation. It detects when an IPv4 address is stored
         * outputs it accordingly.
         *
         * @date    August 20, 2016
         * @author  Eddie Carle &lt;eddie@isatec.ca&gt;
         */
        class Address
        {
        public:
            //! This is the data length of the IPv6 address
            static const size_t size=16;

            //! Data representation of the IPv6 address
            std::array<unsigned char, size> m_data;

            //! Assign the IPv6 address from a data array
            /*!
             * @param[in] data Pointer to a 16 byte array
             */
            Address operator=(const unsigned char* data)
            {
                std::copy(data, data+m_data.size(), m_data.begin());
                return *this;
            }

            Address operator=(const Address& address)
            {
                std::copy(
                        address.m_data.begin(),
                        address.m_data.end(),
                        m_data.begin());
                return *this;
            }

            Address(const Address& address)
            {
                std::copy(
                        address.m_data.begin(),
                        address.m_data.end(),
                        m_data.begin());
            }

            //! Initializes an all zero address
            Address()
            {
                zero();
            }

            //! Construct the IPv6 address from a data array
            /*!
             * @param[in] data Pointer to a 16 byte array
             */
            explicit Address(const unsigned char* data)
            {
                std::copy(data, data+m_data.size(), m_data.begin());
            }

            //! Assign the IP address from a string of characters
            /*!
             * In order for this to work the string must represent either an
             * IPv4 address in standard textual decimal form (127.0.0.1) or an
             * IPv6 in standard form.
             *
             * @param[in] start First character of the string
             * @param[in] end Last character of the string + 1
             * @tparam charT Character type.
             */
            template<class charT> void assign(
                    const charT* start,
                    const charT* end);

            bool operator==(const Address& x) const
            {
                return std::equal(
                        m_data.cbegin(),
                        m_data.cend(),
                        x.m_data.cbegin());
            }

            bool operator<(const Address& x) const
            {
                return std::memcmp(m_data.data(), x.m_data.data(), size)<0;
            }

            //! Returns false if the ip address is zeroed. True otherwise
            operator bool() const;

            Address operator&(const Address& x) const;

            Address& operator&=(const Address& x);

            //! Set all bits to zero in IP address
            void zero()
            {
                m_data.fill(0);
            }
        };

        //! Address stream insertion operation
        /*!
         * This stream inserter obeys all stream manipulators regarding
         * alignment, field width and numerical base.
         */
        template<class charT, class Traits>
        std::basic_ostream<charT, Traits>& operator<<(
                std::basic_ostream<charT, Traits>& os,
                const Address& address);

        //! Address stream extractor operation
        /*!
         * In order for this to work the string must represent either an IPv4
         * address in standard textual decimal form (127.0.0.1) or an IPv6 in
         * standard form.
         */
        template<class charT, class Traits>
        std::basic_istream<charT, Traits>& operator>>(
                std::basic_istream<charT, Traits>& is,
                Address& address);

        //! Data structure of HTTP environment data
        /*!
         * This structure contains all HTTP environment data for each
         * individual request. The data is processed from FastCGI parameter
         * records.
         *
         * @tparam charT Character type to use for strings
         *
         * @date    May 3, 2017
         * @author  Eddie Carle &lt;eddie@isatec.ca&gt;
         */
        template<class charT> struct Environment
        {
            //! Hostname of the server
            std::basic_string<charT> host;

            //! User agent string
            std::basic_string<charT> userAgent;

            //! Content types the client accepts
            std::basic_string<charT> acceptContentTypes;

            //! Languages the client accepts
            std::vector<std::string> acceptLanguages;

            //! Character sets the clients accepts
            std::basic_string<charT> acceptCharsets;
	  
            //! Http authorization string
            std::basic_string<charT> authorization;
	  
            //! Referral URL
            std::basic_string<charT> referer;

            //! Content type of data sent from client
            std::basic_string<charT> contentType;

            //! HTTP root directory
            std::basic_string<charT> root;

            //! Filename of script relative to the HTTP root directory
            std::basic_string<charT> scriptName;

            //! REQUEST_METHOD
            RequestMethod requestMethod;

            //! REQUEST_URI
            std::basic_string<charT> requestUri;

            //! Path information
            std::vector<std::basic_string<charT>> pathInfo;

            //! The etag the client assumes this document should have
            unsigned etag;

            //! How many seconds the connection should be kept alive
            unsigned keepAlive;

            //! Length of content to be received from the client (post data)
            unsigned contentLength;

            //! IP address of the server
            Address serverAddress;

            //! IP address of the client
            Address remoteAddress;

            //! TCP port used by the server
            uint16_t serverPort;

            //! TCP port used by the client
            uint16_t remotePort;

            //! Timestamp the client has for this document
            std::time_t ifModifiedSince;

            //! Container with all url-encoded cookie data
            std::multimap<
                std::basic_string<charT>,
                std::basic_string<charT>> cookies;

            //! Container with all url-encoded GET data
            std::multimap<
                std::basic_string<charT>,
                std::basic_string<charT>> gets;

            //! Container of none-file POST data
            std::multimap<
                std::basic_string<charT>,
                std::basic_string<charT>> posts;

            //! Container of file POST data
            std::multimap<
                std::basic_string<charT>,
                File<charT>> files;

            //! Parses FastCGI parameter data into the data structure
            /*!
             * This function will take the body of a FastCGI parameter record
             * and parse the data into the data structure. data should equal
             * the first character of the records body with size being it's
             * content length.
             *
             * @param[in] data Start of parameter data
             * @param[in] dataEnd 1+ the last byte of parameter data
             */
            void fill(
                    const char* data,
                    const char* dataEnd);

            //! Consolidates POST data into a single buffer
            /*!
             * This function will take arbitrarily divided chunks of raw http
             * post data and consolidate them into m_postBuffer.
             *
             * @param[in] start Start of post data.
             * @param[in] end 1+ the last byte of post data
             */
            void fillPostBuffer(
                    const char* start,
                    const char* end);

            //! Attempts to parse the POST buffer
            /*!
             * If the content type is recognized, this function will parse the
             * post buffer and return true. If it isn't recognized, it will
             * return false.
             *
             * @return True if successfully parsed. False otherwise.
             */
            bool parsePostBuffer();

            //! Get the post buffer
            const std::vector<char>& postBuffer() const
            {
                return m_postBuffer;
            }

            //! Clear the post buffer
            void clearPostBuffer()
            {
                m_postBuffer.clear();
                m_postBuffer.shrink_to_fit();
            }

            Environment():
                requestMethod(RequestMethod::ERROR),
                etag(0),
                keepAlive(0),
                contentLength(0),
                serverPort(0),
                remotePort(0),
                ifModifiedSince(0)
            {}
        private:
            //! Parses "multipart/form-data" http post data
            inline void parsePostsMultipart();

            //! Parses "application/x-www-form-urlencoded" post data
            inline void parsePostsUrlEncoded();

            //! Raw string of characters representing the post boundary
            std::vector<char> boundary;

            //! Buffer for processing post data
            std::vector<char> m_postBuffer;
        };

        //! Convert a char array to a std::wstring
        /*!
         * @param[in] start First byte in char array
         * @param[in] end 1+ last byte of the array (no null terminator)
         * @param[out] string Reference to the wstring that should be modified
         */
        void vecToString(
                const char* start,
                const char* end,
                std::wstring& string);

        //! Convert a char string to a std::string
        /*!
         * @param[in] start First byte in char string
         * @param[in] end 1+ last byte of the string (no null terminator)
         * @param[out] string Reference to the string that should be modified
         */
        inline void vecToString(
                const char* start,
                const char* end,
                std::string& string)
        {
            string.assign(start, end);
        }

        //! Convert a char string to an integer
        /*!
         * This function is very similar to std::atoi() except that it takes
         * start/end values of a non null terminated char string instead of a
         * null terminated string. The first character must be either a number
         * or a minus sign (-). As soon as the end is reached or a non
         * numerical character is reached, the result is tallied and returned.
         *
         * @param[in] start Pointer to the first byte in the string
         * @param[in] end Pointer to the last byte in the string + 1
         * @return Integer value represented by the string
         * @tparam charT Character type
         */
        template<class charT> int atoi(const charT* start, const charT* end);

        //! Convert a char string to a float
        /*!
         * This function is very similar to std::atof() except that it takes
         * start/end values of a non null terminated char string instead of a
         * null terminated string. The first character must be either a number
         * or a minus sign (-). As soon as the end is reached or a non
         * numerical character is reached, the result is tallied and returned.
         *
         * @param[in] start Pointer to the first byte in the string
         * @param[in] end Pointer to the last byte in the string + 1
         * @return Float value represented by the string
         * @tparam charT Character type
         */
        template<class charT> float atof(const charT* start, const charT* end);

        //! Decodes a url-encoded string into a multimap container
        /*!
         * @param[in] data Data to decode
         * @param[in] dataEnd +1 last byte to decode
         * @param[out] output Container to output data into
         * @param[in] fieldSeparator String that signifies field separation
         */
        template<class charT> void decodeUrlEncoded(
                const char* data,
                const char* dataEnd,
                std::multimap<
                    std::basic_string<charT>,
                    std::basic_string<charT>>& output,
                const char* const fieldSeparator="&");

        //! Convert a string with percent escaped byte values to their values
        /*!
         * Since converting a percent escaped string to actual values can only
         * make it shorter, it is safe to assume that the return value will
         * always be smaller than size. It is thereby a safe move to make the
         * destination block of memory the same size as the source.
         *
         * @param[in] start Iterator to the first character in the percent
         *                  escaped string
         * @param[in] end Iterator to +1 the last character in the percent
         *                escaped string
         * @param[out] destination Pointer to the section of memory to write the
         *                         converted string to
         * @return Iterator to +1 the last character written
         */
        char* percentEscapedToRealBytes(
                const char* start,
                const char* end,
                char* destination);

        //! List of characters in order for Base64 encoding.
        extern const std::array<const char, 64> base64Characters;

        //! Convert a binary container of data to a Base64 encoded container.
        /*!
         * If destination is a fixed size container, it should have a size of
         * at least ((end-start-1)/3 + 1)*4 not including null terminators if
         * used and assuming integer arithmetic.
         *
         * @param[in] start Iterator to start of binary data.
         * @param[in] end Iterator to end of binary data.
         * @param[out] destination Iterator to start of Base64 destination.
         *
         * @tparam In Input iterator type. Should be dereferenced to type char.
         * @tparam Out Output iterator type. Should be dereferenced to type
         *             char.
         *
         * @return Iterator to last position written+1 (The normal end()
         *                  iterator).
         */
        template<class In, class Out>
        Out base64Encode(In start, In end, Out destination);

        //! Convert a Base64 encoded container to a binary container.
        /*!
         * If destination is a fixed size container, it should have a size of
         * at least (end-start)*3/4 not including null terminators if used.
         *
         * @param[in] start Iterator to start of Base64 data.
         * @param[in] end Iterator to end of Base64 data.
         * @param[out] destination Iterator to start of binary destination.
         *
         * @tparam In Input iterator type. Should be dereferenced to type char.
         * @tparam Out Output iterator type. Should be dereferenced to type
         *             char.
         *
         * @return Iterator to last position written+1 (The normal end()
         *                  iterator). If the return value equals destination,
         *                  an error occurred.
         */
        template<class In, class Out>
        Out base64Decode(In start, In end, Out destination);

        //! Defines ID values for HTTP sessions.
        /*!
         * @date    March 24, 2016
         * @author  Eddie Carle &lt;eddie@isatec.ca&gt;
         */
        class SessionId
        {
        public:
            //! Size in bytes of the ID data. Make sure it is a multiple of 3.
            static const size_t size=15;

            //! Size in characters of string representation
            static const size_t stringLength=size*4/3;

        private:
            //! ID data
            std::array<unsigned char, size> m_data;

            //! Contains the time this session was last used
            mutable std::time_t m_timestamp;

            //! Resets the last access timestamp to the current time.
            void refresh() const
            {
                m_timestamp = std::time(nullptr);
            }

            template<class T> friend class Sessions;
        public:
            //! This constructor initializes the ID data to a random value
            SessionId();

            SessionId(const SessionId& x):
                m_timestamp(x.m_timestamp)
            {
                std::copy(x.m_data.begin(), x.m_data.end(), m_data.begin());
            }

            //! Initialize the ID data with a base64 encoded string
            /*!
             * Note that only stringLength characeters will be read from the
             * string.
             *
             * @param[in] string Reference to base64 encoded string
             */
            template<class charT>
            SessionId(const std::basic_string<charT>& string);

            template<class charT, class Traits>
            friend std::basic_ostream<charT, Traits>& operator<<(
                    std::basic_ostream<charT, Traits>& os,
                    const SessionId& x);

            bool operator<(const SessionId& x) const
            {
                return std::memcmp(
                        m_data.data(),
                        x.m_data.data(),
                        SessionId::size)<0;
            }

            bool operator==(const SessionId& x) const
            {
                return std::memcmp(
                        m_data.data(),
                        x.m_data.data(),
                        SessionId::size)==0;
            }
        };

        //! Output the ID data in base64 encoding
        template<class charT, class Traits>
        std::basic_ostream<charT, Traits>& operator<<(
                std::basic_ostream<charT, Traits>& os,
                const SessionId& x)
        {
            base64Encode(
                    x.m_data.begin(),
                    x.m_data.end(),
                    std::ostream_iterator<charT, charT, Traits>(os));
            return os;
        }

        //! Container for HTTP sessions
        /*!
         * In many ways this class behaves like an std::map. Additions include
         * a mechanism for clearing out expired sessions based on a keep alive
         * time and a frequency of deletion, and full thread safety. Basically
         * it contains all session data and associates it with ID values.
         *
         * Session data is only available as constant in order to ensure thread
         * safety when accessing the data. It is not only possible, but very
         * probable, that multiple requests/threads will be accessing the same
         * session data simultaneously.
         *
         * @tparam T Class containing session data.
         *
         * @date    May 26, 2016
         * @author  Eddie Carle &lt;eddie@isatec.ca&gt;
         */
        template<class T> class Sessions
        {
        private:
            //! Amount of seconds to keep sessions around for.
            const unsigned int m_keepAlive;

            //! The time that the next session cleanup should be done.
            std::time_t m_cleanupTime;

            //! Actual container of sessions
            std::map<SessionId, std::shared_ptr<const T>> m_sessions;

            //! Thread safe all operations
            mutable std::mutex m_mutex;

            //! Length of expiration string (with null terminator)
            static const size_t expirationLength = 30;

            //! Internal string for cookie expirations
            char m_expiration[2][expirationLength];

            //! Point for the current expiration string
            std::atomic<const char*> m_expirationPtr;

            //! Internal helper for building the m_expiration string
            void setExpiration();

        public:
            //! Constructor takes session keep alive times
            /*!
             * @param[in] keepAlive Amount of seconds a session will stay alive
             *                      for.
             */
            Sessions(unsigned int keepAlive):
                m_keepAlive(keepAlive),
                m_cleanupTime(std::time(nullptr)+keepAlive)
            {
                setExpiration();
            }

            //! Get session data from session ID
            /*!
             * @param[in] id The session ID we are looking for.
             * @return Shared pointer to session data. The pointer will evaluate
             *         to false if the session does not actually exist.
             */
            std::shared_ptr<const T> get(const SessionId& id);

            //! How many active sessions are there?
            size_t size() const
            {
                std::lock_guard<std::mutex> lock(m_mutex);
                return m_sessions.size();
            }

            //! Generates a new session
            /*!
             * @param[in] data Data to store in the session.
             *
             * @return A session ID for the session. This is not a reference for
             * thread safety purposes.
             */
            SessionId generate(const std::shared_ptr<const T>& data);

            //! Erase a session
            /*!
             * @param[in] id The session we want to erase.
             */
            void erase(const SessionId& id)
            {
                std::lock_guard<std::mutex> lock(m_mutex);
                m_sessions.erase(id);
            }

            //! Expiration string for setting cookies
            const char* expiration() const
            {
                return m_expirationPtr;
            }
        };
    }
}

template<class In, class Out>
Out Fastcgipp::Http::base64Decode(In start, In end, Out destination)
{
    Out dest=destination;

    for(int buffer=0, bitPos=-8, padStart=0; start!=end || bitPos>-6; ++dest)
    {
        if(bitPos==-8)
        {
            bitPos=18;
            padStart=-9;
            buffer=0;
            while(bitPos!=-6)
            {
                if(start==end) return destination;
                int value=*start++;
                if(value >= 'A' && 'Z' >= value) value -= 'A';
                else if(value >= 'a' && 'z' >= value) value -= 'a' - 26;
                else if(value >= '0' && '9' >= value) value -= '0' - 52;
                else if(value == '+') value = 62;
                else if(value == '/') value = 63;
                else if(value == '=') { padStart=bitPos; break; }
                else return destination;

                buffer |= value << bitPos;
                bitPos-=6;
            }
            bitPos=16;
        }

        *dest = (buffer >> bitPos) & 0xff;
        bitPos-=8;
        if(padStart>=bitPos)
        {
            if( (padStart-bitPos)/6 )
                return dest;
            else
                return ++dest;
        }
    }

    return dest;
}

template<class In, class Out>
Out Fastcgipp::Http::base64Encode(In start, In end, Out destination)
{
    for(int buffer, bitPos=-6, padded; start!=end || bitPos>-6; ++destination)
    {
        if(bitPos==-6)
        {
            bitPos=16;
            buffer=0;
            padded=-6;
            while(bitPos!=-8)
            {
                if(start!=end)
                    buffer |= static_cast<int>(
                            *reinterpret_cast<const unsigned char*>(
                                start++)) << bitPos;
                else padded+=6;
                bitPos-=8;
            }
            bitPos=18;
        }

        if(padded == bitPos)
        {
            *destination='=';
            padded-=6;
        }
        else *destination=base64Characters[ (buffer >> bitPos)&0x3f ];
        bitPos -= 6;
    }

    return destination;
}

template<class T> Fastcgipp::Http::SessionId
Fastcgipp::Http::Sessions<T>::generate(const std::shared_ptr<const T>& data)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    std::pair<
            typename std::map<SessionId, std::shared_ptr<const T>>::iterator,
            bool>
        retVal;
    retVal.second=false;
    while(!retVal.second)
        retVal=m_sessions.insert(std::pair<SessionId, std::shared_ptr<const T>>(
                    SessionId(),
                    data));
    return retVal.first->first;
}

template<class T> std::shared_ptr<const T>
Fastcgipp::Http::Sessions<T>::get(const SessionId& id)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    const std::time_t now = std::time(nullptr);
    const std::time_t oldest(now-m_keepAlive);

    if(now >= m_cleanupTime)
    {
        auto session = m_sessions.begin();
        while(session != m_sessions.end())
        {
            if(session->first.m_timestamp < oldest)
                session = m_sessions.erase(session);
            else
                ++session;
        }
        m_cleanupTime = std::time(nullptr)+m_keepAlive;
        setExpiration();
    }

    const auto session = m_sessions.find(id);
    if(session != m_sessions.cend())
    {
        if(session->first.m_timestamp < oldest)
            m_sessions.erase(session);
        else
        {
            session->first.refresh();
            return session->second;
        }
    }

    return std::shared_ptr<const T>();
}

template<class T> void Fastcgipp::Http::Sessions<T>::setExpiration()
{
    char* const newExpiration(
        m_expirationPtr==m_expiration[0]?m_expiration[1]:m_expiration[0]);
    const std::time_t expirationTime = m_cleanupTime + m_keepAlive;
    const auto count = std::strftime(
            newExpiration,
            expirationLength,
            "%a, %d %b %Y %H:%M:%S GMT",
            std::gmtime(&expirationTime));
    std::fill(newExpiration+count, newExpiration+expirationLength, 0);
    m_expirationPtr = newExpiration;
}

#endif
