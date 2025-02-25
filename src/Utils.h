/**********************************************\
*
*  Andrey A. Ugolnik
*  http://www.ugolnik.info
*  andrey@ugolnik.info
*
\**********************************************/

#pragma once

#include <cstdint>

uint64_t getCurrentTime();
const char* formatNum(int num, char delimiter = '\'');
const char* isEnabled(bool enabled);
