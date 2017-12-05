/*!
 * @file       webstreambuf.cpp
 * @brief      Defines the WebStreambuf stuff
 * @author     Eddie Carle &lt;eddie@isatec.ca&gt;
 * @date       May 20, 2016
 * @copyright  Copyright &copy; 2016 Eddie Carle. This project is released under
 *             the GNU Lesser General Public License Version 3.
 */

/*******************************************************************************
* Copyright (C) 2016 Eddie Carle [eddie@isatec.ca]                             *
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

#include "fastcgi++/webstreambuf.hpp"
#include "fastcgi++/log.hpp"

#include <algorithm>

template
std::basic_ostream<wchar_t, std::char_traits<wchar_t>>& Fastcgipp::operator<<(
        std::basic_ostream<wchar_t, std::char_traits<wchar_t>>& os,
        const Encoding& encoding);
template
std::basic_ostream<char, std::char_traits<char>>& Fastcgipp::operator<<(
        std::basic_ostream<char, std::char_traits<char>>& os,
        const Encoding& encoding);
template<class charT, class traits>
std::basic_ostream<charT, traits>& Fastcgipp::operator<<(
        std::basic_ostream<charT, traits>& os,
        const Encoding& encoding)
{
    try
    {
        WebStreambuf<charT, traits>* webStreambuf =
            dynamic_cast<WebStreambuf<charT, traits>*>(os.rdbuf());
        webStreambuf->m_encoding = encoding;
    }
    catch(std::bad_cast& e)
    {
        ERROR_LOG("Trying to set encoding on a stream buffer that isn't a "\
                "WebStreambuf")
    }
    return os;
}

namespace Fastcgipp
{
    template <>
    const std::map<char, const std::basic_string<char>>
    Fastcgipp::WebStreambuf<char, std::char_traits<char>>::htmlCharacters
    {
        std::make_pair('"', "&quot;"),
        std::make_pair('>', "&gt;"),
        std::make_pair('<', "&lt;"),
        std::make_pair('&', "&amp;"),
        std::make_pair(0x27, "&apos;")
    };

    template <>
    const std::map<wchar_t, const std::basic_string<wchar_t>>
    Fastcgipp::WebStreambuf<wchar_t, std::char_traits<wchar_t>>::htmlCharacters
    {
        std::make_pair('"', L"&quot;"),
        std::make_pair('>', L"&gt;"),
        std::make_pair('<', L"&lt;"),
        std::make_pair('&', L"&amp;"),
        std::make_pair(0x27, L"&apos;")
    };

    template <>
    const std::map<char, const std::basic_string<char>>
    Fastcgipp::WebStreambuf<char, std::char_traits<char>>::urlCharacters
    {
        std::make_pair('!', "%21"),
        std::make_pair(']', "%5D"),
        std::make_pair('[', "%5B"),
        std::make_pair('#', "%23"),
        std::make_pair('?', "%3F"),
        std::make_pair('/', "%2F"),
        std::make_pair(',', "%2C"),
        std::make_pair('$', "%24"),
        std::make_pair('+', "%2B"),
        std::make_pair('=', "%3D"),
        std::make_pair('&', "%26"),
        std::make_pair('@', "%40"),
        std::make_pair(':', "%3A"),
        std::make_pair(';', "%3B"),
        std::make_pair(')', "%29"),
        std::make_pair('(', "%28"),
        std::make_pair(0x27, "%27"),
        std::make_pair('*', "%2A"),
        std::make_pair('<', "%3C"),
        std::make_pair('>', "%3E"),
        std::make_pair('"', "%22"),
        std::make_pair(' ', "%20"),
        std::make_pair('%', "%25")
    };

    template <>
    const std::map<wchar_t, const std::basic_string<wchar_t>>
    Fastcgipp::WebStreambuf<wchar_t, std::char_traits<wchar_t>>::urlCharacters
    {
        std::make_pair('!', L"%21"),
        std::make_pair(']', L"%5D"),
        std::make_pair('[', L"%5B"),
        std::make_pair('#', L"%23"),
        std::make_pair('?', L"%3F"),
        std::make_pair('/', L"%2F"),
        std::make_pair(',', L"%2C"),
        std::make_pair('$', L"%24"),
        std::make_pair('+', L"%2B"),
        std::make_pair('=', L"%3D"),
        std::make_pair('&', L"%26"),
        std::make_pair('@', L"%40"),
        std::make_pair(':', L"%3A"),
        std::make_pair(';', L"%3B"),
        std::make_pair(')', L"%29"),
        std::make_pair('(', L"%28"),
        std::make_pair(0x27, L"%27"),
        std::make_pair('*', L"%2A"),
        std::make_pair('<', L"%3C"),
        std::make_pair('>', L"%3E"),
        std::make_pair('"', L"%22"),
        std::make_pair(' ', L"%20"),
        std::make_pair('%', L"%25")
    };
}

template <class charT, class traits>
std::streamsize Fastcgipp::WebStreambuf<charT, traits>::xsputn(
        const char_type *s,
        std::streamsize n)
{
    const char_type* const end = s+n;

    while(true)
    {
        if(m_encoding == Encoding::NONE)
        {
            const std::streamsize actual = std::min(
                    end-s,
                    this->epptr()-this->pptr());
            std::copy(s, s+actual, this->pptr());
            this->pbump(actual);
            s += actual;
        }
        else
        {
            const std::map<charT, const std::basic_string<charT>>* map;
            if(m_encoding == Encoding::HTML)
                map = &htmlCharacters;
            else
                map = &urlCharacters;

            while(s<end)
            {
                const size_t writeSpace = this->epptr() - this->pptr();
                const auto mapping = map->find(*s);
                if(mapping == map->cend())
                {
                    if(writeSpace < 1)
                        break;
                    *this->pptr() = *s++;
                    this->pbump(1);
                }
                else
                {
                    if(writeSpace < mapping->second.size())
                        break;
                    std::copy(
                            mapping->second.cbegin(),
                            mapping->second.cend(),
                            this->pptr());
                    this->pbump(mapping->second.size());
                    ++s;
                }
            }
        }

        if(s<end)
            this->sync();
        else
            break;
    }

    return n;
}

template class Fastcgipp::WebStreambuf<wchar_t, std::char_traits<wchar_t>>;
template class Fastcgipp::WebStreambuf<char, std::char_traits<char>>;
