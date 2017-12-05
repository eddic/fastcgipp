/*!
 * @file       message.hpp
 * @brief      Defines the Message data structure.
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

#ifndef MESSAGE_HPP
#define MESSAGE_HPP

#include "fastcgi++/block.hpp"

namespace Fastcgipp
{
    //! Data structure used to pass messages to requests
    /*!
     * This data structure is crucial to all operation in the fastcgi++ library
     * as all data passed to requests must be encapsulated in this data
     * structure.  A type value of 0 means that the message is a FastCGI record
     * and will be processed at a low level by the library. Any other type value
     * and the message will be passed up to the user code to be processed. The
     * data may contain any data that can be serialized into a raw character
     * array.
     */
    struct Message
    {
        Message(const int type_):
            type(type_)
        {}

        Message():
            type(0)
        {}

        Message(Message&& x):
            type(x.type),
            data(std::move(x.data))
        {}

        Message& operator=(Message&& x)
        {
            type=x.type;
            data=std::move(x.data);
            return *this;
        }

        Message(const Message&) =delete;
        Message& operator=(const Message&) =delete;

        //! Type of message. A 0 means FastCGI record. Anything else is open.
        int type;

        //! The raw data being passed along with the message.
        Block data;
    };
}

#endif
