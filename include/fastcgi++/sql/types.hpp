/*!
 * @file       types.hpp
 * @brief      Declares %SQL data types
 * @author     Eddie Carle &lt;eddie@isatec.ca&gt;
 * @date       February 16, 2022
 * @copyright  Copyright &copy; 2022 Eddie Carle. This project is released under
 *             the GNU Lesser General Public License Version 3.
 */

/*******************************************************************************
* Copyright (C) 2022 Eddie Carle [eddie@isatec.ca]                             *
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

#ifndef FASTCGIPP_SQL_TYPES_HPP
#define FASTCGIPP_SQL_TYPES_HPP

#include <chrono>
#include <string>
#include <vector>
#include "fastcgi++/address.hpp"
#include "fastcgi++/endian.hpp"

//! Topmost namespace for the fastcgi++ library
namespace Fastcgipp
{
    //! Contains all fastcgi++ %SQL facilities
    namespace SQL
    {
        typedef bool BOOL;
        typedef int16_t SMALLINT;
        typedef int32_t INTEGER;
        typedef int64_t BIGINT;
        typedef float REAL;
        typedef double DOUBLE_PRECISION;
        typedef std::string TEXT;
        typedef std::wstring WTEXT;
        typedef std::chrono::time_point<
                std::chrono::system_clock,
                std::chrono::days> DATE;
        typedef std::chrono::time_point<
                std::chrono::system_clock,
                std::chrono::microseconds> TIMESTAMPTZ;
        typedef Fastcgipp::Address INET;
        template<class T> using ARRAY = std::vector<T>;
        typedef ARRAY<char> BYTEA;
        typedef BigEndian<int32_t> ARRAY_SIZE;
    }
}

#endif
