/**********************************************\
*
*  Andrey A. Ugolnik
*  http://www.ugolnik.info
*  andrey@ugolnik.info
*
\**********************************************/

#include "AtlasPacker.h"
#include "Config.h"
#include "File.h"
#include "Image.h"
#include "KDTreePacker.h"
#include "Log.h"
#include "MaxRectsPacker.h"
#include "Trim.h"
#include "Types/Types.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <fmt/core.h>

namespace
{
    // Approximate XML Name check; non-ASCII (UTF-8) bytes are treated as
    // valid name characters to avoid false warnings on Unicode ids.
    bool isValidXmlName(const std::string& name)
    {
        auto isNameStart = [](unsigned char c) {
            return std::isalpha(c) != 0 || c == '_' || c == ':' || c >= 0x80;
        };
        auto isNameChar = [](unsigned char c) {
            return std::isalnum(c) != 0 || c == '_' || c == ':' || c == '-' || c == '.' || c >= 0x80;
        };

        if (name.empty() || isNameStart(static_cast<unsigned char>(name.front())) == false)
        {
            return false;
        }

        for (auto c : name)
        {
            if (isNameChar(static_cast<unsigned char>(c)) == false)
            {
                return false;
            }
        }

        return true;
    }

    // Escape a string for use as an XML attribute value (texture path).
    std::string escapeXmlAttrValue(const std::string& value)
    {
        std::string out;
        out.reserve(value.size());
        for (auto c : value)
        {
            switch (c)
            {
            case '&':
                out += "&amp;";
                break;
            case '<':
                out += "&lt;";
                break;
            case '>':
                out += "&gt;";
                break;
            case '"':
                out += "&quot;";
                break;
            default:
                out += c;
                break;
            }
        }

        return out;
    }
} // namespace

std::unique_ptr<AtlasPacker> AtlasPacker::createPacker(sConfig::Algorithm algorithm, const sConfig& config)
{
    if (algorithm == sConfig::Algorithm::MaxRects)
    {
        return std::make_unique<MaxRectsPacker>(config);
    }

    return std::make_unique<KDTreePacker>(config);
}

AtlasPacker::AtlasPacker(const sConfig& config)
    : m_config(config)
{
}

AtlasPacker::~AtlasPacker()
{
}

void AtlasPacker::copyBitmap(const sRect& rc, const cImage* image, bool overlay)
{
    auto& bmp = image->getBitmap();

    const auto& size = bmp.getSize();

    const auto padding = m_config.padding;

    const auto offx = rc.left;
    const auto offy = rc.top;
    const auto offxPadded = offx + padding;
    const auto offyPadded = offy + padding;
    const auto pitch = m_atlas.getPitch();

    auto srcData = bmp.getData();
    auto dstData = m_atlas.getData();

    for (uint32_t y = 0; y < size.height; y++)
    {
        auto dst = dstData + (y + offyPadded) * pitch + offxPadded;
        for (uint32_t x = 0; x < size.width; x++)
        {
            *dst++ = *srcData++;
        }
    }

    if (padding > 0)
    {
        // adding the border
        /*
          Consider the following picture:
          ++======++
          ++======++
          ||oooooo||
          ||oooooo||
          ||oooooo||
          ||oooooo||
          ++======++
          ++======++

          o - image pixels, copied in the previous loop
          | - left and right border
          = - top and bottom border
          + - corner pixels
        */

        auto srcLeft = bmp.getData();
        for (uint32_t y = 0; y < size.height; ++y)
        {
            auto left = dstData + (y + offyPadded) * pitch + offx;
            auto right = dstData + (y + offyPadded) * pitch + offxPadded + size.width;
            for (uint32_t i = 0; i < padding; ++i)
            {
                *(left + i) = *srcLeft;
                *(right + i) = *(srcLeft + size.width - 1);
            }
            srcLeft += size.width;
        }

        auto srcTop = bmp.getData();
        for (uint32_t x = 0; x < size.width; ++x)
        {
            auto top = dstData + offy * pitch + offxPadded + x;
            auto bottom = dstData + (offyPadded + size.height) * pitch + offxPadded + x;
            for (uint32_t i = 0; i < padding; ++i)
            {
                *(top + pitch * i) = *srcTop;
                *(bottom + pitch * i) = *(srcTop + size.width * (size.height - 1));
            }
            srcTop++;
        }

        // border corner pixels
        {
            auto dst = dstData + offx + offy * pitch;
            auto widthOffset = size.width + padding;
            auto heightOffset = size.height + padding;

            auto topLeft = dst;
            auto topRight = dst + widthOffset;
            auto bottomLeft = dst + pitch * heightOffset;
            auto bottomRight = dst + pitch * heightOffset + widthOffset;

            auto src = dstData + offyPadded * pitch + offxPadded;

            auto tlValue = *src;
            auto trValue = *(src + size.width - 1);
            auto blValue = *(src + pitch * (size.height - 1));
            auto brValue = *(src + pitch * (size.height - 1) + size.width - 1);
            for (uint32_t x = 0; x < padding; ++x)
            {
                for (uint32_t y = 0; y < padding; ++y)
                {
                    auto c = x * pitch + y;

                    *(topLeft + c) = tlValue;
                    *(topRight + c) = trValue;
                    *(bottomLeft + c) = blValue;
                    *(bottomRight + c) = brValue;
                }
            }
        }
    }

    if (overlay)
    {
        const float sR = 0.0f;
        const float sG = 1.0f;
        const float sB = 0.0f;
        const float sA = 0.6f;
        const float inv = 1.0f / 255.0f;

        for (uint32_t y = 0; y < size.height; y++)
        {
            auto dst = dstData + (y + offyPadded) * pitch + offxPadded;
            for (uint32_t x = 0; x < size.width; x++)
            {
                const float dR = dst->r * inv;
                const float dG = dst->g * inv;
                const float dB = dst->b * inv;
                const float dA = dst->a * inv;

                const float r = sA * (sR - dR) + dR;
                const float g = sA * (sG - dG) + dG;
                const float b = sA * (sB - dB) + dB;
                const float a = dA * (1.0f - sA) + sA;

                *dst++ = {
                    static_cast<uint8_t>(r * 255.0f),
                    static_cast<uint8_t>(g * 255.0f),
                    static_cast<uint8_t>(b * 255.0f),
                    static_cast<uint8_t>(a * 255.0f)
                };
            }
        }
    }
}

