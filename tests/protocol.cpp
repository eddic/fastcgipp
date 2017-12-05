#include "fastcgi++/log.hpp"
#include "fastcgi++/protocol.hpp"

#include <random>
#include <memory>
#include <cstdint>

int main()
{
    // Testing Fastcgipp::Protocol::BigEndian with a 64 bit signed integer
    {
        const int64_t actual = -0x62c74ce376736dd0;
        const Fastcgipp::Protocol::BigEndian<int64_t> reversed(actual);
        const unsigned char* data = reinterpret_cast<const unsigned char*>(
                &reversed);

        if(!(
                    reversed == actual &&
                    data[0] == 0x9d &&
                    data[1] == 0x38 &&
                    data[2] == 0xb3 &&
                    data[3] == 0x1c &&
                    data[4] == 0x89 &&
                    data[5] == 0x8c &&
                    data[6] == 0x92 &&
                    data[7] == 0x30))
            FAIL_LOG("Fastcgipp::Protocol::BigEndian with a 64 bit signed int")
    }

    // Testing Fastcgipp::Protocol::BigEndian with a 16 bit unsigned integer
    {
        const uint16_t actual = 57261;
        const Fastcgipp::Protocol::BigEndian<uint16_t> reversed(actual);
        const unsigned char* data = reinterpret_cast<const unsigned char*>(
                &reversed);

        if(!(
                    reversed == actual &&
                    data[0] == 0xdf &&
                    data[1] == 0xad))
            FAIL_LOG("Fastcgipp::Protocol::BigEndian with a 16 bit unsigned int")
    }

    // Testing Fastcgipp::Protocol::BigEndian with a 32 bit float
    {
        const float actual = -3.21748e-05;
        const Fastcgipp::Protocol::BigEndian<float> reversed(actual);
        const unsigned char* data = reinterpret_cast<const unsigned char*>(
                &reversed);

        if(!(
                    reversed == actual &&
                    data[0] == 0xb8 &&
                    data[1] == 0x06 &&
                    data[2] == 0xf3 &&
                    data[3] == 0x6e))
            FAIL_LOG("Fastcgipp::Protocol::BigEndian with a 32 bit float")
    }

    // Testing Fastcgipp::Protocol::BigEndian with a 64 bit float
    {
        const double actual = 8.854187817e-12;
        const Fastcgipp::Protocol::BigEndian<double> reversed(actual);
        const unsigned char* data = reinterpret_cast<const unsigned char*>(
                &reversed);

        if(!(
                    reversed == actual &&
                    data[0] == 0x3d &&
                    data[1] == 0xa3 &&
                    data[2] == 0x78 &&
                    data[3] == 0x76 &&
                    data[4] == 0xf1 &&
                    data[5] == 0x48 &&
                    data[6] == 0x11 &&
                    data[7] == 0x2e))
            FAIL_LOG("Fastcgipp::Protocol::BigEndian with a 64 bit float")
    }

    std::random_device device;
    std::default_random_engine engine(device());
    std::uniform_int_distribution<size_t> randomShortSize(1, 127);
    std::uniform_int_distribution<size_t> randomLongSize(128, 10000000);

    // Testing Fastcgipp::Protocol::processParamHeader() with short values and
    // short names
    for(int i=0; i<10; ++i)
    {
        const size_t nameSize = randomShortSize(engine);
        const size_t valueSize = randomShortSize(engine);
        const size_t dataSize = 2+nameSize+valueSize;
        std::unique_ptr<char[]> body(new char[dataSize]);
        const char* const bodyStart = body.get();
        const char* const bodyEnd = bodyStart+dataSize;
        const char* const name = bodyStart+2;
        const char* const value = name+nameSize;
        const char* const end = value+valueSize;
        body[0] = static_cast<char>(nameSize);
        body[1] = static_cast<char>(valueSize);

        const char* nameResult;
        const char* valueResult;
        const char* endResult;

        const char* const passedEnd = i<5?bodyStart+i:bodyEnd-i;

        const bool retVal = Fastcgipp::Protocol::processParamHeader(
                bodyStart,
                passedEnd,
                nameResult,
                valueResult,
                endResult);

        if(((passedEnd==bodyEnd)!=retVal) || (retVal && !(
                nameResult == name &&
                valueResult == value &&
                endResult == end)))
            FAIL_LOG("Fastcgipp::Protocol::processParamHeader with short "\
                    "values and short names")
    }

    // Testing Fastcgii::Protocol::processParamHeader() with long values and
    // short names
    for(int i=0; i<100; ++i)
    {
        const size_t nameSize = randomShortSize(engine);
        const size_t valueSize = randomLongSize(engine);
        const size_t dataSize = 5+nameSize+valueSize;
        std::unique_ptr<char[]> body(new char[dataSize]);
        const char* const bodyStart = body.get();
        const char* const bodyEnd = bodyStart+dataSize;
        const char* const name = bodyStart+5;
        const char* const value = name+nameSize;
        const char* const end = value+valueSize;
        body[0] = static_cast<char>(nameSize);
        *reinterpret_cast<Fastcgipp::Protocol::BigEndian<int32_t>*>(&body[1])
            = static_cast<uint32_t>(valueSize);
        body[1] |= 0x80;

        const char* nameResult;
        const char* valueResult;
        const char* endResult;

        const char* const passedEnd = i<50?bodyStart+i:bodyEnd-i;

        const bool retVal = Fastcgipp::Protocol::processParamHeader(
                bodyStart,
                passedEnd,
                nameResult,
                valueResult,
                endResult);

        if(((passedEnd==bodyEnd)!=retVal) || (retVal && !(
                nameResult == name &&
                valueResult == value &&
                endResult == end)))
            FAIL_LOG("Fastcgipp::Protocol::processParamHeader with long "\
                    "values and short names")
    }

    // Testing Fastcgii::Protocol::processParamHeader() with short values and
    // long names
    for(int i=0; i<10; ++i)
    {
        const size_t nameSize = randomLongSize(engine);
        const size_t valueSize = randomShortSize(engine);
        const size_t dataSize = 5+nameSize+valueSize;
        std::unique_ptr<char[]> body(new char[dataSize]);
        const char* const bodyStart = body.get();
        const char* const bodyEnd = bodyStart+dataSize;
        const char* const name = bodyStart+5;
        const char* const value = name+nameSize;
        const char* const end = value+valueSize;
        *reinterpret_cast<Fastcgipp::Protocol::BigEndian<int32_t>*>(body.get())
            = static_cast<uint32_t>(nameSize);
        body[0] |= 0x80;
        body[4] = static_cast<char>(valueSize);

        const char* nameResult;
        const char* valueResult;
        const char* endResult;

        const char* const passedEnd = i<50?bodyStart+i:bodyEnd-i;

        const bool retVal = Fastcgipp::Protocol::processParamHeader(
                bodyStart,
                passedEnd,
                nameResult,
                valueResult,
                endResult);

        if(((passedEnd==bodyEnd)!=retVal) || (retVal && !(
                nameResult == name &&
                valueResult == value &&
                endResult == end)))
            FAIL_LOG("Fastcgipp::Protocol::processParamHeader with short "\
                    "values and long names")
    }

    // Testing Fastcgii::Protocol::processParamHeader() with long values and
    // long names
    for(int i=0; i<10; ++i)
    {
        const size_t nameSize = randomLongSize(engine);
        const size_t valueSize = randomLongSize(engine);
        const size_t dataSize = 8+nameSize+valueSize;
        std::unique_ptr<char[]> body(new char[dataSize]);
        const char* const bodyStart = body.get();
        const char* const bodyEnd = bodyStart+dataSize;
        const char* const name = bodyStart+8;
        const char* const value = name+nameSize;
        const char* const end = value+valueSize;
        *reinterpret_cast<Fastcgipp::Protocol::BigEndian<int32_t>*>(body.get())
            = static_cast<uint32_t>(nameSize);
        body[0] |= 0x80;
        *reinterpret_cast<Fastcgipp::Protocol::BigEndian<int32_t>*>(&body[4])
            = static_cast<uint32_t>(valueSize);
        body[4] |= 0x80;

        const char* nameResult;
        const char* valueResult;
        const char* endResult;

        const char* const passedEnd = i<50?bodyStart+i:bodyEnd-i;

        const bool retVal = Fastcgipp::Protocol::processParamHeader(
                bodyStart,
                passedEnd,
                nameResult,
                valueResult,
                endResult);

        if(((passedEnd==bodyEnd)!=retVal) || (retVal && !(
                nameResult == name &&
                valueResult == value &&
                endResult == end)))
            FAIL_LOG("Fastcgipp::Protocol::processParamHeader with long "\
                    "values and long names")
    }

    return 0;
}
