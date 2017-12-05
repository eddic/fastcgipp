/*!
 * @file       parameters.cpp
 * @brief      Defines SQL parameters types
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

#include "fastcgi++/sql/parameters.hpp"

#include <locale>
#include <codecvt>

void Fastcgipp::SQL::Parameters_base::build()
{
    const size_t columns = size();
    m_raws.clear();
    m_raws.reserve(columns);
    m_sizes.clear();
    m_sizes.reserve(columns);
    build_impl();
}

Fastcgipp::SQL::Parameter<std::string>&
Fastcgipp::SQL::Parameter<std::string>::operator=(const std::wstring& x)
{
    std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;
    try
    {
        m_data = converter.to_bytes(x);
    }
    catch(const std::range_error& e)
    {
        WARNING_LOG("Error in code conversion to utf8 in SQL parameter")
    }
    return *this;
}
