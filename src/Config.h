/**********************************************\
*
*  Andrey A. Ugolnik
*  http://www.ugolnik.info
*  andrey@ugolnik.info
*
\**********************************************/

#pragma once

#include <cstdint>

struct sConfig
{
    uint32_t border = 0;
    uint32_t padding = 1;
    bool pot = false;
    bool trimSprite = false;
    bool enableMultiAtlas = false;
    bool keepFloat = false;
    bool anchorOnly = false;
    bool overlay = false;
    bool allowDupes = false;
    bool verbose = false;
    enum class Algorithm
    {
        KDTree,
        MaxRects,
        Auto
    };
    Algorithm algorithm = Algorithm::MaxRects;
    uint32_t maxAtlasSize = 2048u;
    uint32_t nameWidth = 0;

    void dump() const;

    static Algorithm ToAlgorithm(const char* str);
    static const char* ToName(Algorithm algo);
};
