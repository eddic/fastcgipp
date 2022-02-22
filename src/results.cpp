/*!
 * @file       results.cpp
 * @brief      Defines SQL results types
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

#include "fastcgi++/sql/results.hpp"
#include "fastcgi++/log.hpp"
#include "sqlTraits.hpp"

#include <locale>
#include <codecvt>
#include <cstdio>
#include <map>

using namespace Fastcgipp::SQL;

// Column verification

template<typename T>
bool Results_base::verifyColumn(int column) const
{
    return Traits<T>::verifyType(m_res, column);
}
template bool Results_base::verifyColumn<BOOL>(int column) const;
template bool Results_base::verifyColumn<SMALLINT>(int column) const;
template bool Results_base::verifyColumn<INTEGER>(int column) const;
template bool Results_base::verifyColumn<BIGINT>(int column) const;
template bool Results_base::verifyColumn<REAL>(int column) const;
template bool Results_base::verifyColumn<DOUBLE_PRECISION>(int column) const;
template bool Results_base::verifyColumn<TIMESTAMPTZ>(int column) const;
template bool Results_base::verifyColumn<DATE>(int column) const;
template bool Results_base::verifyColumn<INET>(int column) const;
template bool Results_base::verifyColumn<TEXT>(int column) const;
template bool Results_base::verifyColumn<WTEXT>(int column) const;
template bool Results_base::verifyColumn<ARRAY<char>>(int column) const;
template bool Results_base::verifyColumn<ARRAY<SMALLINT>>(int column) const;
template bool Results_base::verifyColumn<ARRAY<INTEGER>>(int column) const;
template bool Results_base::verifyColumn<ARRAY<BIGINT>>(int column) const;
template bool Results_base::verifyColumn<ARRAY<REAL>>(int column) const;
template bool Results_base::verifyColumn<ARRAY<DOUBLE_PRECISION>>(int column) const;
template bool Results_base::verifyColumn<ARRAY<TEXT>>(int column) const;
template bool Results_base::verifyColumn<ARRAY<WTEXT>>(int column) const;

// Non-array field return

template<typename Numeric> void Results_base::field(
        int row,
        int column,
        Numeric& value) const
{
    static_assert(
            std::is_integral<Numeric>::value ||
                std::is_floating_point<Numeric>::value,
            "Numeric must be a numeric type.");
    value = BigEndian<Numeric>::read(
            PQgetvalue(reinterpret_cast<const PGresult*>(m_res), row, column));
}
template void Results_base::field<SMALLINT>(
        int row,
        int column,
        SMALLINT& value) const;
template void Results_base::field<INTEGER>(
        int row,
        int column,
        INTEGER& value) const;
template void Results_base::field<BIGINT>(
        int row,
        int column,
        BIGINT& value) const;
template void Results_base::field<REAL>(
        int row,
        int column,
        REAL& value) const;
template void Results_base::field<DOUBLE_PRECISION>(
        int row,
        int column,
        DOUBLE_PRECISION& value) const;

// Non-array field return specializations

template<> void Results_base::field<BOOL>(
        int row,
        int column,
        BOOL& value) const
{
    value = static_cast<BOOL>(
            *PQgetvalue(reinterpret_cast<const PGresult*>(m_res), row, column));
}

template<> void Results_base::field<TEXT>(
        int row,
        int column,
        TEXT& value) const
{
    value.assign(
            PQgetvalue(reinterpret_cast<const PGresult*>(m_res), row, column),
            PQgetlength(reinterpret_cast<const PGresult*>(m_res), row, column));
}

template<> void Results_base::field<WTEXT>(
        int row,
        int column,
        WTEXT& value) const
{
    std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;
    const char* const start = PQgetvalue(
            reinterpret_cast<const PGresult*>(m_res),
            row,
            column);
    const char* const end = start+PQgetlength(
            reinterpret_cast<const PGresult*>(m_res),
            row,
            column);
    try
    {
        value = converter.from_bytes(start, end);
    }
    catch(const std::range_error& e)
    {
        WARNING_LOG("Error in code conversion from utf8 in SQL result")
    }
}

template<> void Results_base::field<TIMESTAMPTZ>(
          int row,
          int column,
          TIMESTAMPTZ& value) const
{
    using namespace std::chrono;
    constexpr TIMESTAMPTZ epoch(TIMESTAMPTZ() + seconds(946684800));
    const microseconds microseconds(
        BigEndian<BIGINT>::read(PQgetvalue(
                reinterpret_cast<const PGresult*>(m_res),
                row,
                column)));
    value = epoch + microseconds;
}

template<> void Results_base::field<DATE>(
          int row,
          int column,
          DATE& value) const
{
    using namespace std::chrono;
    constexpr DATE epoch(DATE() + days(10957));
    const days days(
        BigEndian<INTEGER>::read(PQgetvalue(
                reinterpret_cast<const PGresult*>(m_res),
                row,
                column)));
    value = epoch + days;
}

template<> void Results_base::field<INET>(
        int row,
        int column,
        Address& value) const
{
    char* address_p = reinterpret_cast<char*>(&value);
    const char* const data = PQgetvalue(
            reinterpret_cast<const PGresult*>(m_res),
            row,
            column);

    switch(PQgetlength(reinterpret_cast<const PGresult*>(m_res), row, column))
    {
        case 8:
        {
            address_p = std::fill_n(address_p, 10, char(0));
            address_p = std::fill_n(address_p, 2, char(-1));
            std::copy_n(
                    data+4,
                    4,
                    address_p);
            break;
        }
        case 20:
        {
            std::copy_n(data+4, Address::size, address_p);
            break;
        }
    }
}

// Array field returns

template<typename Numeric>
void Results_base::field(
        int row,
        int column,
        ARRAY<Numeric>& value) const
{
    static_assert(
            std::is_integral<Numeric>::value ||
                std::is_floating_point<Numeric>::value,
            "Numeric must be a numeric type.");
    const char* const start = PQgetvalue(
            reinterpret_cast<const PGresult*>(m_res),
            row,
            column);

    const auto ndim(*reinterpret_cast<const ARRAY_SIZE*>(
                start+0*sizeof(ARRAY_SIZE)));
    if(ndim != 1)
    {
        WARNING_LOG("SQL result array type for ARRAY<Numeric> has "\
                "ndim != 1");
        return;
    }

    const auto hasNull(*reinterpret_cast<const ARRAY_SIZE*>(
                start+1*sizeof(ARRAY_SIZE)));
    if(hasNull != 0)
    {
        WARNING_LOG("SQL result array type for ARRAY<Numeric> has "\
                "ndim != 0");
        return;
    }

    const auto elementType(*reinterpret_cast<const ARRAY_SIZE*>(
                start+2*sizeof(ARRAY_SIZE)));
    if(elementType != Traits<Numeric>::oid)
    {
        WARNING_LOG("SQL result array type for ARRAY<Numeric> has "\
                "the wrong element type");
        return;
    }

    const auto size(*reinterpret_cast<const ARRAY_SIZE*>(
                start+3*sizeof(ARRAY_SIZE)));

    value.clear();
    value.reserve(size);
    for(int i=0; i<size; ++i)
    {
        const auto length(
                *reinterpret_cast<const ARRAY_SIZE*>(
                    start + 5*sizeof(ARRAY_SIZE)
                    + i*(sizeof(ARRAY_SIZE) + sizeof(Numeric))));
        if(length != sizeof(Numeric))
        {
            WARNING_LOG("SQL result array for Numeric has element of wrong size");
            continue;
        }

        value.push_back(*reinterpret_cast<const BigEndian<Numeric>*>(
                    start + 6*sizeof(ARRAY_SIZE)
                    + i*(sizeof(ARRAY_SIZE) + sizeof(Numeric))));
    }
}
template void Results_base::field<SMALLINT>(
        int row,
        int column,
        ARRAY<SMALLINT>& value) const;
template void Results_base::field<INTEGER>(
        int row,
        int column,
        ARRAY<INTEGER>& value) const;
template void Results_base::field<BIGINT>(
        int row,
        int column,
        ARRAY<BIGINT>& value) const;
template void Results_base::field<REAL>(
        int row,
        int column,
        ARRAY<REAL>& value) const;
template void Results_base::field<DOUBLE_PRECISION>(
        int row,
        int column,
        ARRAY<DOUBLE_PRECISION>& value) const;

template<> void Results_base::field<char>(
        int row,
        int column,
        ARRAY<char>& value) const
{
    const unsigned size = PQgetlength(
            reinterpret_cast<const PGresult*>(m_res),
            row,
            column);
    const char* const start = PQgetvalue(
            reinterpret_cast<const PGresult*>(m_res),
            row,
            column);
    const char* const end = start+size;

    value.reserve(size);
    value.assign(start, end);
}

template<>
void Results_base::field<TEXT>(
        int row,
        int column,
        ARRAY<TEXT>& value) const
{
    const char* ptr = PQgetvalue(
            reinterpret_cast<const PGresult*>(m_res),
            row,
            column);

    const auto ndim(*reinterpret_cast<const ARRAY_SIZE*>(ptr));
    ptr += sizeof(ARRAY_SIZE);
    if(ndim != 1)
    {
        WARNING_LOG("SQL result array type for ARRAY<TEXT> has "\
                "ndim != 1");
        return;
    }

    const auto hasNull(*reinterpret_cast<const ARRAY_SIZE*>(ptr));
    ptr += sizeof(ARRAY_SIZE);
    if(hasNull != 0)
    {
        WARNING_LOG("SQL result array type for ARRAY<TEXT> has "\
                "ndim != 0");
        return;
    }

    const auto elementType(*reinterpret_cast<const ARRAY_SIZE*>(ptr));
    ptr += sizeof(ARRAY_SIZE);
    if(elementType != Traits<TEXT>::oid)
    {
        WARNING_LOG("SQL result array type for ARRAY<TEXT> has "\
                "the wrong element type");
        return;
    }

    const auto size(*reinterpret_cast<const ARRAY_SIZE*>(ptr));
    ptr += 2*sizeof(ARRAY_SIZE);

    value.clear();
    value.reserve(size);
    for(int i=0; i<size; ++i)
    {
        const auto length(*reinterpret_cast<const ARRAY_SIZE*>(ptr));
        ptr += sizeof(ARRAY_SIZE);

        value.emplace_back(ptr, length);
        ptr += length;
    }
}

template<>
void Results_base::field<WTEXT>(
        int row,
        int column,
        ARRAY<WTEXT>& value) const
{
    const char* ptr = PQgetvalue(
            reinterpret_cast<const PGresult*>(m_res),
            row,
            column);

    const auto ndim(*reinterpret_cast<const ARRAY_SIZE*>(ptr));
    ptr += sizeof(ARRAY_SIZE);
    if(ndim != 1)
    {
        WARNING_LOG("SQL result array type for ARRAY<WTEXT> has "\
                "ndim != 1");
        return;
    }

    const auto hasNull(*reinterpret_cast<const ARRAY_SIZE*>(ptr));
    ptr += sizeof(ARRAY_SIZE);
    if(hasNull != 0)
    {
        WARNING_LOG("SQL result array type for ARRAY<WTEXT> has "\
                "ndim != 0");
        return;
    }

    const auto elementType(*reinterpret_cast<const ARRAY_SIZE*>(ptr));
    ptr += sizeof(ARRAY_SIZE);
    if(elementType != Traits<TEXT>::oid)
    {
        WARNING_LOG("SQL result array type for ARRAY<WTEXT> has "\
                "the wrong element type");
        return;
    }

    const auto size(*reinterpret_cast<const ARRAY_SIZE*>(ptr));
    ptr += 2*sizeof(ARRAY_SIZE);

    value.clear();
    value.reserve(size);
    std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;
    try
    {
        for(int i=0; i<size; ++i)
        {
            const auto length(*reinterpret_cast<const ARRAY_SIZE*>(ptr));
            ptr += sizeof(ARRAY_SIZE);

            value.emplace_back(std::move(
                        converter.from_bytes(ptr, ptr+length)));
            ptr += length;
        }
    }
    catch(const std::range_error& e)
    {
        WARNING_LOG("Error in array code conversion to utf8 in SQL parameter")
    }
}

// Done result fields

Status Results_base::status() const
{
    static const std::map<ExecStatusType, Status> statuses{
        {PGRES_EMPTY_QUERY, Status::emptyQuery},
        {PGRES_COMMAND_OK, Status::commandOk},
        {PGRES_TUPLES_OK, Status::rowsOk},
        {PGRES_COPY_OUT, Status::copyOut},
        {PGRES_COPY_IN, Status::copyIn},
        {PGRES_BAD_RESPONSE, Status::badResponse},
        {PGRES_NONFATAL_ERROR, Status::nonfatalError},
        {PGRES_COPY_BOTH, Status::copyBoth},
        {PGRES_SINGLE_TUPLE, Status::singleTuple},
    };

    if(reinterpret_cast<const PGresult*>(m_res) == nullptr)
        return Status::noResult;

    const auto status = statuses.find(
            PQresultStatus(reinterpret_cast<const PGresult*>(m_res)));

    if(status == statuses.end())
        return Status::fatalError;

    return status->second;
}

unsigned Results_base::affectedRows() const
{
    return std::atoi(PQcmdTuples(reinterpret_cast<PGresult*>(m_res)));
}

Results_base::~Results_base()
{
    if(m_res != nullptr)
        PQclear(reinterpret_cast<PGresult*>(m_res));
}

const char* Results_base::errorMessage() const
{
    return PQresultErrorMessage(reinterpret_cast<const PGresult*>(m_res));
}

unsigned Results_base::rows() const
{
    return PQntuples(reinterpret_cast<const PGresult*>(m_res));
}

bool Results_base::null(int row, int column) const
{
    return static_cast<bool>(PQgetisnull(
                reinterpret_cast<const PGresult*>(m_res),
                row,
                column));
}

int Results_base::columns() const
{
    return PQnfields(reinterpret_cast<const PGresult*>(m_res));
}

const char* Fastcgipp::SQL::statusString(const Status status)
{
    static const std::map<Status, const char*> statuses{
        {Status::noResult, "No Result"},
        {Status::emptyQuery, "Empty Query"},
        {Status::commandOk, "Command OK"},
        {Status::rowsOk, "Rows OK"},
        {Status::copyOut, "Copy Out"},
        {Status::copyIn, "Copy In"},
        {Status::badResponse, "Bad Response"},
        {Status::nonfatalError, "Non-fatal Error"},
        {Status::copyBoth, "Copy Both"},
        {Status::singleTuple, "Single Tuple"},
        {Status::fatalError, "Fatal Error"},
    };

    auto it = statuses.find(status);

    if(it == statuses.end())
        it = statuses.find(Status::fatalError);

    return it->second;
}
