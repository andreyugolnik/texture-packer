/**********************************************\
*
*  Andrey A. Ugolnik
*  http://www.ugolnik.info
*  andrey@ugolnik.info
*
\**********************************************/

#include "AtlasSize.h"
#include "Config.h"

#include <algorithm>
#include <cmath>

cAtlasSize::cAtlasSize(const sConfig& config)
    : m_config(config)
{
}

bool cAtlasSize::isFitToMaxSize(const sSize& size) const
{
    // Include atlas BORDER
    auto width = size.width + m_config.border * 2u;
    auto height = size.height + m_config.border * 2u;

    return width <= m_config.maxAtlasSize && height <= m_config.maxAtlasSize;
}

void cAtlasSize::addRect(const sSize& size)
{
    // include sprite PADDING
    auto width = size.width + m_config.padding * 2u;
    auto height = size.height + m_config.padding * 2u;

    m_maxRectSize.width = std::max(m_maxRectSize.width, width);
    m_maxRectSize.height = std::max(m_maxRectSize.height, height);

    m_area += static_cast<uint64_t>(width) * height;
}

uint64_t cAtlasSize::getArea() const
{
    return m_area;
}

sSize cAtlasSize::calcSize() const
{
    auto sq = static_cast<uint32_t>(std::sqrt(m_area));
    auto w = std::max(sq, m_maxRectSize.width) + m_config.border * 2u;
    auto h = std::max(sq, m_maxRectSize.height) + m_config.border * 2u;

    auto width = std::max(w, h);
    auto height = static_cast<uint32_t>(m_area / width);

    return {
        FixSize(width, m_config.pot),
        FixSize(height, m_config.pot)
    };
}

sSize cAtlasSize::nextSize(const sSize& size, uint32_t step) const
{
    auto nextWidth = size.width + step;
    auto growWidth = m_config.pot
        ? (nextWidth < FixSize(size.width, true) && nextWidth < size.height)
        : (nextWidth <= size.height);

    if (growWidth)
    {
        auto width = FixSize(nextWidth, m_config.pot);
        return { width, size.height };
    }

    auto height = FixSize(size.height + step, m_config.pot);
    return { size.width, height };
}

bool cAtlasSize::isGood(const sSize& size) const
{
    return size.width <= m_config.maxAtlasSize
        && size.height <= m_config.maxAtlasSize;
}

uint32_t cAtlasSize::NextPot(uint32_t size)
{
    size--;
    size |= size >> 1;
    size |= size >> 2;
    size |= size >> 4;
    size |= size >> 8;
    size |= size >> 16;
    size++;

    return size;
}

uint32_t cAtlasSize::FixSize(uint32_t size, bool isPot)
{
    if (isPot)
    {
        size = NextPot(size);
    }
    else if ((size & 0x01) != 0)
    {
        size++;
    }

    return size;
}
