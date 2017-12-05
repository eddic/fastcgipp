/*!
 * @file       block.cpp
 * @brief      Defines the Block data structure.
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

#include "fastcgi++/block.hpp"
#include <algorithm>

void Fastcgipp::Block::reserve(size_t x)
{
    if(x != m_reserve)
    {
        std::unique_ptr<char[]> data(new char[x]);
        size_t newSize = std::min(m_size, x);
        std::copy(
                m_data.get(),
                m_data.get()+newSize,
                data.get());

        m_reserve = x;
        m_size = newSize;
        m_data = std::move(data);

    }
}

Fastcgipp::Block::Block():
    m_reserve(0),
    m_size(0)
{}

Fastcgipp::Block::Block(const size_t size_):
    m_reserve(size_),
    m_size(size_),
    m_data(new char[size_])
{}

Fastcgipp::Block::Block(const char* const data, const size_t size_):
    m_reserve(size_),
    m_size(size_),
    m_data(new char[size_])
{
    std::copy(data, data+size_, m_data.get());
}

Fastcgipp::Block::Block(Block&& x):
    m_reserve(x.m_reserve),
    m_size(x.m_size),
    m_data(std::move(x.m_data))
{
    x.m_reserve = 0;
    x.m_size = 0;
}

Fastcgipp::Block& Fastcgipp::Block::operator=(Block&& x)
{
    m_reserve = x.m_reserve;
    x.m_reserve = 0;
    m_size = x.m_size;
    x.m_size = 0;
    m_data = std::move(x.m_data);
    return *this;
}

void Fastcgipp::Block::size(size_t x)
{
    if(x > m_reserve)
    {
        m_size = m_reserve;
        reserve(x);
    }
    m_size = x;
}

void Fastcgipp::Block::clear()
{
    m_reserve = 0;
    m_size = 0;
    m_data.reset();
}

void Fastcgipp::Block::assign(const char* const data, const size_t size_)
{
    if(size_ > m_reserve)
    {
        m_data.reset(new char[size_]);
        m_reserve = size_;
    }
    m_size = size_;
    std::copy(data, data+size_, m_data.get());
}
