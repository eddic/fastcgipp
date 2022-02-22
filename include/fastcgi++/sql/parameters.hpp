/*!
 * @file       parameters.hpp
 * @brief      Declares %SQL parameters types
 * @author     Eddie Carle &lt;eddie@isatec.ca&gt;
 * @date       February 18, 2022
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

#ifndef FASTCGIPP_SQL_PARAMETERS_HPP
#define FASTCGIPP_SQL_PARAMETERS_HPP

#include "fastcgi++/endian.hpp"
#include "fastcgi++/sql/types.hpp"

#include <tuple>
#include <vector>
#include <memory>

//! Topmost namespace for the fastcgi++ library
namespace Fastcgipp
{
    //! Contains all fastcgi++ %SQL facilities
    namespace SQL
    {
        //! A single parameter in an %SQL query
        /*!
         * All these types are assignable from an object of the template
         * parameter type while providing some additional interfacing so they
         * fit in nicely into a Parameters tuple. Note that no generic
         * definition exists for this class so only specializations are valid.
         */
        template<typename T> class Parameter;

        template<>
        class Parameter<BOOL>
        {
        private:
            char m_data;
        public:
            static const unsigned oid;
            constexpr Parameter(BOOL x) noexcept:
                m_data(static_cast<char>(x))
            {}
            constexpr Parameter& operator=(BOOL x) noexcept
            {
                m_data = static_cast<char>(x);
                return *this;
            }
            constexpr const char* data() const
            {
                return &m_data;
            }
            constexpr unsigned size() const
            {
                return 1;
            }
        };

        template<>
        struct Parameter<SMALLINT>: public BigEndian<SMALLINT>
        {
            using BigEndian<SMALLINT>::BigEndian;
            using BigEndian<SMALLINT>::operator=;
            static const unsigned oid;
        };

        template<>
        struct Parameter<INTEGER>: public BigEndian<INTEGER>
        {
            using BigEndian<INTEGER>::BigEndian;
            using BigEndian<INTEGER>::operator=;
            static const unsigned oid;
        };

        template<>
        struct Parameter<BIGINT>: public BigEndian<BIGINT>
        {
            using BigEndian<BIGINT>::BigEndian;
            using BigEndian<BIGINT>::operator=;
            static const unsigned oid;
        };

        template<>
        struct Parameter<REAL>: public BigEndian<REAL>
        {
            using BigEndian<REAL>::BigEndian;
            using BigEndian<REAL>::operator=;
            static const unsigned oid;
        };

        template<>
        struct Parameter<DOUBLE_PRECISION>: public BigEndian<DOUBLE_PRECISION>
        {
            using BigEndian<DOUBLE_PRECISION>::BigEndian;
            using BigEndian<DOUBLE_PRECISION>::operator=;
            static const unsigned oid;
        };

        template<>
        struct Parameter<TEXT>: public TEXT
        {
            Parameter(const TEXT& x):
                TEXT(x)
            {}
            using TEXT::TEXT;
            using TEXT::operator=;
            static const unsigned oid;
        };

        template<>
        struct Parameter<BYTEA>: public BYTEA
        {
            Parameter(const BYTEA& x):
                BYTEA(x)
            {}
            using BYTEA::BYTEA;
            using BYTEA::operator=;
            static const unsigned oid;
        };

        template<>
        class Parameter<WTEXT>: public TEXT
        {
        private:
            static TEXT convert(const WTEXT& x);

        public:
            Parameter& operator=(const WTEXT& x)
            {
                 assign(convert(x));
                 return *this;
            }

            Parameter(const WTEXT& x):
                TEXT(convert(x))
            {}

            static const unsigned oid;
        };

        template<>
        class Parameter<TIMESTAMPTZ>:
            public BigEndian<BIGINT>
        {
        private:
            using BigEndian<BIGINT>::operator=;
            static BIGINT convert(const TIMESTAMPTZ& x)
            {
                using namespace std::chrono;
                constexpr TIMESTAMPTZ epoch(TIMESTAMPTZ() + seconds(946684800));
                return (x-epoch).count();
            }

        public:
            Parameter& operator=(const TIMESTAMPTZ& x)
            {
                *this = convert(x);
                return *this;
            }

            Parameter(const TIMESTAMPTZ& x):
                BigEndian<BIGINT>(convert(x))
            {}

            static const unsigned oid;
        };

        template<>
        class Parameter<DATE>:
            public BigEndian<INTEGER>
        {
        private:
            using BigEndian<INTEGER>::operator=;
            static INTEGER convert(const DATE& x)
            {
                using namespace std::chrono;
                constexpr DATE epoch(DATE() + days(10957));
                return (x-epoch).count();
            }

        public:
            Parameter& operator=(const DATE& x)
            {
                *this = convert(x);
                return *this;
            }

            Parameter(const DATE& x):
                BigEndian<INTEGER>(convert(x))
            {}

            static const unsigned oid;
        };

        template<>
        struct Parameter<INET>: public std::array<char, 20>
        {
            Parameter& operator=(const INET& x)
            {
                auto next = begin();
                *next++ = addressFamily;
                *next++ = 128;
                *next++ = 0;
                *next++ = 16;
                next = std::copy_n(
                        reinterpret_cast<const char*>(&x),
                        INET::size,
                        next);
                return *this;
            }
            Parameter(const INET& x)
            {
                *this = x;
            }
            static const unsigned oid;
            static const char addressFamily;
        };

        template<typename Numeric>
        class Parameter<ARRAY<Numeric>>
        {
        private:
            static_assert(
                    std::is_integral<Numeric>::value ||
                        std::is_floating_point<Numeric>::value,
                    "Numeric must be a numeric type.");
            unsigned m_size;
            std::unique_ptr<char[]> m_data;
        public:
            void resize(const unsigned size);

            Parameter& operator=(const ARRAY<Numeric>& x);

            Parameter(const ARRAY<Numeric>& x)
            {
                *this = x;
            }

            Parameter(const unsigned size)
            {
                resize(size);
            }

            BigEndian<Numeric>& operator[](const unsigned i)
            {
                return *reinterpret_cast<BigEndian<Numeric>*>(
                        m_data.get() + 6*sizeof(ARRAY_SIZE)
                        + i*(sizeof(ARRAY_SIZE) + sizeof(Numeric)));
            }

            unsigned size() const
            {
                return m_size;
            }

            const char* data() const
            {
                return m_data.get();
            }

            static const unsigned oid;
        };

        template<>
        class Parameter<ARRAY<TEXT>>
        {
        private:
            unsigned m_size;
            std::unique_ptr<char[]> m_data;
        protected:
            void assign(const ARRAY<TEXT>& x);
        public:
            Parameter& operator=(const ARRAY<TEXT>& x)
            {
                assign(x);
                return *this;
            }

            Parameter(const ARRAY<TEXT>& x)
            {
                assign(x);
            }

            TEXT operator[](const unsigned i) const;

            unsigned size() const
            {
                return m_size;
            }

            const char* data() const
            {
                return m_data.get();
            }

            static const unsigned oid;
        };

        template<>
        class Parameter<ARRAY<WTEXT>>:
            public Parameter<ARRAY<TEXT>>
        {
        private:
            static ARRAY<TEXT> convert(
                    const ARRAY<WTEXT>& x);
            static WTEXT convert(const TEXT& x);
        public:
            Parameter& operator=(const ARRAY<WTEXT>& x)
            {
                assign(convert(x));
                return *this;
            }

            Parameter(const ARRAY<WTEXT>& x):
                Parameter<ARRAY<TEXT>>(convert(x))
            {}

            WTEXT operator[](const unsigned i) const
            {
                return convert(Parameter<ARRAY<TEXT>>::operator[](i));
            }
        };

        //! De-templated base class for Parameters
        class Parameters_base
        {
        private:
            //! How many columns do we have?
            const int m_size;

            //! True indicates null on the respective column
            std::vector<bool> m_nulls;

        protected:
            //! Array of oids for each parameter
            /*!
             * This gets initialized by calling build().
             */
            const std::vector<unsigned>& m_oids;

            //! Array of raw data pointers for each parameter
            /*!
             * This gets initialized by calling build().
             */
            std::vector<const char*> m_raws;

            //! Array of sizes for each parameter
            /*!
             * This gets initialized by calling build().
             */
            std::vector<int> m_sizes;

            //! Array of formats for each parameter
            /*!
             * This gets initialized by calling build(). It is really just an
             * array of 1s.
             */
            const std::vector<int>& m_formats;

            Parameters_base(
                    const std::vector<unsigned>& oids,
                    const std::vector<int>& formats):
                m_size(oids.size()),
                m_nulls(m_size, false),
                m_oids(oids),
                m_raws(m_size),
                m_sizes(m_size),
                m_formats(formats)
            {}

        public:
            //! Initialize the arrays needed by %SQL
            virtual void build() =0;

            //! Constant pointer to array of all parameter oids
            /*!
             * This is not valid until build() is called
             */
            const unsigned* oids() const
            {
                return m_oids.data();
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
            const int* sizes() const
            {
                return m_sizes.data();
            }

            //! Constant pointer to array of all formats
            const int* formats() const
            {
                return m_formats.data();
            }

            //! How many parameters in this tuple?
            int size() const
            {
                return m_size;
            }

            //! Set a single parameter column to null (zero indexed)
            void setNull(size_t column)
            {
                m_nulls[column] = true;
            }

            //! Check null on a single parameter column (zero indexed)
            bool isNull(size_t column) const
            {
                return m_nulls[column];
            }

            virtual ~Parameters_base() {}
        };

        //! A tuple of parameters to tie to a %SQL query
        /*!
         * This class allows you to pass separate parameters to a %SQL
         * query. From the interface perspective this should behave exactly like
         * an std::tuple<Types...>. The differences from std::tuple lie in it's
         * ability to format the tuple data in a way %SQL wants to see it.
         *
         * @tparam Types Pack of types to contain in the tuple.
         * @date    October 7, 2018
         * @author  Eddie Carle &lt;eddie@isatec.ca&gt;
         */
        template<typename... Types>
        class Parameters:
            public std::tuple<Parameter<Types>...>,
            public Parameters_base
        {
        private:
            static const std::vector<unsigned> s_oids;
            static const std::vector<int> s_formats;

            //! Recursive template %SQL array building function
            template<size_t column, size_t... columns>
            inline void build_impl(std::index_sequence<column, columns...>)
            {
                if(isNull(column))
                    m_raws[column] = nullptr;
                else
                    m_raws[column] = std::get<column>(*this).data();
                m_sizes[column] = std::get<column>(*this).size();
                build_impl(std::index_sequence<columns...>{});
            }

            //! Terminating template %SQL array building function
            template<size_t column>
            inline void build_impl(std::index_sequence<column>)
            {
                if(isNull(column))
                    m_raws[column] = nullptr;
                else
                    m_raws[column] = std::get<column>(*this).data();
                m_sizes[column] = std::get<column>(*this).size();
            }

            void build();

            Parameters(const std::tuple<Types...>& tuple):
                std::tuple<Parameter<Types>...>(tuple),
                Parameters_base(s_oids, s_formats)
            {}

            Parameters(const Types&... args):
                std::tuple<Parameter<Types>...>(args...),
                Parameters_base(s_oids, s_formats)
            {}

            friend std::shared_ptr<Parameters<Types...>> make_Parameters<>(
                    const Types&... args);
            friend std::shared_ptr<Parameters<Types...>> make_Parameters<>(
                    const std::tuple<Types...>& tuple);
        };

        template<typename... Types>
        std::shared_ptr<Parameters<Types...>> make_Parameters(
                const Types&... args)
        {
            return std::shared_ptr<Parameters<Types...>>(
                    new Parameters<Types...>(args...));
        }

        template<typename... Types>
        std::shared_ptr<Parameters<Types...>> make_Parameters(
                const std::tuple<Types...>& tuple)
        {
            return std::shared_ptr<Parameters<Types...>>(
                    new Parameters<Types...>(tuple));
        }
    }
}

template<typename... Types>
void Fastcgipp::SQL::Parameters<Types...>::build()
{
    build_impl(std::index_sequence_for<Types...>{});
}

template<typename... Types>
const std::vector<unsigned> Fastcgipp::SQL::Parameters<Types...>::s_oids
{
    Fastcgipp::SQL::Parameter<Types>::oid...
};

template<typename... Types>
const std::vector<int> Fastcgipp::SQL::Parameters<Types...>::s_formats(
        sizeof...(Types),
        1);

#endif
