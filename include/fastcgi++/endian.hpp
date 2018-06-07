/*!
 * @file       endian.hpp
 * @brief      Helper file for converting native and big endian numbers
 * @author     Aaron Bishop &lt;erroneous@gmail.com&gt;
 * @date       June 7, 2018
 * @copyright  Copyright &copy; 2017 Eddie Carle. This project is released under
 *             the GNU Lesser General Public License Version 3.
 */

/*******************************************************************************
* Copyright (C) 2018 Aaron Bishop [erroneous@gmail.com]                             *
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

#ifndef FASTCGIPP_ENDIAN_HPP
#define FASTCGIPP_ENDIAN_HPP

#include <cstdint>
#include <type_traits>

#ifdef BSD
#include <sys/endian.h>
#elif defined (__unix__) || (defined (__APPLE__) && defined (__MACH__))
#include <endian.h>
#elif defined(WIN32) && (defined(_M_IX86) || defined(_M_X64))
#define BIG_ENDIAN 1234
#define LITTLE_ENDIAN 4321
#define BYTE_ORDER LITTLE_ENDIAN
#endif

#ifndef BIG_ENDIAN
#if defined(__BIG_ENDIAN)
#define BIG_ENDIAN __BIG_ENDIAN
#elif defined(__BIG_ENDIAN__)
#define BIG_ENDIAN __BIG_ENDIAN__
#endif
#endif

#ifndef LITTLE_ENDIAN
#if defined(__LITTLE_ENDIAN)
#define LITTLE_ENDIAN __LITTLE_ENDIAN
#elif defined(__LITTLE_ENDIAN__)
#define LITTLE_ENDIAN __LITTLE_ENDIAN__
#endif
#endif

#ifndef BYTE_ORDER
#if defined(__BYTE_ORDER)
#define BYTE_ORDER __BYTE_ORDER
#elif defined(__BYTE_ORDER__)
#define BYTE_ORDER __BYTE_ORDER__
#endif
#endif

#if !(defined(BIG_ENDIAN) && defined(LITTLE_ENDIAN) && defined(BYTE_ORDER))
#error "Please define BIG_ENDIAN, LITTLE_ENDIAN, and BYTE_ORDER"
#endif

namespace Fastcgipp
{
    namespace detail
    {
        template<class U>
        using IsByteSwappable = std::integral_constant<bool,
            std::is_same<U, std::uint16_t>::value ||
            std::is_same<U, std::uint32_t>::value ||
            std::is_same<U, std::uint64_t>::value>;
    }
    enum class Endian
    {
        little = LITTLE_ENDIAN,
        big = BIG_ENDIAN,
        native = BYTE_ORDER
    };
    
    constexpr std::uint16_t byteSwap(const std::uint16_t v) noexcept
    {
        return
            (v & 0xff00) >> 8 |
            (v & 0x00ff) << 8;
    }
    constexpr std::uint32_t byteSwap(const std::uint32_t v) noexcept
    {
        return
            (v & 0xff000000) >> 24 |
            (v & 0x00ff0000) >> 8 |
            (v & 0x0000ff00) << 8 |
            (v & 0x000000ff) << 24;
    }
    constexpr std::uint64_t byteSwap(const std::uint64_t v) noexcept
    {
        return
            (v & 0xff00000000000000) >> 56 |
            (v & 0x00ff000000000000) >> 40 |
            (v & 0x0000ff0000000000) >> 24 |
            (v & 0x000000ff00000000) >> 8 |
            (v & 0x00000000ff000000) << 8 |
            (v & 0x0000000000ff0000) << 24 |
            (v & 0x000000000000ff00) << 40 |
            (v & 0x00000000000000ff) << 56;
    }
    template<Endian e>
    using EndianTag = std::integral_constant<Endian, e>;
    
    using BigEndianTag = EndianTag<Endian::big>;
    using LittleEndianTag = EndianTag<Endian::little>;
    using OtherEndianTag = EndianTag<static_cast<Endian>(10)>;
    
    using NativeEndianTag = EndianTag<Endian::native>;

    template<class U,
        typename = typename std::enable_if<
            detail::IsByteSwappable<U>::value>::type>
    constexpr U toBigEndian(U v, BigEndianTag) noexcept
    {
        return v;
    }

    template<class U,
        typename = typename std::enable_if<
            detail::IsByteSwappable<U>::value>::type>
    constexpr U toBigEndian(U v, LittleEndianTag) noexcept
    {
        return byteSwap(v);
    }
    
    template<class U,
        typename = typename std::enable_if<
            detail::IsByteSwappable<U>::value>::type>
    constexpr U toBigEndian(U v, OtherEndianTag) noexcept
    {
        union {
            U u;
            std::uint8_t b[sizeof(U)];
        } ret = { .u = 0 };
        for(unsigned i = sizeof(U); i != 0; )
        {
            ret.b[--i] = static_cast<std::uint8_t>(v & 0xff);
            v >>= 8;
        }
        return ret.u;
    }
    template<class U,
        typename = typename std::enable_if<
            detail::IsByteSwappable<U>::value>::type>
    constexpr U toBigEndian(U v) noexcept
    {
        return toBigEndian(v, NativeEndianTag{});
    }

    template<class U,
        typename = typename std::enable_if<
            detail::IsByteSwappable<U>::value>::type>
    constexpr U fromBigEndian(U v, BigEndianTag) noexcept
    {
        return v;
    }

    template<class U,
        typename = typename std::enable_if<
            detail::IsByteSwappable<U>::value>::type>
    constexpr U fromBigEndian(U v, LittleEndianTag) noexcept
    {
        return byteSwap(v);
    }
    
    template<class U,
        typename = typename std::enable_if<
            detail::IsByteSwappable<U>::value>::type>
    constexpr U fromBigEndian(U v, OtherEndianTag) noexcept
    {
        union {
            U u;
            std::uint8_t b[sizeof(U)];
        } un = { .u = v };
        U ret = 0;
        for(unsigned i = 0; i < sizeof(U); ++i)
            ret = (ret << 8) | un.b[i];
        return ret;
    }
    template<class U,
        typename = typename std::enable_if<
            detail::IsByteSwappable<U>::value>::type>
    constexpr U fromBigEndian(U v) noexcept
    {
        return fromBigEndian(v, NativeEndianTag{});
    }
}

#endif
