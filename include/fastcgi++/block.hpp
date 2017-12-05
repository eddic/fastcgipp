/*!
 * @file       block.hpp
 * @brief      Declares the Block data structure.
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

#ifndef BLOCK_HPP
#define BLOCK_HPP

#include <memory>

namespace Fastcgipp
{
    //! Data structure to hold a block of raw data
    /*!
     * This is basically a stripped down std::vector. It contains a contiguous
     * block of memory with two sizes associated with it. The reserve indicates
     * how much data is actually allocated while the size tells us how much of
     * the data is relevant. The motivation for this as opposed to a vector is
     * that this lacks element initialization.
     */
    class Block
    {
    private:
        //! Total bytes allocated
        size_t m_reserve;

        //! Size of relevant part of data allocation
        size_t m_size;

        //! Point to allocated data
        std::unique_ptr<char[]> m_data;

    public:
        //! Initialize an empty block
        Block();

        //! Initialize a block with equal size and reserve
        Block(const size_t size_);

        //! Initialize a block with equal size and reserve from source data
        Block(const char* const data, const size_t size_);

        //! Assign a sequence a data to the block
        /*!
         * If the reserve if smaller the requested size then reallocation
         * occurs. Otherwise the allocation is unchanged.
         */
        void assign(const char* const data, const size_t size_);

        //! Steal the data from another block
        Block(Block&& x);

        //! Steal the data from another block
        Block& operator=(Block&& x);

        //! See the reserve size
        size_t reserve() const
        {
            return m_reserve;
        }

        //! Set the reserve size
        /*!
         * Unlike std::vector this always obeys your command even if you are
         * decreasing the reserve size. All data is copied over when a reserve
         * is changed.
         */
        void reserve(size_t x);

        //! See the relevant data size
        size_t size() const
        {
            return m_size;
        }

        //! Set the relevant data size
        void size(size_t x);

        //! Pointer to the first element
        char* begin()
        {
            return m_data.get();
        }

        //! Constant pointer to the first element
        const char* begin() const
        {
            return m_data.get();
        }

        //! Pointer to 1+ the last element
        char* end()
        {
            return m_data.get()+m_size;
        }

        //! Constant pointer to 1+ the last element
        const char* end() const
        {
            return m_data.get()+m_size;
        }

        //! Deallocate memory and set size and reserve to zero
        void clear();

        Block(const Block&) =delete;
        Block& operator=(const Block&) =delete;
    };
}

#endif
