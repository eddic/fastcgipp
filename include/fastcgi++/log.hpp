/*!
 * @file       log.hpp
 * @brief      Declares the Fastcgipp debugging/logging facilities
 * @author     Eddie Carle &lt;eddie@isatec.ca&gt;
 * @date       May 4, 2017
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

#ifndef FASTCGIPP_LOG_HPP
#define FASTCGIPP_LOG_HPP

#include "fastcgi++/config.hpp"

#include <ostream>
#include <mutex>
#include <cstdlib>
#include <string>

//! Topmost namespace for the fastcgi++ library
namespace Fastcgipp
{
    //! Contains the Fastcgipp debugging/logging mechanism
    namespace Logging
    {
        //! The actual stream we will be logging to.
        extern std::wostream* logstream;

        //! Thread safe the logging mechanism
        extern std::mutex mutex;

        //! Hostname to use in logging
        extern std::wstring hostname;

        //! Application name to use in loggin
        extern std::wstring program;

        //! Set to true if you want to suppress non-error logs
        extern bool suppress;

        //! Communicate the log level to the header generator
        enum Level
        {
            INFO = 0,
            FAIL = 1,
            ERROR = 2,
            WARNING = 3,
            DEBUG = 4,
            DIAG = 5,
        };

        //! Send a log header to logstream
        void header(Level level);
    }
}

//! This is for the user to log whatever they want.
#define INFO_LOG(data) {\
    if(!::Fastcgipp::Logging::suppress)\
    { \
        std::lock_guard<std::mutex> lock(::Fastcgipp::Logging::mutex);\
        ::Fastcgipp::Logging::header(::Fastcgipp::Logging::INFO);\
        *::Fastcgipp::Logging::logstream << data << std::endl;\
    }}

//! Log any "errors" that cannot be recovered from and then exit.
/*!
 * This should encompass all errors that should \a never happen, regardless of
 * what the external conditions are. This also presumes that there is no
 * possibility to recover from the error and we should simply terminate.
 */
#define FAIL_LOG(data) {\
    if(!::Fastcgipp::Logging::suppress)\
    { \
        std::lock_guard<std::mutex> lock(::Fastcgipp::Logging::mutex);\
        ::Fastcgipp::Logging::header(::Fastcgipp::Logging::FAIL);\
        *::Fastcgipp::Logging::logstream << data << std::endl;\
        std::exit(EXIT_FAILURE);\
    }}

#if FASTCGIPP_LOG_LEVEL > 0
//! Log any "errors" that can be recovered from.
/*!
 * This should encompass all errors that should \a never happen, regardless of
 * what the external conditions are. This also presumes that there is a
 * mechanism to recover from said error.
 */
#define ERROR_LOG(data) \
    { \
        std::lock_guard<std::mutex> lock(::Fastcgipp::Logging::mutex);\
        ::Fastcgipp::Logging::header(::Fastcgipp::Logging::ERROR);\
        *::Fastcgipp::Logging::logstream << data << std::endl;\
    }
#else
#define ERROR_LOG(data) {}
#endif

#if FASTCGIPP_LOG_LEVEL > 1
//! Log any externally caused "errors"
/*!
 * We allocate this "log level" to errors that are caused by external factors
 * like bad data from a web server. Once can't say that these errors should
 * never happen since they are external controlled.
 */
#define WARNING_LOG(data) {\
    if(!::Fastcgipp::Logging::suppress)\
    { \
        std::lock_guard<std::mutex> lock(::Fastcgipp::Logging::mutex);\
        ::Fastcgipp::Logging::header(::Fastcgipp::Logging::WARNING);\
        *::Fastcgipp::Logging::logstream << data << std::endl;\
    }}
#else
#define WARNING_LOG(data) {}
#endif

#if FASTCGIPP_LOG_LEVEL > 2
//! The intention here is for general debug/analysis logging
#define DEBUG_LOG(data) {\
    if(!::Fastcgipp::Logging::suppress)\
    { \
        std::lock_guard<std::mutex> lock(::Fastcgipp::Logging::mutex);\
        ::Fastcgipp::Logging::header(::Fastcgipp::Logging::DEBUG);\
        *::Fastcgipp::Logging::logstream << data << std::endl;\
    }}
#else
#define DEBUG_LOG(data) {}
#endif

#if FASTCGIPP_LOG_LEVEL > 3
//! The intention here is for internal library debug/analysis logging
#define DIAG_LOG(data) {\
    if(!::Fastcgipp::Logging::suppress)\
    { \
        std::lock_guard<std::mutex> lock(::Fastcgipp::Logging::mutex);\
        ::Fastcgipp::Logging::header(::Fastcgipp::Logging::DIAG);\
        *::Fastcgipp::Logging::logstream << data << std::endl;\
    }}
#else
#define DIAG_LOG(data) {}
#endif

#endif
