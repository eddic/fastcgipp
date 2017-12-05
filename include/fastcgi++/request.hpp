/*!
 * @file       request.hpp
 * @brief      Declares the Request class
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

#ifndef FASTCGIPP_REQUEST_HPP
#define FASTCGIPP_REQUEST_HPP

#include "fastcgi++/protocol.hpp"
#include "fastcgi++/fcgistreambuf.hpp"
#include "fastcgi++/http.hpp"

#include <ostream>
#include <functional>
#include <queue>
#include <mutex>

//! Topmost namespace for the fastcgi++ library
namespace Fastcgipp
{
    //! De-templating base class for Request
    class Request_base
    {
    public:
        //! Request Handler
        /*!
         * This function is called by Manager::handler() to handle messages
         * destined for the request.  It deals with FastCGI messages (type=0)
         * while passing all other messages off to response().
         *
         * @return A lock locking the requests message queue. If the request
         *         completes this will be unlocked. If the request isn't
         *         complete it will be locked. If locked, makes sure to unlock
         *         it \e after unlocking Request::mutex.
         * @sa callback
         */
        virtual std::unique_lock<std::mutex> handler() =0;

        virtual ~Request_base() {}

        //! Only one thread is allowed to handle the request at a time
        std::mutex mutex;

        //! Send a message to the request
        inline void push(Message&& message)
        {
            std::lock_guard<std::mutex> lock(m_messagesMutex);
            m_messages.push(std::move(message));
        }

    protected:
        //! A queue of message for the request
        std::queue<Message> m_messages;

        //! Thread safe our message queue
        std::mutex m_messagesMutex;
    };

    //! %Request handling class
    /*!
     * Derivations of this class will handle requests. This includes building
     * the environment data, processing post/get data, fetching data (files,
     * database), and producing a response.  Once all client data is organized,
     * response() will be called.  At minimum, derivations of this class must
     * define response().
     *
     * If you want to use utf8 encoding pass wchar_t as the template argument
     * and use wide character unicode internally for everything. If you want to
     * use a 8bit character set pass char as the template argument and use char
     * for everything internally.
     *
     * @tparam charT Character type for internal processing (wchar_t or char)
     *
     * @date    May 3, 2017
     * @author  Eddie Carle &lt;eddie@isatec.ca&gt;
     */
    template<class charT> class Request: public Request_base
    {
    public:
        //! Initializes what it can. configure() to finish.
        /*!
         * @param maxPostSize This would be the maximum size, in bytes, you want
         *                    to allow for post data. Any data beyond this size
         *                    would result in a call to bigPostErrorHandler().
         *                    With the default being 0, POST uploading is by
         *                    *default* prohibited. Should you wish to have the
         *                    limit as large as possible, pass either
         *                    (size_t)-1, std::string::npos or
         *                    std::numeric_limits<size_t>::max().
         */
        Request(const size_t maxPostSize=0):
            out(&m_outStreamBuffer),
            err(&m_errStreamBuffer),
            m_maxPostSize(maxPostSize),
            m_state(Protocol::RecordType::PARAMS),
            m_status(Protocol::ProtocolStatus::REQUEST_COMPLETE)
        {
            out.imbue(std::locale("C"));
            err.imbue(std::locale("C"));
        }

        //! Configures the request with the data it needs.
        /*!
         * This function is an "after-the-fact" constructor that build vital
         * initial data for the request.
         *
         * @param[in] id Complete ID of the request
         * @param[in] role The role that the other side expects this request to4
         *                 play
         * @param[in] kill Boolean value indicating whether or not the socket
         *                 should be closed upon completion
         * @param[in] send Function for sending data out of the stream buffers
         * @param[in] callback Callback function capable of passing messages to
         *                     the request
         */
        void configure(
                const Protocol::RequestId& id,
                const Protocol::Role& role,
                bool kill,
                const std::function<void(const Socket&, Block&&, bool)>
                    send,
                const std::function<void(Message)> callback);

        std::unique_lock<std::mutex> handler();

        virtual ~Request() {}

    protected:
        //! Accessor for the HTTP environment data
        const Http::Environment<charT>& environment() const
        {
            return m_environment;
        }

        //! Standard output stream to the client
        std::basic_ostream<charT> out;

        //! Output stream to the HTTP server error log
        std::basic_ostream<charT> err;

        //! Called when a processing error occurs
        /*!
         * This function is called whenever a processing error happens inside
         * the request. By default it will send a standard 500 Internal Server
         * Error message to the user.  Override for more specialized purposes.
         */
        virtual void errorHandler();

        //! Called when too much post data is recieved.
        /*!
         * This function is called when the client sends too much data the
         * request. By default it will send a standard 413 Request Entity Too
         * Large Error message to the user.  Override for more specialized
         * purposes.
         */
        virtual void bigPostErrorHandler();

        //! See the requests role
        Protocol::Role role() const
        {
            return m_role;
        }

        //! Callback function for dealings outside the fastcgi++ library
        /*!
         * The purpose of the callback function is to provide a thread safe
         * mechanism for functions and classes outside the fastcgi++ library to
         * talk to the requests. Should the library wish to have another thread
         * process or fetch some data, that thread can call this function when
         * it is finished. It is equivalent to this:
         *
         * void callback(Message msg);
         *
         * The sole parameter is a Message that contains both a type value for
         * processing by response() and a Block for some data.
         */
        const std::function<void(Message)>& callback() const
        {
            return m_callback;
        }

        //! Response generator
        /*!
         * This function is called by handler() once all request data has been
         * received from the other side or if a Message not of a FastCGI type
         * has been passed to it. The function shall return true if it has
         * completed the response and false if it has not (waiting for a
         * callback message to be sent).
         *
         * @return Boolean value indication completion (true means complete)
         * @sa callback
         */
        virtual bool response() =0;

        //! Generate a data input response
        /*!
         * This function exists should the library user wish to do something
         * like generate a partial response based on bytes received from the
         * client. The function is called by handler() every time a FastCGI IN
         * record is received.  The function has no access to the data, but
         * knows exactly how much was received based on the value that was
         * passed. Note this value represents the amount of data received in
         * the individual record, not the total amount received in the
         * environment. If the library user wishes to have such a value they
         * would have to keep a tally of all size values passed.
         *
         * @param[in] bytesReceived Amount of bytes received in this FastCGI
         *                          record
         */
        virtual void inHandler(int bytesReceived)
        {}

        //! Process custom POST data
        /*!
         * Override this function should you wish to process non-standard post
         * data. The library will on it's own process post data of the types
         * "multipart/form-data" and "application/x-www-form-urlencoded". To
         * use this function, your raw post data is fully assembled in
         * environment().postBuffer() and the type string is stored in
         * environment().contentType. Should the content type be what you're
         * looking for and you've processed it, simply return true. Otherwise
         * return false.  Do not worry about freeing the data in the post
         * buffer. Should you return false, the system will try to internally
         * process it.
         *
         * @return Return true if you've processed the data.
         */
        bool virtual inProcessor()
        {
            return false;
        }

        //! The message associated with the current handler() call.
        /*!
         * This is only of use to the library user when a non FastCGI (type=0)
         * Message is passed by using the requests callback.
         *
         * @sa callback
         */
        Message m_message;

        //! Dumps raw data directly into the FastCGI protocol
        /*!
         * This function exists as a mechanism to dump raw data out the stream
         * bypassing the stream buffer or any code conversion mechanisms. If the
         * user has any binary data to send, this is the function to do it with.
         *
         * @param[in] data Pointer to first byte of data to send
         * @param[in] size Size in bytes of data to be sent
         */
        void dump(const char* data, size_t size)
        {
            m_outStreamBuffer.dump(data, size);
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
        void dump(const unsigned char* data, size_t size)
        {
            m_outStreamBuffer.dump(reinterpret_cast<const char*>(data), size);
        }

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
        void dump(std::basic_istream<char>& stream)
        {
            m_outStreamBuffer.dump(stream);
        }

        //! Pick a locale
        /*!
         * Basically this finds the first language in
         * environment().acceptLanguages that matches a locale in the container
         * passed as a parameter. It returns the index in the parameters that
         * points to the chosen locale.
         */
        unsigned pickLocale(const std::vector<std::string>& locales);

        //! Set the output stream's locale
        void setLocale(const std::string& locale);

    private:
        //! The callback function for dealings outside the fastcgi++ library
        /*!
         * The purpose of the callback object is to provide a thread safe
         * mechanism for functions and classes outside the fastcgi++ library to
         * talk to the requests. Should the library wish to have another thread
         * process or fetch some data, that thread can call this function when
         * it is finished. It is equivalent to this:
         *
         * void callback(Message msg);
         *
         * The sole parameter is a Message that contains both a type value for
         * processing by response() and the raw castable data.
         */
        std::function<void(Message)> m_callback;

        //! The data structure containing all HTTP environment data
        Http::Environment<charT> m_environment;

        //! The maximum amount of post data, in bytes, that can be recieved
        const size_t m_maxPostSize;

        //! The role that the other side expects this request to play
        Protocol::Role m_role;

        //! The complete ID (request id & file descriptor) associated with the request
        Protocol::RequestId m_id;

        //! Should the socket be closed upon completion.
        bool m_kill;

        //! What the request is current doing
        Protocol::RecordType m_state;

        //! Generates an END_REQUEST FastCGI record
        void complete();

        //! Function to actually send the record
        std::function<void(const Socket&, Block&&, bool kill)> m_send;

        //! Status to end the request with
        Protocol::ProtocolStatus m_status;

        //! Stream buffer for the out stream
        FcgiStreambuf<charT> m_outStreamBuffer;

        //! Stream buffer for the err stream
        FcgiStreambuf<charT> m_errStreamBuffer;

        //! Codepage
        inline const char* codepage() const;
    };
}

#endif
