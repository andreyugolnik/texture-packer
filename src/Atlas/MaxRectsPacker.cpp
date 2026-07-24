/**********************************************\
*
*  Andrey A. Ugolnik
*  http://www.ugolnik.info
*  andrey@ugolnik.info
*
\**********************************************/

#include "MaxRectsPacker.h"
#include "Config.h"
#include "Image.h"
#include "Types/Types.h"

#include <algorithm>
#include <limits>

namespace
{
    // Is inner fully contained in outer?
    bool contains(const sRect& outer, const sRect& inner)
    {
        return inner.left >= outer.left && inner.top >= outer.top
            && inner.right <= outer.right && inner.bottom <= outer.bottom;
    }

} // namespace

MaxRectsPacker::MaxRectsPacker(const sConfig& config)
    : AtlasPacker(config)
{
}

MaxRectsPacker::~MaxRectsPacker()
{
}

// Both orders sort by the longest side descending (empirically the densest
// for MaxRects); the search tries both and keeps the smaller atlas.
bool MaxRectsPacker::Compare(const cImage* a, const cImage* b)
{
    auto& sizea = a->getBitmap().getSize();
    auto& sizeb = b->getBitmap().getSize();

    return std::max(sizea.width, sizea.height) > std::max(sizeb.width, sizeb.height);
}

bool MaxRectsPacker::CompareAlt(const cImage* a, const cImage* b)
{
    auto& sizea = a->getBitmap().getSize();
    auto& sizeb = b->getBitmap().getSize();

    const auto maxa = std::max(sizea.width, sizea.height);
    const auto maxb = std::max(sizeb.width, sizeb.height);
    if (maxa != maxb)
    {
        return maxa > maxb;
    }

    const auto areaa = static_cast<uint64_t>(sizea.width) * sizea.height;
    const auto areab = static_cast<uint64_t>(sizeb.width) * sizeb.height;
    if (areaa != areab)
    {
        return areaa > areab;
    }

    return sizea.height > sizeb.height;
}

void MaxRectsPacker::setSize(const sSize& size)
{
    const auto border = m_config.border;

    m_placed.clear();
    m_freeRects.clear();
    if (size.width > border * 2 && size.height > border * 2)
    {
        m_freeRects.push_back({ border, border, size.width - border, size.height - border });
    }

    m_atlas.setSize(size);
}

bool MaxRectsPacker::findPosition(uint32_t width, uint32_t height, sRect& out) const
{
    // Best Short Side Fit: minimize the smaller leftover edge, then the larger.
    auto bestShort = std::numeric_limits<uint32_t>::max();
    auto bestLong = std::numeric_limits<uint32_t>::max();
    bool found = false;

    for (const auto& f : m_freeRects)
    {
        const auto fw = f.width();
        const auto fh = f.height();
        if (fw < width || fh < height)
        {
            continue;
        }

        const auto shortSide = std::min(fw - width, fh - height);
        const auto longSide = std::max(fw - width, fh - height);
        if (shortSide < bestShort || (shortSide == bestShort && longSide < bestLong))
        {
            bestShort = shortSide;
            bestLong = longSide;
            out = { f.left, f.top, f.left + width, f.top + height };
            found = true;
        }
    }

    return found;
}

bool MaxRectsPacker::add(const cImage* image)
{
    auto& size = image->getBitmap().getSize();
    const auto padding = m_config.padding;
    const auto footprintW = size.width + padding * 2;
    const auto footprintH = size.height + padding * 2;

    sRect footprint;
    if (findPosition(footprintW, footprintH, footprint) == false)
    {
        return false;
    }

    placeFootprint(footprint);

    // Store a sprite-sized rect anchored at the footprint origin: copyBitmap
    // writes the sprite plus its surrounding padding starting there.
    m_placed.push_back({ image,
                         { footprint.left, footprint.top,
                           footprint.left + size.width, footprint.top + size.height } });

    return true;
}

void MaxRectsPacker::placeFootprint(const sRect& used)
{
    std::vector<sRect> pieces;
    for (auto it = m_freeRects.begin(); it != m_freeRects.end();)
    {
        const auto& f = *it;
        const bool overlaps = used.left < f.right && used.right > f.left
            && used.top < f.bottom && used.bottom > f.top;
        if (overlaps == false)
        {
            ++it;
            continue;
        }

        // Split f into up to four maximal free rects around the used area.
        if (used.left > f.left)
        {
            pieces.push_back({ f.left, f.top, used.left, f.bottom });
        }
        if (used.right < f.right)
        {
            pieces.push_back({ used.right, f.top, f.right, f.bottom });
        }
        if (used.top > f.top)
        {
            pieces.push_back({ f.left, f.top, f.right, used.top });
        }
        if (used.bottom < f.bottom)
        {
            pieces.push_back({ f.left, used.bottom, f.right, f.bottom });
        }

        it = m_freeRects.erase(it);
    }

    m_freeRects.insert(m_freeRects.end(), pieces.begin(), pieces.end());
    pruneFreeRects();
}

void MaxRectsPacker::pruneFreeRects()
{
    for (size_t i = 0; i < m_freeRects.size();)
    {
        bool redundant = false;
        for (size_t j = 0; j < m_freeRects.size(); ++j)
        {
            if (i != j && contains(m_freeRects[j], m_freeRects[i]))
            {
                redundant = true;
                break;
            }
        }

        if (redundant)
        {
            m_freeRects.erase(m_freeRects.begin() + i);
        }
        else
        {
            ++i;
        }
    }
}

void MaxRectsPacker::makeAtlas(bool overlay)
{
    for (const auto& piece : m_placed)
    {
        copyBitmap(piece.rect, piece.image, overlay);
    }
}

uint32_t MaxRectsPacker::getRectsCount() const
{
    return static_cast<uint32_t>(m_placed.size());
}

const cImage* MaxRectsPacker::getImageByIndex(uint32_t idx) const
{
    return m_placed[idx].image;
}

const sRect& MaxRectsPacker::getRectByIndex(uint32_t idx) const
{
    return m_placed[idx].rect;
}
