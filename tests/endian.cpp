#include "fastcgi++/log.hpp"
#include "fastcgi++/endian.hpp"

using namespace Fastcgipp;
using namespace Fastcgipp::detail;

int main()
{
    static_assert(IsByteSwappable<std::uint16_t>::value, "IsByteSwappable");
    static_assert(IsByteSwappable<std::uint32_t>::value, "IsByteSwappable");
    static_assert(IsByteSwappable<std::uint64_t>::value, "IsByteSwappable");

    union {
        std::uint32_t i;
        std::uint8_t b[4];
    } u = { .b = {1, 2, 3, 4} };
    if(fromBigEndian(u.i) != 0x01020304)
        FAIL_LOG("Failed fromBigEndian");
    if(byteSwap(byteSwap(0x01020304u)) != 0x01020304)
        FAIL_LOG("Failed byteSwap");
    u.i = toBigEndian(0x01020304u);
    if(u.b[0] != 1 || u.b[1] != 2 || u.b[2] != 3 || u.b[3] != 4)
        FAIL_LOG("Failed toBigEndian");

    return 0;
}
