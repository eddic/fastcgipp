/*!
 * @file       chunkstreambuf.hpp
 * @brief      Declares the ChunkStreamBuf stuff
 * @author     Eddie Carle &lt;eddie@isatec.ca&gt;
 * @date       May 6, 2020
 * @copyright  Copyright &copy; 2020 Eddie Carle. This project is released under
 *             the GNU Lesser General Public License Version 3.
 */
/*******************************************************************************
* Copyright (C) 2020 Eddie Carle [eddie@isatec.ca]                             *
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

#ifndef FASTCGIPP_CHUNKSTREAMBUF_HPP
#define FASTCGIPP_CHUNKSTREAMBUF_HPP

#include <memory>
#include <list>
#include <ostream>

#include "fastcgi++/webstreambuf.hpp"

//! Topmost namespace for the fastcgi++ library
namespace Fastcgipp
{
    //! De-templated base class for ChunkStreamBuf
    /*!
     * @date    May 6, 2020
     * @author  Eddie Carle &lt;eddie@isatec.ca&gt;
     */
    class ChunkStreamBuf_base
    {
    public:
        //! A chunk of body data
        struct Chunk
        {
            //! Total capacity of a chunk
            static constexpr unsigned capacity = 4096;

            //! Pointer to chunk data
            std::unique_ptr<char[]> data;

            //! Size of chunk data
            unsigned size;

            Chunk():
                data(new char[capacity]),
                size(0)
            {}
        };

        //! Chunks of body data
        std::list<Chunk> m_body;
    };

    template<class charT> class ChunkStreamBuf;

    //! Specialized %ChunkStreamBuf stream buffer for char
    template<> class ChunkStreamBuf<char>:
        public WebStreambuf<char>,
        public ChunkStreamBuf_base
    {
    private:
        //! Setup the stream buffer pointer
        inline void setBufferPtr();

    public:
        ChunkStreamBuf();

        void clear();

        //! Empty/flush the buffer
        bool emptyBuffer();

        virtual ~ChunkStreamBuf()
        {
            emptyBuffer();
        }
    };

    //! Specialized %ChunkStreamBuf stream buffer for wchar_t
    template<> class ChunkStreamBuf<wchar_t>:
        public WebStreambuf<wchar_t>,
        public ChunkStreamBuf_base
    {
    private:
        //! Buffer for wide character stuff
        wchar_t m_buffer[Chunk::capacity];

    public:
        ChunkStreamBuf();

        void clear();

        //! Empty/flush the buffer
        bool emptyBuffer();

        virtual ~ChunkStreamBuf()
        {
            emptyBuffer();
        }
    };
}

#endif
