/**********************************************\
*
*  Andrey A. Ugolnik
*  http://www.ugolnik.info
*  andrey@ugolnik.info
*
\**********************************************/

#include "Utils.h"

#include <cctype>
#include <cerrno>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <sys/time.h>

uint64_t getCurrentTime()
{
    timeval t;
    ::gettimeofday(&t, nullptr);

    return static_cast<uint64_t>(t.tv_sec) * 1000000u + t.tv_usec;
}

const char* toString(bool enabled)
{
    return enabled
        ? "enabled"
        : "disabled";
}

bool isOption(const char* arg, const char* name)
{
    if (::strncmp(arg, "--", 2) != 0)
    {
        return ::strcmp(arg, name) == 0;
    }
    return ::strncmp(arg, name, ::strlen(name)) == 0;
}

bool parseUint(const char* value, uint32_t& out)
{
    if (value == nullptr || std::isdigit(static_cast<unsigned char>(*value)) == 0)
    {
        return false;
    }

    char* end = nullptr;
    errno = 0;
    const auto v = std::strtoul(value, &end, 10);
    if (*end != '\0' || errno != 0 || v > UINT32_MAX)
    {
        return false;
    }

    out = static_cast<uint32_t>(v);
    return true;
}
