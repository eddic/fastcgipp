/*!
 * @file       protocol.cpp
 * @brief      Defines everything for relating to the FastCGI protocol itself.
 * @author     Eddie Carle &lt;eddie@isatec.ca&gt;
 * @date       May 30, 2018
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

#include "fastcgi++/protocol.hpp"
#include "fastcgi++/config.hpp"

bool Fastcgipp::Protocol::processParamHeader(
        const char* data,
        const char* const dataEnd,
        const char*& name,
        const char*& value,
        const char*& end)
{
    size_t nameSize;
    size_t valueSize;

    if(data>=dataEnd)
        return false;
    if(*data & 0x80)
    {
        const auto size=data;
        data += sizeof(uint32_t);

        if(data>dataEnd)
            return false;

        nameSize=BigEndian<uint32_t>::read(&*size) & 0x7fffffff;
    }
    else
        nameSize=*data++;

    if(data>=dataEnd)
        return false;
    if(*data & 0x80)
    {
        const auto size=data;
        data += sizeof(uint32_t);

        if(data>dataEnd)
            return false;

        valueSize=BigEndian<uint32_t>::read(&*size) & 0x7fffffff;
    }
    else
        valueSize=*data++;

    name = data;
    value = name+nameSize;
    end = value+valueSize;

    if(end>dataEnd)
        return false;
    else
        return true;
}

const Fastcgipp::Protocol::ManagementReply<14, 2>
Fastcgipp::Protocol::maxConnsReply("FCGI_MAX_CONNS", "10");

const Fastcgipp::Protocol::ManagementReply<13, 2>
Fastcgipp::Protocol::maxReqsReply("FCGI_MAX_REQS", "50");

const Fastcgipp::Protocol::ManagementReply<15, 1>
Fastcgipp::Protocol::mpxsConnsReply("FCGI_MPXS_CONNS", "1");

const char Fastcgipp::version[]=FASTCGIPP_VERSION;

size_t Fastcgipp::Protocol::getRecordSize(size_t contentLength)
{
    if(contentLength > 0xffffU)
        contentLength = 0xffffU;

    const size_t recordSize = (contentLength+sizeof(Header)+chunkSize-1)
        /chunkSize * chunkSize;

    return recordSize;
}
