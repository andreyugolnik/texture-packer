/**********************************************\
*
*  Andrey A. Ugolnik
*  http://www.ugolnik.info
*  andrey@ugolnik.info
*
\**********************************************/

#include "Config.h"
#include "Log.h"
#include "Utils.h"

namespace
{
    constexpr static struct
    {
        const char* id;
        const char* name;
        sConfig::Algorithm algorithm;
    } Algos[] = {
        { "kdtree", "KD-Tree", sConfig::Algorithm::KDTree },
        { "classic", "Classic", sConfig::Algorithm::Classic },
    };

} // namespace

void sConfig::dump() const
{
    // General
    cLog::Info("Algorithm:          {}", ToName(algorithm));
    cLog::Info("Border:             {} px", border);
    cLog::Info("Padding:            {} px", padding);
    cLog::Info("Max atlas size:     {} px", maxAtlasSize);
    cLog::Info("Multi-atlas:        {}", toString(enableMultiAtlas));

    // Features
    cLog::Info("Keep hotspot float: {}", toString(keepFloat));
    cLog::Info("Anchor only:        {}", toString(anchorOnly));
    cLog::Info("Power of Two:       {}", toString(pot));
    cLog::Info("Trim sprites:       {}", toString(trimSprite));
    cLog::Info("Allow duplicates:   {}", toString(alowDupes));
    cLog::Info("Overlay:            {}", toString(overlay));
}

sConfig::Algorithm sConfig::ToAlgorithm(const char* str)
{
    for (const auto& m : Algos)
    {
        if (::strcmp(str, m.id) == 0)
        {
            return m.algorithm;
        }
    }
    return sConfig::Algorithm::KDTree;
}

const char* sConfig::ToName(Algorithm algo)
{
    for (const auto& m : Algos)
    {
        if (m.algorithm == algo)
        {
            return m.name;
        }
    }
    return "Unknown";
}