bool AtlasPacker::buildAtlas()
{
    const auto atlasSize = m_atlas.getSize();
    if (m_atlas.createBitmap(atlasSize) == false)
    {
        cLog::Error("Failed to allocate atlas {} x {}.", atlasSize.width, atlasSize.height);
        return false;
    }

    makeAtlas(m_config.overlay);

    cTrimRightBottom trim(m_config);
    if (trim.trim("atlas", m_atlas))
    {
        m_atlas = std::move(trim.getBitmap());
    }

    return true;
}

bool AtlasPacker::generateResFile(cFile& file, const std::string& atlasName)
{
    std::string out;

    const uint32_t rectsCount = getRectsCount();
    std::vector<uint32_t> indexes(rectsCount);
    for (uint32_t i = 0; i < rectsCount; i++)
    {
        indexes[i] = i;
    }

    std::sort(indexes.begin(), indexes.end(), [this](uint32_t a, uint32_t b) {
        auto& na = getImageByIndex(a)->getName();
        auto& nb = getImageByIndex(b)->getName();
        return na < nb;
    });

    for (uint32_t i = 0; i < rectsCount; i++)
    {
        const auto idx = indexes[i];

        auto image = getImageByIndex(idx);
        auto& spriteId = image->getSpriteId();

        if (isValidXmlName(spriteId) == false)
        {
            cLog::Warning("Sprite id '{}' is not a valid XML name; the atlas descriptor may fail to parse.", spriteId);
        }

        const auto& rc = getRectByIndex(idx);
        sOffset pos{
            rc.left + m_config.padding,
            rc.top + m_config.padding
        };
        sSize size{
            rc.width(),
            rc.height()
        };

        auto& originalSize = image->getOriginalSize();
        auto& offset = image->getOffset();
        sHotspot hotspot{
            m_config.keepFloat
                ? originalSize.width * 0.5f - offset.x
                : std::trunc(originalSize.width * 0.5f - offset.x),
            m_config.keepFloat
                ? originalSize.height * 0.5f - offset.y
                : std::trunc(originalSize.height * 0.5f - offset.y)
        };

        // Anchor is the hotspot normalized by sprite size, so size * anchor
        // reproduces the hotspot (integer when keepFloat is off).
        sHotspot anchor{
            size.width != 0
                ? hotspot.x / size.width
                : 0.0f,
            size.height != 0
                ? hotspot.y / size.height
                : 0.0f
        };

        std::string hotspotAttr;
        if (m_config.anchorOnly == false)
        {
            hotspotAttr = fmt::format(" hotspot=\"{} {}\"", hotspot.x, hotspot.y);
        }

        out += fmt::format("    <{} texture=\"{}\" rect=\"{} {} {} {}\"{} anchor=\"{} {}\" />\n",
                           spriteId, escapeXmlAttrValue(atlasName),
                           pos.x, pos.y, size.width, size.height,
                           hotspotAttr, anchor.x, anchor.y);
    }

    return file.write(out.c_str(), out.length()) == out.length();
}
