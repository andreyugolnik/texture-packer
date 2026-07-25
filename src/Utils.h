/**********************************************\
*
*  Andrey A. Ugolnik
*  http://www.ugolnik.info
*  andrey@ugolnik.info
*
\**********************************************/

#pragma once

#include <cstddef>
#include <cstdint>

// Compile-time length of a string literal (without the terminating null).
template <std::size_t N>
constexpr std::size_t litLen(const char (&)[N])
{
    return N - 1;
}

uint64_t getCurrentTime();
const char* toString(bool enabled);

bool isOption(const char* arg, const char* name);

bool parseUint(const char* value, uint32_t& out);
