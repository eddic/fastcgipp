/*!
 * @file       parameters.hpp
 * @brief      Declares SQL parameters types
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

#ifndef FASTCGIPP_SQL_PARAMETERS_HPP
#define FASTCGIPP_SQL_PARAMETERS_HPP

#include "fastcgi++/protocol.hpp"
#include "fastcgi++/log.hpp"

#include <postgres.h>
#include <libpq-fe.h>
#include <catalog/pg_type.h>
#include <tuple>
#include <utility>
#include <string>
#include <vector>
#include <type_traits>

//! Topmost namespace for the fastcgi++ library
namespace Fastcgipp
{
    //! Contains all fastcgi++ SQL facilities
    namespace SQL
    {
        //! A single parameter in an SQL query
        /*!
         * All these types are assignable from an object of the template
         * parameter type while providing some additional interfacing so they
         * fit in nicely into a Parameters tuple.
         *
         * This generic version is specifically for any trivially copyable type
         * that should be represented database side as a BYTEA.
         */
        template<typename T> class Parameter
        {
            static_assert(std::is_trivially_copyable<T>::value, "Only trivially"
                    " copyable types allowed in the BYTEA Parameter");

        private:
            T m_data;

        public:
            constexpr Parameter()
            {}

            Parameter(const T& x):
                m_data(x)
            {}

            Parameter& operator=(const T& x)
            {
                m_data = x;
            }

            //! Size in bytes of this contiguous block of data
            constexpr size_t size() const
            {
                return sizeof(T);
            }

            //! Associated PostgreSQL Oid
            static const Oid oid = BYTEAOID;

            //! Pointer to the data
            const char* raw() const
            {
                return reinterpret_cast<const char*>(&m_data);
            }
        };

        //! Parameter specialization for 16 bit signed integers
        template<>
        class Parameter<int16_t>
        {
        private:
            Protocol::BigEndian<int16_t> m_data;

        public:
            constexpr Parameter()
            {}

            Parameter(const int16_t x):
                m_data(x)
            {}

            Parameter& operator=(const int16_t x)
            {
                m_data = x;
                return *this;
            }

            //! Size in bytes of integer (2)
            constexpr size_t size() const
            {
                return sizeof(int16_t);
            }

            //! Associated PostgreSQL Oid
            static const Oid oid = INT2OID;

            //! Pointer to start of big endian integer representation
            const char* raw() const
            {
                return reinterpret_cast<const char*>(&m_data);
            }
        };

        //! Parameter specialization for 32 bit signed integers
        template<>
        class Parameter<int32_t>
        {
        private:
            Protocol::BigEndian<int32_t> m_data;

        public:
            constexpr Parameter()
            {}

            Parameter(const int32_t x):
                m_data(x)
            {}

            Parameter& operator=(const int32_t x)
            {
                m_data = x;
                return *this;
            }

            //! Size in bytes of integer (4)
            constexpr size_t size() const
            {
                return sizeof(int32_t);
            }

            //! Associated PostgreSQL Oid
            static const Oid oid = INT4OID;

            //! Pointer to start of big endian integer representation
            const char* raw() const
            {
                return reinterpret_cast<const char*>(&m_data);
            }
        };

        //! Parameter specialization for 64 bit signed integers
        template<>
        class Parameter<int64_t>
        {
        private:
            Protocol::BigEndian<int64_t> m_data;

        public:
            constexpr Parameter()
            {}

            Parameter(const int64_t x):
                m_data(x)
            {}

            Parameter& operator=(const int64_t x)
            {
                m_data = x;
                return *this;
            }

            //! Size in bytes of integer (8)
            constexpr size_t size() const
            {
                return sizeof(int64_t);
            }

            //! Associated PostgreSQL Oid
            static const Oid oid = INT8OID;

            //! Pointer to start of big endian integer representation
            const char* raw() const
            {
                return reinterpret_cast<const char*>(&m_data);
            }
        };

        //! Parameter specialization for 32 bit floating point values
        template<>
        class Parameter<float>
        {
        private:
            Protocol::BigEndian<float> m_data;

        public:
            constexpr Parameter()
            {}

            Parameter(const float x):
                m_data(x)
            {}

            Parameter& operator=(const float x)
            {
                m_data = x;
                return *this;
            }

            //! Size in bytes of float (4)
            constexpr size_t size() const
            {
                return sizeof(float);
            }

            //! Associated PostgreSQL Oid
            static const Oid oid = FLOAT4OID;

            //! Pointer to start of big endian floating point representation
            const char* raw() const
            {
                return reinterpret_cast<const char*>(&m_data);
            }
        };

        //! Parameter specialization for 64 bit floating point values
        template<>
        class Parameter<double>
        {
        private:
            Protocol::BigEndian<double> m_data;

        public:
            constexpr Parameter()
            {}

            Parameter(const double x):
                m_data(x)
            {}

            Parameter& operator=(const double x)
            {
                m_data = x;
                return *this;
            }

            //! Size in bytes of double (8)
            constexpr size_t size() const
            {
                return sizeof(double);
            }

            //! Associated PostgreSQL Oid
            static const Oid oid = FLOAT8OID;

            //! Pointer to start of big endian double float representation
            const char* raw() const
            {
                return reinterpret_cast<const char*>(&m_data);
            }
        };

        //! Parameter specialization for character text strings
        /*!
         * This specialization should be used for both wide and narrow character
         * strings.
         */
        template<>
        struct Parameter<std::string>
        {
        private:
            std::string m_data;

        public:
            Parameter()
            {}

            Parameter(const std::string& x):
                m_data(x)
            {}

            Parameter& operator=(const std::string& x)
            {
                m_data = x;
                return *this;
            }

            Parameter& operator=(const std::wstring& x);

            Parameter(const std::wstring& x)
            {
                *this = x;
            }

            //! Associated PostgreSQL Oid
            static const Oid oid = TEXTOID;

            //! Pointer to start of text string
            const char* raw() const
            {
                return m_data.data();
            }

            size_t size() const
            {
                return m_data.size();
            }
        };

        //! De-templated base class for Parameters
        class Parameters_base
        {
        protected:
            //! Array of Oids for each parameter
            /*!
             * This gets initialized by calling build().
             */
            const std::vector<Oid>* m_oids;

            //! Array of raw data pointers for each parameter
            /*!
             * This gets initialized by calling build().
             */
            std::vector<const char*> m_raws;

            //! Array of sizes for each parameter
            /*!
             * This gets initialized by calling build().
             */
            std::vector<size_t> m_sizes;

            //! Template side virtual to populate the above arrays
            /*!
             * This just gets called by build().
             */
            virtual void build_impl() =0;

        public:
            //! Initialize the arrays needed by PostgreSQL
            void build();

            //! Constant pointer to array of all parameter Oids
            /*!
             * This is not valid until build() is called
             */
            const Oid* oids() const
            {
                return m_oids->data();
            }

            //! Constant pointer to pointer array of all raw parameter data
            /*!
             * This is not valid until build() is called
             */
            const char* const* raws() const
            {
                return m_raws.data();
            }

            //! Constant pointer to array of all parameter sizes
            const size_t* sizes() const
            {
                return m_sizes.data();
            }

            //! How many parameters in this tuple?
            virtual size_t size() const =0;

            virtual ~Parameters_base() {}
        };

        //! A tuple of parameters to tie to a PostgreSQL query
        /*!
         * This class allows you to pass separate parameters to a PostgreSQL
         * query. From the interface perspective this should behave exactly like
         * an std::tuple<Types...>. The differences from std::tuple lie in it's
         * ability to format the tuple data in a way PostgreSQL wants to see it.
         *
         * @tparam Types Pack of types to contain in the tuple.
         * @date    May 13, 2017
         * @author  Eddie Carle &lt;eddie@isatec.ca&gt;
         */
        template<typename... Types>
        class Parameters:
            public std::tuple<Parameter<Types>...>,
            public Parameters_base
        {
        private:
            static const std::vector<Oid> s_oids;

            //! How many items in the tuple?
            constexpr size_t size() const
            {
                return sizeof...(Types);
            }

            //! Recursive template PostgreSQL array building function
            template<size_t column, size_t... columns>
            inline void build_impl(std::index_sequence<column, columns...>)
            {
                m_raws.push_back(std::get<column>(*this).raw());
                m_sizes.push_back(std::get<column>(*this).size());
                build_impl(std::index_sequence<columns...>{});
            }

            //! Terminating template PostgreSQL array building function
            template<size_t column>
            inline void build_impl(std::index_sequence<column>)
            {
                m_raws.push_back(std::get<column>(*this).raw());
                m_sizes.push_back(std::get<column>(*this).size());
            }

            void build_impl();

        public:
            using std::tuple<Parameter<Types>...>::tuple;
            using std::tuple<Parameter<Types>...>::operator=;
        };
    }
}

template<typename... Types>
void Fastcgipp::SQL::Parameters<Types...>::build_impl()
{
    m_oids = &s_oids;
    build_impl(std::index_sequence_for<Types...>{});
}

template<typename... Types>
const std::vector<Oid> Fastcgipp::SQL::Parameters<Types...>::s_oids
{
    Fastcgipp::SQL::Parameter<Types>::oid...
};

#endif
