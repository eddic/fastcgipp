#include "fastcgi++/log.hpp"
#include "fastcgi++/http.hpp"

#include <list>
#include <array>
#include <sstream>
#include <algorithm>
#include <string>
#include <map>
#include <thread>
#include <chrono>
#include <random>
#include <cstring>

int main()
{
    // Test Fastcgipp::Http::Address
    {
        const unsigned char randomAddress1Data[Fastcgipp::Http::Address::size] =
        {
            0xcc, 0x22, 0x40, 0x08, 0x79, 0xa1, 0xc1, 0x78, 0x05, 0xc5, 0x88,
            0x2a, 0x19, 0x0d, 0x7f, 0xbf
        };
        const char randomAddress1String[] =
            "cc22:4008:79a1:c178:5c5:882a:190d:7fbf";
        const Fastcgipp::Http::Address randomAddress1(randomAddress1Data);

        const unsigned char randomAddress2Data[Fastcgipp::Http::Address::size] =
        {
            0xce, 0x9c, 0x51, 0x16, 0x78, 0x17, 0x00, 0x00, 0x00, 0x00, 0x8d,
            0x97, 0x00, 0x00, 0xe7, 0x55
        };
        const char randomAddress2String[] = "ce9c:5116:7817::8d97:0:e755";
        const Fastcgipp::Http::Address randomAddress2(randomAddress2Data);

        const unsigned char ipv4AddressData[Fastcgipp::Http::Address::size] =
        {
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff,
            0xff, 0xb3, 0x7c, 0x83, 0x91
        };
        const char ipv4AddressStringNew[] = "::ffff:179.124.131.145";
        const char ipv4AddressStringOld[] = "179.124.131.145";
        const Fastcgipp::Http::Address ipv4Address(ipv4AddressData);

        const char badAddressString1[] =
            "cc22:4008:79a1:c178:5y5:882a:190d:7fbf";
        const char badAddressString2[] =
            "cc22:4008:79a1:c178:5c5:190d:7fbf";

        std::list<Fastcgipp::Http::Address> correctAddresses;
        correctAddresses.push_back(ipv4Address);
        correctAddresses.push_back(randomAddress1);
        correctAddresses.push_back(randomAddress2);

        // Test assign()
        {
            Fastcgipp::Http::Address address;

            address.assign(
                    randomAddress1String,
                    randomAddress1String+sizeof(randomAddress1String)-1);
            if(address != randomAddress1)
                FAIL_LOG("Fastcgipp::Http::Address::assign() with randomAddress1")

            address.assign(
                    randomAddress2String,
                    randomAddress2String+sizeof(randomAddress2String)-1);
            if(address != randomAddress2)
                FAIL_LOG("Fastcgipp::Http::Address::assign() with randomAddress2")

            address.assign(
                    ipv4AddressStringNew,
                    ipv4AddressStringNew+sizeof(ipv4AddressStringNew)-1);
            if(address != ipv4Address)
                FAIL_LOG("Fastcgipp::Http::Address::assign() with ipv4 new style")

            address.assign(
                    ipv4AddressStringOld,
                    ipv4AddressStringOld+sizeof(ipv4AddressStringOld)-1);
            if(address != ipv4Address)
                FAIL_LOG("Fastcgipp::Http::Address::assign() with ipv4 old style")

            Fastcgipp::Logging::suppress=true;
            address.assign(
                    badAddressString1,
                    badAddressString1+sizeof(badAddressString1)-1);
            if(address)
                FAIL_LOG("Fastcgipp::Http::Address::assign() badAddress1")

            address.assign(
                    badAddressString2,
                    badAddressString2+sizeof(badAddressString2)-1);
            if(address)
                FAIL_LOG("Fastcgipp::Http::Address::assign() badAddress1")
            Fastcgipp::Logging::suppress=false;
        }

        // Test stream insertion
        {
            std::ostringstream ss;

            ss << randomAddress1;
            if(ss.str() != randomAddress1String)
                FAIL_LOG(\
                        "Fastcgipp::Http::Address stream insertion with"\
                        " randomAddress1. Got " << ss.str().c_str())

            ss.str("");
            ss << randomAddress2;
            if(ss.str() != randomAddress2String)
                FAIL_LOG(\
                        "Fastcgipp::Http::Address stream insertion with"\
                        " randomAddress2. Got " << ss.str().c_str())

            ss.str("");
            ss << ipv4Address;
            if(ss.str() != ipv4AddressStringNew)
                FAIL_LOG(\
                        "Fastcgipp::Http::Address stream insertion with"\
                        " ipv4Address. Got " << ss.str().c_str())
        }

        // Testing stream extraction
        {
            std::istringstream ss;
            Fastcgipp::Http::Address address;

            ss.clear();
            ss.str(randomAddress1String);
            ss >> address;
            if(address != randomAddress1)
                FAIL_LOG(\
                        "Fastcgipp::Http::Address stream extraction with"\
                        " randomAddress1. Got " << ss.str().c_str())

            ss.clear();
            ss.str(randomAddress2String);
            ss >> address;
            if(address != randomAddress2)
                FAIL_LOG(\
                        "Fastcgipp::Http::Address stream extraction with"\
                        " randomAddress2. Got " << ss.str().c_str())

            ss.clear();
            ss.str(ipv4AddressStringNew);
            ss >> address;
            if(address != ipv4Address)
                FAIL_LOG(\
                        "Fastcgipp::Http::Address stream extraction with"\
                        " ipv4Address new style. Got " << ss.str().c_str())

            ss.clear();
            ss.str(ipv4AddressStringOld);
            ss >> address;
            if(address != ipv4Address)
                FAIL_LOG(\
                        "Fastcgipp::Http::Address stream extraction with"\
                        " ipv4Address old style. Got " << ss.str().c_str())

            address.zero();
            ss.clear();
            ss.str(badAddressString1);
            ss >> address;
            if(address)
                FAIL_LOG(\
                        "Fastcgipp::Http::Address stream extraction with"\
                        " badAddress1. Got " << ss.str().c_str())

            ss.clear();
            ss.str(badAddressString2);
            ss >> address;
            if(address)
                FAIL_LOG(\
                        "Fastcgipp::Http::Address stream extraction with"\
                        " badAddress2. Got " << ss.str().c_str())
        }

        // Testing sorting abilities
        {
            std::list<Fastcgipp::Http::Address> addresses;
            addresses.push_back(randomAddress1);
            addresses.push_back(ipv4Address);
            addresses.push_back(randomAddress2);

            addresses.sort();

            if(addresses != correctAddresses)
                FAIL_LOG("Fastcgipp::Http::Address sorting")
        }
    }

    // Test base64 encoding/decoding stuff
    {
        const char string1[] =
            "ltG5tYELSwWdsqMJO+5vYCIjF5YduP0un4vohTdyieHCYXtK4dEk9UKoXGxl6lDAlQ"
            "qtH1xfrU46wnxoGGhxp8A+r9GsJDUQfTliZBBvgMLSQkN8iRGIzjgGm6ngfWfYW9VZ"
            "1tmEukDARgGivXftB6Nnke3kgW+b9m/gYpDWJb+ZkC0TY0fnQ8xnWBljF7piPsc4hT"
            "+vsxwrUcTmp6JGwlRtAbPxQuM1BFjdcJ5eDLVXcPQby3JjBKi3zniaygHS9RkhG+a4"
            "qfjxcZHO6ahZoNYMOiG0nev/00guZB5cuP1vjxHYB6RnEvLK29mYhGqu5fs3R5odiJ"
            "+QKgfHNWhgNgN2";

        const unsigned char data1[258] =
        {
            0x96, 0xd1, 0xb9, 0xb5, 0x81, 0x0b, 0x4b, 0x05, 0x9d, 0xb2, 0xa3,
            0x09, 0x3b, 0xee, 0x6f, 0x60, 0x22, 0x23, 0x17, 0x96, 0x1d, 0xb8,
            0xfd, 0x2e, 0x9f, 0x8b, 0xe8, 0x85, 0x37, 0x72, 0x89, 0xe1, 0xc2,
            0x61, 0x7b, 0x4a, 0xe1, 0xd1, 0x24, 0xf5, 0x42, 0xa8, 0x5c, 0x6c,
            0x65, 0xea, 0x50, 0xc0, 0x95, 0x0a, 0xad, 0x1f, 0x5c, 0x5f, 0xad,
            0x4e, 0x3a, 0xc2, 0x7c, 0x68, 0x18, 0x68, 0x71, 0xa7, 0xc0, 0x3e,
            0xaf, 0xd1, 0xac, 0x24, 0x35, 0x10, 0x7d, 0x39, 0x62, 0x64, 0x10,
            0x6f, 0x80, 0xc2, 0xd2, 0x42, 0x43, 0x7c, 0x89, 0x11, 0x88, 0xce,
            0x38, 0x06, 0x9b, 0xa9, 0xe0, 0x7d, 0x67, 0xd8, 0x5b, 0xd5, 0x59,
            0xd6, 0xd9, 0x84, 0xba, 0x40, 0xc0, 0x46, 0x01, 0xa2, 0xbd, 0x77,
            0xed, 0x07, 0xa3, 0x67, 0x91, 0xed, 0xe4, 0x81, 0x6f, 0x9b, 0xf6,
            0x6f, 0xe0, 0x62, 0x90, 0xd6, 0x25, 0xbf, 0x99, 0x90, 0x2d, 0x13,
            0x63, 0x47, 0xe7, 0x43, 0xcc, 0x67, 0x58, 0x19, 0x63, 0x17, 0xba,
            0x62, 0x3e, 0xc7, 0x38, 0x85, 0x3f, 0xaf, 0xb3, 0x1c, 0x2b, 0x51,
            0xc4, 0xe6, 0xa7, 0xa2, 0x46, 0xc2, 0x54, 0x6d, 0x01, 0xb3, 0xf1,
            0x42, 0xe3, 0x35, 0x04, 0x58, 0xdd, 0x70, 0x9e, 0x5e, 0x0c, 0xb5,
            0x57, 0x70, 0xf4, 0x1b, 0xcb, 0x72, 0x63, 0x04, 0xa8, 0xb7, 0xce,
            0x78, 0x9a, 0xca, 0x01, 0xd2, 0xf5, 0x19, 0x21, 0x1b, 0xe6, 0xb8,
            0xa9, 0xf8, 0xf1, 0x71, 0x91, 0xce, 0xe9, 0xa8, 0x59, 0xa0, 0xd6,
            0x0c, 0x3a, 0x21, 0xb4, 0x9d, 0xeb, 0xff, 0xd3, 0x48, 0x2e, 0x64,
            0x1e, 0x5c, 0xb8, 0xfd, 0x6f, 0x8f, 0x11, 0xd8, 0x07, 0xa4, 0x67,
            0x12, 0xf2, 0xca, 0xdb, 0xd9, 0x98, 0x84, 0x6a, 0xae, 0xe5, 0xfb,
            0x37, 0x47, 0x9a, 0x1d, 0x88, 0x9f, 0x90, 0x2a, 0x07, 0xc7, 0x35,
            0x68, 0x60, 0x36, 0x03, 0x76
        };

        const char string2[] =
            "J7HnFFjNaN8pNQbYKGpmZFbWHe6kxO0odRpic7d5tUkZ7egpEYaMxpROrDRettPhNw"
            "9cfQ21kRq6l5keGUA9S8C7S+pod8POUBVFHYIJTGHwVzr1RWpPzUdB4w8qc548dvkA"
            "NrM08to4P/3UvyOXDG0Wbqz4h4OH/P7knu/BitK3JDTBus4/kP0hi1MWWKKrr6NPTt"
            "nwaq2b7yqidlm7K9wI3bCHFWREfDZvIRmUD3rTaQx67Xn3cAB7XKzrhGxS1w==";
        const unsigned char data2[193] =
        {
            0x27, 0xb1, 0xe7, 0x14, 0x58, 0xcd, 0x68, 0xdf, 0x29, 0x35, 0x06,
            0xd8, 0x28, 0x6a, 0x66, 0x64, 0x56, 0xd6, 0x1d, 0xee, 0xa4, 0xc4,
            0xed, 0x28, 0x75, 0x1a, 0x62, 0x73, 0xb7, 0x79, 0xb5, 0x49, 0x19,
            0xed, 0xe8, 0x29, 0x11, 0x86, 0x8c, 0xc6, 0x94, 0x4e, 0xac, 0x34,
            0x5e, 0xb6, 0xd3, 0xe1, 0x37, 0x0f, 0x5c, 0x7d, 0x0d, 0xb5, 0x91,
            0x1a, 0xba, 0x97, 0x99, 0x1e, 0x19, 0x40, 0x3d, 0x4b, 0xc0, 0xbb,
            0x4b, 0xea, 0x68, 0x77, 0xc3, 0xce, 0x50, 0x15, 0x45, 0x1d, 0x82,
            0x09, 0x4c, 0x61, 0xf0, 0x57, 0x3a, 0xf5, 0x45, 0x6a, 0x4f, 0xcd,
            0x47, 0x41, 0xe3, 0x0f, 0x2a, 0x73, 0x9e, 0x3c, 0x76, 0xf9, 0x00,
            0x36, 0xb3, 0x34, 0xf2, 0xda, 0x38, 0x3f, 0xfd, 0xd4, 0xbf, 0x23,
            0x97, 0x0c, 0x6d, 0x16, 0x6e, 0xac, 0xf8, 0x87, 0x83, 0x87, 0xfc,
            0xfe, 0xe4, 0x9e, 0xef, 0xc1, 0x8a, 0xd2, 0xb7, 0x24, 0x34, 0xc1,
            0xba, 0xce, 0x3f, 0x90, 0xfd, 0x21, 0x8b, 0x53, 0x16, 0x58, 0xa2,
            0xab, 0xaf, 0xa3, 0x4f, 0x4e, 0xd9, 0xf0, 0x6a, 0xad, 0x9b, 0xef,
            0x2a, 0xa2, 0x76, 0x59, 0xbb, 0x2b, 0xdc, 0x08, 0xdd, 0xb0, 0x87,
            0x15, 0x64, 0x44, 0x7c, 0x36, 0x6f, 0x21, 0x19, 0x94, 0x0f, 0x7a,
            0xd3, 0x69, 0x0c, 0x7a, 0xed, 0x79, 0xf7, 0x70, 0x00, 0x7b, 0x5c,
            0xac, 0xeb, 0x84, 0x6c, 0x52, 0xd7
        };

        const char string3[] =
            "EA/b88j/QeYddtvNqehi4kk7Kcc1hV3QNTnrhMZKjqaCi9yQNKS/wRsS8JMRXIotuF"
            "QQyNMHsFISLha81nDOisbEvqZEVu21zcnzZM0lKiWjsH64/183a8b/1ULZjVo/QI23"
            "BSXMUAKSDLT+LYXy3u4m64e7c/OHi4EzKCWN3thORofRkp4MpFRupfUoB9FEhCpnSX"
            "fFpugQD3c8TEErsDhGkx2gLUu8FDdBBkWIVlW7CFqgYLkymEdLYC7gAr/UuSwcGev0"
            "9ikua7rHVcWh5UYxrmNBY4XqMQp7ZVQmJ6JqnNFAF7F3b43mIkK5/zTPwgws67t2dn"
            "knssXkJGCUlzXxTrPbUn7GXXn/KRn6tQ3kKEvP5ys81clDSvg0iMdszqvLtlDoLyfY"
            "xvYLIC6YYB4GMfoNDzWTNnyEmSqLKoUOcfpbeWmF9uf6nqajWARzmz/kitRBXSwKAm"
            "4yQupzb70pMdsn/DAxgYQ=";
        const unsigned char data3[362] =
        {
            0x10, 0x0f, 0xdb, 0xf3, 0xc8, 0xff, 0x41, 0xe6, 0x1d, 0x76, 0xdb,
            0xcd, 0xa9, 0xe8, 0x62, 0xe2, 0x49, 0x3b, 0x29, 0xc7, 0x35, 0x85,
            0x5d, 0xd0, 0x35, 0x39, 0xeb, 0x84, 0xc6, 0x4a, 0x8e, 0xa6, 0x82,
            0x8b, 0xdc, 0x90, 0x34, 0xa4, 0xbf, 0xc1, 0x1b, 0x12, 0xf0, 0x93,
            0x11, 0x5c, 0x8a, 0x2d, 0xb8, 0x54, 0x10, 0xc8, 0xd3, 0x07, 0xb0,
            0x52, 0x12, 0x2e, 0x16, 0xbc, 0xd6, 0x70, 0xce, 0x8a, 0xc6, 0xc4,
            0xbe, 0xa6, 0x44, 0x56, 0xed, 0xb5, 0xcd, 0xc9, 0xf3, 0x64, 0xcd,
            0x25, 0x2a, 0x25, 0xa3, 0xb0, 0x7e, 0xb8, 0xff, 0x5f, 0x37, 0x6b,
            0xc6, 0xff, 0xd5, 0x42, 0xd9, 0x8d, 0x5a, 0x3f, 0x40, 0x8d, 0xb7,
            0x05, 0x25, 0xcc, 0x50, 0x02, 0x92, 0x0c, 0xb4, 0xfe, 0x2d, 0x85,
            0xf2, 0xde, 0xee, 0x26, 0xeb, 0x87, 0xbb, 0x73, 0xf3, 0x87, 0x8b,
            0x81, 0x33, 0x28, 0x25, 0x8d, 0xde, 0xd8, 0x4e, 0x46, 0x87, 0xd1,
            0x92, 0x9e, 0x0c, 0xa4, 0x54, 0x6e, 0xa5, 0xf5, 0x28, 0x07, 0xd1,
            0x44, 0x84, 0x2a, 0x67, 0x49, 0x77, 0xc5, 0xa6, 0xe8, 0x10, 0x0f,
            0x77, 0x3c, 0x4c, 0x41, 0x2b, 0xb0, 0x38, 0x46, 0x93, 0x1d, 0xa0,
            0x2d, 0x4b, 0xbc, 0x14, 0x37, 0x41, 0x06, 0x45, 0x88, 0x56, 0x55,
            0xbb, 0x08, 0x5a, 0xa0, 0x60, 0xb9, 0x32, 0x98, 0x47, 0x4b, 0x60,
            0x2e, 0xe0, 0x02, 0xbf, 0xd4, 0xb9, 0x2c, 0x1c, 0x19, 0xeb, 0xf4,
            0xf6, 0x29, 0x2e, 0x6b, 0xba, 0xc7, 0x55, 0xc5, 0xa1, 0xe5, 0x46,
            0x31, 0xae, 0x63, 0x41, 0x63, 0x85, 0xea, 0x31, 0x0a, 0x7b, 0x65,
            0x54, 0x26, 0x27, 0xa2, 0x6a, 0x9c, 0xd1, 0x40, 0x17, 0xb1, 0x77,
            0x6f, 0x8d, 0xe6, 0x22, 0x42, 0xb9, 0xff, 0x34, 0xcf, 0xc2, 0x0c,
            0x2c, 0xeb, 0xbb, 0x76, 0x76, 0x79, 0x27, 0xb2, 0xc5, 0xe4, 0x24,
            0x60, 0x94, 0x97, 0x35, 0xf1, 0x4e, 0xb3, 0xdb, 0x52, 0x7e, 0xc6,
            0x5d, 0x79, 0xff, 0x29, 0x19, 0xfa, 0xb5, 0x0d, 0xe4, 0x28, 0x4b,
            0xcf, 0xe7, 0x2b, 0x3c, 0xd5, 0xc9, 0x43, 0x4a, 0xf8, 0x34, 0x88,
            0xc7, 0x6c, 0xce, 0xab, 0xcb, 0xb6, 0x50, 0xe8, 0x2f, 0x27, 0xd8,
            0xc6, 0xf6, 0x0b, 0x20, 0x2e, 0x98, 0x60, 0x1e, 0x06, 0x31, 0xfa,
            0x0d, 0x0f, 0x35, 0x93, 0x36, 0x7c, 0x84, 0x99, 0x2a, 0x8b, 0x2a,
            0x85, 0x0e, 0x71, 0xfa, 0x5b, 0x79, 0x69, 0x85, 0xf6, 0xe7, 0xfa,
            0x9e, 0xa6, 0xa3, 0x58, 0x04, 0x73, 0x9b, 0x3f, 0xe4, 0x8a, 0xd4,
            0x41, 0x5d, 0x2c, 0x0a, 0x02, 0x6e, 0x32, 0x42, 0xea, 0x73, 0x6f,
            0xbd, 0x29, 0x31, 0xdb, 0x27, 0xfc, 0x30, 0x31, 0x81, 0x84,
        };

        std::array<char, 1024> string;
        // Test Fastcgipp::Http::base64Encode()
        {
            auto end = Fastcgipp::Http::base64Encode(
                    data1,
                    data1+sizeof(data1),
                    string.begin());

            if(!std::equal(
                        string1,
                        string1+sizeof(string1)-1,
                        string.begin(),
                        end))
                FAIL_LOG("Fastcgipp::Http::base64Encode() with data1. Got \"" \
                        << std::wstring(string.begin(), end) << "\"")

            end = Fastcgipp::Http::base64Encode(
                    data2,
                    data2+sizeof(data2),
                    string.begin());

            if(!std::equal(
                        string2,
                        string2+sizeof(string2)-1,
                        string.begin(),
                        end))
                FAIL_LOG("Fastcgipp::Http::base64Encode() with data2. Got \"" \
                        << std::wstring(string.begin(), end) << "\"")

            end = Fastcgipp::Http::base64Encode(
                    data3,
                    data3+sizeof(data3),
                    string.begin());

            if(!std::equal(
                        string3,
                        string3+sizeof(string3)-1,
                        string.begin(),
                        end))
                FAIL_LOG("Fastcgipp::Http::base64Encode() with data3. Got \"" \
                        << std::wstring(string.begin(), end) << "\"")
        }

        std::array<unsigned char, 1024> data;
        // Test Fastcgipp::Http::base64Decode()
        {
            auto end = Fastcgipp::Http::base64Decode(
                    string1,
                    string1+sizeof(string1)-1,
                    data.begin());

            if(!std::equal(
                        data1,
                        data1+sizeof(data1),
                        data.begin(),
                        end))
                FAIL_LOG("Fastcgipp::Http::base64Encode() with string1")

            end = Fastcgipp::Http::base64Decode(
                    string2,
                    string2+sizeof(string2)-1,
                    data.begin());

            if(!std::equal(
                        data2,
                        data2+sizeof(data2),
                        data.begin(),
                        end))
                FAIL_LOG("Fastcgipp::Http::base64Encode() with string2")

            end = Fastcgipp::Http::base64Decode(
                    string3,
                    string3+sizeof(string3)-1,
                    data.begin());

            if(!std::equal(
                        data3,
                        data3+sizeof(data3),
                        data.begin(),
                        end))
                FAIL_LOG("Fastcgipp::Http::base64Encode() with string3")
        }
    }

    // Test Fastcgipp::Http::percentEscapedToRealBytes()
    {
        const char properDecoded[] =
            "E#H8i*H8!TkuxIGQya7bd^b%(JcEfkT5h#1qPift#VXDONNPhEUg_XYsH(if*7wz";

        const char encoded[] =
            "E%23H8i*H8!TkuxIGQya7bd%5Eb%25(JcEfkT5h%231qPift%23VXDONNPhEUg_XYs"
            "H(if*7wz";
        std::unique_ptr<char[]> decoded(new char[sizeof(encoded)-1]);

        auto end = Fastcgipp::Http::percentEscapedToRealBytes(
                encoded,
                encoded+sizeof(encoded)-1,
                decoded.get());

        if(!std::equal(
                    decoded.get(),
                    end,
                    properDecoded,
                    properDecoded+sizeof(properDecoded)-1))
            FAIL_LOG("Fastcgipp::Http::percentEscapedToRealBytes()")
    }

    // Testing Fastcgipp::Http::decodeUrlEncoded()
    {
        const char input[] =
            "%268c2LuPm=ccPd%5E92c%24Qd_1ab41hq%5EHDjHp!t!NJBa"
            "&9cIZvi%25-gGtqSQbo=!Llm_0-4Eo-KlIyL"
            "&unicode=%D0%B6%D0%B8%D0%B2%D0%BE%D1%82%D0%BD%D0%BE%D0%B5"
            "&unicode=%E3%82%A4%E3%83%B3%E3%82%BF%E3%83%BC%E3%83%8D%E3%83%83%E3"
            "%83%88";

        std::multimap<std::wstring, std::wstring> properOutput;
        properOutput.insert(std::pair<std::wstring, std::wstring>(
                    L"&8c2LuPm",
                    L"ccPd^92c$Qd_1ab41hq^HDjHp!t!NJBa"));
        properOutput.insert(std::pair<std::wstring, std::wstring>(
                    L"9cIZvi%-gGtqSQbo",
                    L"!Llm_0-4Eo-KlIyL"));
        properOutput.insert(std::pair<std::wstring, std::wstring>(
                    L"unicode",
                    L"животное"));
        properOutput.insert(std::pair<std::wstring, std::wstring>(
                    L"unicode",
                    L"インターネット"));

        std::multimap<std::wstring, std::wstring> output;
        Fastcgipp::Http::decodeUrlEncoded(
                input,
                input+sizeof(input)-1,
                output);

        if(output != properOutput)
            FAIL_LOG("Fastcgipp::Http::decodeUrlEncoded()")
    }

    // Testing Fastcgipp::Http::Environment
    {
        Fastcgipp::Http::Address loopback;
        loopback.m_data.back() = 1;

        static const std::vector<std::wstring> properPath
        {
            L"this",
            L"is",
            L"a",
            L"test\\ path"
        };

        std::multimap<std::wstring, std::wstring> properGets
        {
            {L"enctype", L"multipart"},
            {L"getVar", L"testing"},
            {L"secondGetVar", L"tested"},
            {L"utf8GetVarTest", L"проверка"}
        };

        std::multimap<std::wstring, std::wstring> properPosts
        {
            {L"submit", L"submit"},
            {L"+= aquí está el campo", L"Él está con un niño"}
        };

        static const std::multimap<std::wstring, std::wstring> properCookies
        {
            {L"echoCookie", L"<\"русский\">;"}
        };

        static const std::vector<std::string> properLanguages
        {
            "en_CA",
            "en_US",
            "en"
        };

        // Doing test with multipart POST
        {
            Fastcgipp::Http::Environment<wchar_t> environment;
            {
                {
                    const unsigned char parms[] = 
#include "multipartParam.hpp"
                    environment.fill(
                            reinterpret_cast<const char*>(parms),
                            reinterpret_cast<const char*>(parms+sizeof(parms)-1));
                }

                // Check parameters
                if(
                        environment.host != L"localhost" ||
                        environment.userAgent != L"Mozilla/5.0 (X11; Linux"
                            " x86_64; rv:45.0) Gecko/20100101 Firefox/45.0" ||
                        environment.acceptContentTypes != L"text/html,"
                            "application/xhtml+xml,application/xml;q=0.9,*/*;"
                            "q=0.8" ||
                        environment.acceptLanguages != properLanguages ||
                        environment.acceptCharsets != L"" ||
                        environment.referer != L"http://localhost/examples/"
                            "echo-form.html" ||
                        environment.contentType != L"multipart/form-data" ||
                        environment.root != L"/var/www/localhost/htdocs" ||
                        environment.scriptName != L"/examples/echo.fcgi" ||
                        environment.requestMethod !=
                            Fastcgipp::Http::RequestMethod::POST ||
                        environment.contentLength != 59071 ||
                        environment.requestUri != L"/examples/echo.fcgi/this/is"
                            "/a/test%5C+path?getVar=testing&secondGetVar=tested"
                            "&utf8GetVarTest=%D0%BF%D1%80%D0%BE%D0%B2%D0%B5%D1%"
                            "80%D0%BA%D0%B0&enctype=multipart" ||
                        environment.etag != 0 ||
                        environment.keepAlive != 0 ||
                        environment.serverAddress != loopback ||
                        environment.remoteAddress != loopback ||
                        environment.serverPort != 80 ||
                        environment.remotePort != 49003)
                    FAIL_LOG("Fastcgipp::Http::Environment multipart "\
                            "parameters didn't decode properly")

                // Checking pathInfo
                if(properPath != environment.pathInfo)
                    FAIL_LOG("Fastcgipp::Http::Environment multipart "\
                            "pathInfo didn't decode properly")

                // Checking gets
                if(properGets != environment.gets)
                    FAIL_LOG("Fastcgipp::Http::Environment multipart "\
                            "gets didn't decode properly")

                // Checking cookies
                if(properCookies != environment.cookies)
                    FAIL_LOG("Fastcgipp::Http::Environment multipart "\
                            "cookies didn't decode properly")

                // Checking posts
                {
                    static const unsigned char data[] = 
#include "multipartPost.hpp"
                    static const char* const dataStart =
                        reinterpret_cast<const char*>(data);
                    static const char* const halfPoint =
                        reinterpret_cast<const char*>(data+sizeof(data)/2);
                    static const char* const dataEnd =
                        reinterpret_cast<const char*>(data+sizeof(data));
                    environment.fillPostBuffer(
                            dataStart,
                            halfPoint);
                    environment.fillPostBuffer(
                            halfPoint,
                            dataEnd);
                    environment.parsePostBuffer();
                }
                if(properPosts != environment.posts)
                    FAIL_LOG("Fastcgipp::Http::Environment multipart "\
                            "posts didn't decode properly")

                // Checking files
                {
                    static const unsigned char gnu_png[] = 
#include "gnu.png.hpp"
                    if(
                            environment.files.size() != 1 ||
                            environment.files.begin()->first != L"aFile" ||
                            environment.files.begin()->second.filename
                                != L"gnu.png" ||
                            environment.files.begin()->second.contentType
                                != L"image/png" ||
                            environment.files.begin()->second.size != 58587 ||
                            !std::equal(
                                reinterpret_cast<const char*>(gnu_png),
                                reinterpret_cast<const char*>(gnu_png)
                                    +sizeof(gnu_png),
                                environment.files.begin()->second.data.get(),
                                environment.files.begin()->second.data.get()
                                    +environment.files.begin()->second.size))
                        FAIL_LOG("Fastcgipp::Http::Environment multipart "\
                                "files didn't decode properly")
                }
            }
        }

        // Doing test with urlencoded POST
        {
            Fastcgipp::Http::Environment<wchar_t> environment;
            {
                {
                    static const unsigned char data[] = 
#include "urlencodedParam.hpp"
                    static const char* const dataStart =
                        reinterpret_cast<const char*>(data);
                    static const char* const dataEnd =
                        reinterpret_cast<const char*>(data+sizeof(data));
                    environment.fill(
                            dataStart,
                            dataEnd);
                }

                properPosts.insert(std::pair<std::wstring, std::wstring>(
                            L"aFile",
                            L"gnu.png"));

                properGets.erase(L"enctype");
                properGets.insert(std::pair<std::wstring, std::wstring>(
                            L"enctype",
                            L"url-encoded"));

                // Checking parameters
                if(
                        environment.host != L"localhost" ||
                        environment.userAgent != L"Mozilla/5.0 (X11; Linux"
                            " x86_64; rv:45.0) Gecko/20100101 Firefox/45.0" ||
                        environment.acceptContentTypes != L"text/html,"
                            "application/xhtml+xml,application/xml;q=0.9,*/*;"
                            "q=0.8" ||
                        environment.acceptLanguages != properLanguages ||
                        environment.acceptCharsets != L"" ||
                        environment.referer != L"http://localhost/examples/"
                            "echo-form.html" ||
                        environment.contentType != L"application/x-www-form-urlencoded" ||
                        environment.root != L"/var/www/localhost/htdocs" ||
                        environment.scriptName != L"/examples/echo.fcgi" ||
                        environment.requestMethod !=
                            Fastcgipp::Http::RequestMethod::POST ||
                        environment.contentLength != 98 ||
                        environment.requestUri != L"/examples/echo.fcgi/this/is"
                            "/a/test%5C+path?getVar=testing&secondGetVar=tested"
                            "&utf8GetVarTest=%D0%BF%D1%80%D0%BE%D0%B2%D0%B5%D1%"
                            "80%D0%BA%D0%B0&enctype=url-encoded" ||
                        environment.etag != 0 ||
                        environment.keepAlive != 0 ||
                        environment.serverAddress != loopback ||
                        environment.remoteAddress != loopback ||
                        environment.serverPort != 80 ||
                        environment.remotePort != 49116)
                    FAIL_LOG("Fastcgipp::Http::Environment urlencoded "\
                            "parameters didn't decode properly")

                // Checking pathInfo
                if(properPath != environment.pathInfo)
                    FAIL_LOG("Fastcgipp::Http::Environment urlencoded "\
                            "pathInfo didn't decode properly")

                // Checking gets
                if(properGets != environment.gets)
                    FAIL_LOG("Fastcgipp::Http::Environment urlencoded "\
                            "gets didn't decode properly")

                // Checking cookies
                if(properCookies != environment.cookies)
                    FAIL_LOG("Fastcgipp::Http::Environment urlencoded "\
                            "cookies didn't decode properly")

                // Checking posts
                {
                    const unsigned char data[] = 
#include "urlencodedPost.hpp"
                    static const char* const dataStart =
                        reinterpret_cast<const char*>(data);
                    static const char* const halfPoint =
                        reinterpret_cast<const char*>(data+sizeof(data)/2);
                    static const char* const dataEnd =
                        reinterpret_cast<const char*>(data+sizeof(data));
                    environment.fillPostBuffer(
                            dataStart,
                            halfPoint);
                    environment.fillPostBuffer(
                            halfPoint,
                            dataEnd);
                    environment.parsePostBuffer();
                }
                if(properPosts != environment.posts)
                    FAIL_LOG("Fastcgipp::Http::Environment urlencoded "\
                            "posts didn't decode properly")
            }
        }
    }

    // Testing Fastcgipp::Http::SessionId
    {
        Fastcgipp::Http::SessionId session1;
        std::ostringstream ss;
        ss << session1;
        Fastcgipp::Http::SessionId session2(ss.str());

        if(!(session2 == session1))
            FAIL_LOG("Fastcgipp::Http::SessionId")
    }

    // Testing Fastcgipp::Http::Sessions
    {
        char properExpiration[30];
        std::time_t expiration;

        std::random_device device;
        std::uniform_int_distribution<unsigned short> alphanumeric(0, 61);

        expiration = std::time(nullptr)+6;
        Fastcgipp::Http::Sessions<std::wstring> sessions(3);
        std::fill(
                properExpiration,
                properExpiration+sizeof(properExpiration),
                0);
        std::strftime(
                properExpiration,
                sizeof(properExpiration),
                "%a, %d %b %Y %H:%M:%S GMT",
                std::gmtime(&expiration));
        if(!std::equal(
                    properExpiration,
                    properExpiration+sizeof(properExpiration),
                    sessions.expiration()))
            FAIL_LOG("Fastcgipp::Http::Sessions expiration string error 1")


        std::wstringstream ss;
        const Fastcgipp::Http::SessionId badId;

        for(int i=0; i<100; ++i)
        {
            std::shared_ptr<std::wstring> data(new std::wstring);
            for(int i=0; i<16; ++i)
                data->push_back(
                        Fastcgipp::Http::base64Characters[
                            alphanumeric(device)]);

            sessions.generate(data);
        }
        if(sessions.size() != 100)
            FAIL_LOG("Fastcgipp::Http::Sessions error inserting");
        std::this_thread::sleep_for(std::chrono::seconds(2));
        sessions.get(badId);
        if(sessions.size() != 100)
            FAIL_LOG("Fastcgipp::Http::Sessions cleanup happened when it "\
                    "shouldn't have");

        if(!std::equal(
                    properExpiration,
                    properExpiration+sizeof(properExpiration),
                    sessions.expiration()))
            FAIL_LOG("Fastcgipp::Http::Sessions expiration string error 2")

        for(int i=0; i<100; ++i)
        {
            std::shared_ptr<std::wstring> data(new std::wstring);
            for(int i=0; i<16; ++i)
                data->push_back(
                        Fastcgipp::Http::base64Characters[
                            alphanumeric(device)]);

            auto id = sessions.generate(data);
            ss << id << ' ' << *data << std::endl;
        }
        if(sessions.size() != 200)
            FAIL_LOG("Fastcgipp::Http::Sessions error inserting more sessions");
        std::this_thread::sleep_for(std::chrono::seconds(2));
        expiration = std::time(nullptr)+6;
        sessions.get(badId);
        if(sessions.size() != 100)
            FAIL_LOG("Fastcgipp::Http::Sessions cleanup didn't work");

        std::fill(
                properExpiration,
                properExpiration+sizeof(properExpiration),
                0);
        std::strftime(
                properExpiration,
                sizeof(properExpiration),
                "%a, %d %b %Y %H:%M:%S GMT",
                std::gmtime(&expiration));
        if(!std::equal(
                    properExpiration,
                    properExpiration+sizeof(properExpiration),
                    sessions.expiration()))
            FAIL_LOG("Fastcgipp::Http::Sessions expiration string error 3")

        std::wstring sessionId;
        for(int i=0; i<100; ++i)
        {
            std::wstring data;

            ss >> sessionId >> data;

            auto theData = sessions.get(sessionId);
            if(!theData)
                FAIL_LOG("Fastcgipp::Http::Sessions session(s) missing");
            if(*theData != data)
                FAIL_LOG("Fastcgipp::Http::Sessions session data non matching");
        }
        sessions.erase(sessionId);
        if(sessions.size() != 99)
            FAIL_LOG("Fastcgipp::Http::Sessions::erase() didn't work");
    }

    return 0;
}
