#include <iostream>
#include <memory>
#include <algorithm>
#include <array>

#include "fastcgi++/sql/results.hpp"
#include "fastcgi++/sql/parameters.hpp"

int main()
{
    // Test the SQL parameters stuff
    {
        static const int16_t zero = -1413;
        static const int32_t one = 123342945;
        static const int64_t two = -123342945112312323;
        static const std::string three = "This is a test!!34234";
        static const float four = -1656e-8;
        static const double five = 2354e15;

        struct Six
        {
            int one;
            int two;
        };
        static const Six six{2006, 2017};

        typedef std::array<char, 6> Seven;
        static const Seven seven{'a', 'b', 'c', 'd', 'e', 'f'};

        static const std::wstring eight(L"インターネット");
        static const std::array<unsigned char, 21> properEight =
        {
            0xe3, 0x82, 0xa4, 0xe3, 0x83, 0xb3, 0xe3, 0x82, 0xbf, 0xe3, 0x83,
            0xbc, 0xe3, 0x83, 0x8d, 0xe3, 0x83, 0x83, 0xe3, 0x83, 0x88
        };

        typedef Fastcgipp::SQL::Parameters<
            int16_t,
            int32_t,
            int64_t,
            std::string,
            float,
            double,
            Six,
            Seven,
            std::string> Row;
        std::shared_ptr<Row> data(new Row(
                zero,
                one,
                two,
                three,
                four,
                five,
                six,
                seven,
                eight));
        std::shared_ptr<Fastcgipp::SQL::Parameters_base> base(data);
        data.reset();

        base->build();

        if(
                *(base->oids()+0) != INT2OID ||
                *(base->sizes()+0) != 2 ||
                Fastcgipp::Protocol::BigEndian<int16_t>::read(
                    *(base->raws()+0)) != zero)
            FAIL_LOG("Fastcgipp::SQL::Parameters failed on column 0")
        if(
                *(base->oids()+1) != INT4OID ||
                *(base->sizes()+1) != 4 ||
                Fastcgipp::Protocol::BigEndian<int32_t>::read(
                    *(base->raws()+1)) != one)
            FAIL_LOG("Fastcgipp::SQL::Parameters failed on column 1")
        if(
                *(base->oids()+2) != INT8OID ||
                *(base->sizes()+2) != 8 ||
                Fastcgipp::Protocol::BigEndian<int64_t>::read(
                    *(base->raws()+2)) != two)
            FAIL_LOG("Fastcgipp::SQL::Parameters failed on column 2")
        if(
                *(base->oids()+3) != TEXTOID ||
                *(base->sizes()+3) != three.size() ||
                !std::equal(
                    three.begin(),
                    three.end(),
                    *(base->raws()+3),
                    *(base->raws()+3)+*(base->sizes()+3)))
            FAIL_LOG("Fastcgipp::SQL::Parameters failed on column 3")
        if(
                *(base->oids()+4) != FLOAT4OID ||
                *(base->sizes()+4) != 4 ||
                Fastcgipp::Protocol::BigEndian<float>::read(
                    *(base->raws()+4)) != four)
            FAIL_LOG("Fastcgipp::SQL::Parameters failed on column 4")
        if(
                *(base->oids()+5) != FLOAT8OID ||
                *(base->sizes()+5) != 8 ||
                Fastcgipp::Protocol::BigEndian<double>::read(
                    *(base->raws()+5)) != five)
            FAIL_LOG("Fastcgipp::SQL::Parameters failed on column 5")
        if(
                *(base->oids()+6) != BYTEAOID ||
                *(base->sizes()+6) != sizeof(Six) ||
                reinterpret_cast<const Six*>(*(base->raws()+6))->one != six.one ||
                reinterpret_cast<const Six*>(*(base->raws()+6))->two != six.two)
            FAIL_LOG("Fastcgipp::SQL::Parameters failed on column 6")
        if(
                *(base->oids()+7) != BYTEAOID ||
                *(base->sizes()+7) != sizeof(Seven) ||
                *reinterpret_cast<const Seven*>(*(base->raws()+7)) != seven)
            FAIL_LOG("Fastcgipp::SQL::Parameters failed on column 7")
        if(
                *(base->oids()+8) != TEXTOID ||
                *(base->sizes()+8) != properEight.size() ||
                !std::equal(
                    reinterpret_cast<const char*>(properEight.begin()),
                    reinterpret_cast<const char*>(properEight.end()),
                    *(base->raws()+8),
                    *(base->raws()+8)+*(base->sizes()+8)))
            FAIL_LOG("Fastcgipp::SQL::Parameters failed on column 8")
    }

    // Test the SQL results stuff
    {
        Fastcgipp::SQL::Results<int16_t, int32_t, int64_t, std::string> results;
        results.doNothing();
    }

    return 0;
}
