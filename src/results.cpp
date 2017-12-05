/*!
 * @file       results.cpp
 * @brief      Defines SQL results types
 * @author     Eddie Carle &lt;eddie@isatec.ca&gt;
 * @date       May 13, 2017
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

#include "fastcgi++/sql/results.hpp"

Fastcgipp::SQL::Status Fastcgipp::SQL::Results_base::status() const
{
    if(m_res == nullptr)
        return Status::noResult;

    switch(PQresultStatus(m_res))
    {
        case PGRES_EMPTY_QUERY:
            return Status::emptyQuery;
        case PGRES_COMMAND_OK:
            return Status::commandOk;
        case PGRES_TUPLES_OK:
            return Status::rowsOk;
        case PGRES_COPY_OUT:
            return Status::copyOut;
        case PGRES_COPY_IN:
            return Status::copyIn;
        case PGRES_BAD_RESPONSE:
            return Status::badResponse;
        case PGRES_NONFATAL_ERROR:
            return Status::nonfatalError;
        case PGRES_COPY_BOTH:
            return Status::copyBoth;
        case PGRES_SINGLE_TUPLE:
            return Status::singleTuple;
        default:
            return Status::fatalError;
    };
}
