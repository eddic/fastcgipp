/*!
 * @file       results.hpp
 * @brief      Declares SQL results types
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

#ifndef FASTCGIPP_SQL_RESULTS_HPP
#define FASTCGIPP_SQL_RESULTS_HPP

#include "fastcgi++/protocol.hpp"
#include "fastcgi++/log.hpp"

#include <postgres.h>
#include <libpq-fe.h>
#include <catalog/pg_type.h>
#include <tuple>

//! Topmost namespace for the fastcgi++ library
namespace Fastcgipp
{
    //! Contains all fastcgi++ SQL facilities
    namespace SQL
    {
        enum class Status
        {
            noResult,
            emptyQuery,
            commandOk,
            rowsOk,
            copyOut,
            copyIn,
            badResponse,
            nonfatalError,
            fatalError,
            copyBoth,
            singleTuple
        };

        class Results_base
        {
        public:
            PGresult *m_res;
            virtual ~Results_base()
            {
                if(m_res != nullptr)
                    PQclear(m_res);
            }

            Status status() const;

            const char* errorMessage() const
            {
                return PQresultErrorMessage(m_res);
            }
            
            int rows() const
            {
                return PQntuples(m_res);
            }

            int null(int row, int column) const
            {
                return static_cast<bool>(PQgetisnull(m_res, row, column));
            }

        protected:
            Results_base():
                m_res(nullptr)
            {}

            int columns() const
            {
                return PQnfields(m_res);
            }

            int columnLength(int row, int column) const
            {
                return PQgetlength(m_res, row, column);
            }

            template<typename T> bool verifyColumn(int column) const;
            template<typename T> T field(int row, int column) const;
        };

        template<typename T>
        bool Results_base::verifyColumn(int column) const
        {
            return PQftype(m_res, column) == BYTEAOID
                && PQfsize(m_res, column) == sizeof(T);
        }
        template<typename T>
        T Results_base::field(int row, int column) const
        {
            return *reinterpret_cast<const T*>(PQgetvalue(m_res, row, column));
        }

        template<>
        bool Results_base::verifyColumn<int16_t>(int column) const
        {
            return PQftype(m_res, column) == INT2OID
                && PQfsize(m_res, column) == 2;
        }
        template<>
        int16_t Results_base::field<int16_t>(int row, int column) const
        {
            return Protocol::BigEndian<int16_t>::read(
                    PQgetvalue(m_res, row, column));
        }

        template<>
        bool Results_base::verifyColumn<int32_t>(int column) const
        {
            return PQftype(m_res, column) == INT4OID
                && PQfsize(m_res, column) == 4;
        }
        template<>
        int32_t Results_base::field<int32_t>(int row, int column) const
        {
            return Protocol::BigEndian<int32_t>::read(
                    PQgetvalue(m_res, row, column));
        }

        template<>
        bool Results_base::verifyColumn<int64_t>(int column) const
        {
            return PQftype(m_res, column) == INT8OID
                && PQfsize(m_res, column) == 8;
        }
        template<>
        int64_t Results_base::field<int64_t>(int row, int column) const
        {
            return Protocol::BigEndian<int64_t>::read(
                    PQgetvalue(m_res, row, column));
        }

        template<>
        bool Results_base::verifyColumn<float>(int column) const
        {
            return PQftype(m_res, column) == FLOAT4OID
                && PQfsize(m_res, column) == 4;
        }
        template<>
        float Results_base::field<float>(int row, int column) const
        {
            return Protocol::BigEndian<float>::read(
                    PQgetvalue(m_res, row, column));
        }

        template<>
        bool Results_base::verifyColumn<double>(int column) const
        {
            return PQftype(m_res, column) == FLOAT8OID
                && PQfsize(m_res, column) == 8;
        }
        template<>
        double Results_base::field<double>(int row, int column) const
        {
            return Protocol::BigEndian<double>::read(
                    PQgetvalue(m_res, row, column));
        }

        template<>
        bool Results_base::verifyColumn<std::string>(int column) const
        {
            const Oid type = PQftype(m_res, column);
            return type == TEXTOID || type == VARCHAROID;
        }
        template<>
        std::string Results_base::field<std::string>(int row, int column) const
        {
            return std::string(
                    PQgetvalue(m_res, row, column),
                    PQgetlength(m_res, row, column));
        }

        template<typename... Types>
        class Results: public Results_base
        {
        public:
            typedef std::tuple<Types...> Row;

        private:
            static const size_t size = sizeof...(Types);

            template<int column, int... columns>
            int verify_impl(
                    std::integer_sequence<int, column, columns...>) const
            {
                if(verifyColumn<std::tuple_element<column, Types...>>(column))
                    return verify_impl(
                            std::integer_sequence<int, columns...>{});
                else
                    return column+1;
            }

            template<int column>
            int verify_impl(std::integer_sequence<int, column>) const
            {
                if(verifyColumn<std::tuple_element<column, Types...>>(column))
                    return 0;
                else
                    return column+1;
            }

            template<int... columns>
            Row row_impl(
                    int index, std::integer_sequence<int, columns...>) const
            {
                return Row(field<std::tuple_element<columns, Types...>>(
                            index,
                            columns)...);
            }

        public:
            int verify() const
            {
                if(columns() != size)
                    return -1;
                else
                    return verify_impl(
                            std::make_integer_sequence<int, size>{});
            }

            Row row(int index) const
            {
                return row_impl(
                        index,
                        std::make_integer_sequence<int, size>{});
            }

            void doNothing()
            {
                if(1 == 2)
                    return;
            }
        };
    }
}

#endif
