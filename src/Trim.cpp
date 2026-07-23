/**********************************************\
*
*  Andrey A. Ugolnik
*  http://www.ugolnik.info
*  andrey@ugolnik.info
*
\**********************************************/

#include "Trim.h"
#include "Atlas/AtlasSize.h"
#include "Config.h"
#include "Log.h"

bool cTrim::doTrim(const cBitmap& input, cBitmap& output, sOffset& offset) const
{
    auto left = findLeft(input);
    auto right = findRigth(input);
    auto top = findTop(input);
    auto bottom = findBottom(input);

    auto& size = input.getSize();
    // cLog::Debug("  source size: {} x {}", size.width, size.height);

    if (left == 0 && right == size.width - 1 && top == 0 && bottom == size.height - 1)
    {
        // cLog::Debug("  original size");
        return false;
    }
    else if (left > right || top > bottom)
    {
        // cLog::Debug("  empty sprite");
        return true;
    }

    // cLog::Debug("  new rect: {} <-> {} , {} <-> {}", left, right, top, bottom);
    offset = { left, top };

    auto src = input.getData() + top * size.width;

    auto width = right - left + 1;
    auto height = bottom - top + 1;

    if (output.createBitmap({ width, height }) == false)
    {
        return false;
    }
    auto dst = output.getData();

    for (uint32_t y = top; y <= bottom; y++)
    {
        for (uint32_t x = left; x <= right; x++)
        {
            *dst++ = src[x];
        }
        src += size.width;
    }

    return true;
}

bool cTrim::trim(const char* /*path*/, const cBitmap& input)
{
    // cLog::Debug("Trim begin: '{}'", path);

    m_bitmap.clear();
    m_offset = { 0u, 0u };

    auto result = doTrim(input, m_bitmap, m_offset);

    // cLog::Debug("  trim end.");

    return result;
}

uint32_t cTrim::findLeft(const cBitmap& input) const
{
    auto& size = input.getSize();
    for (uint32_t x = 0u; x < size.width; x++)
    {
        auto src = input.getData() + x;
        for (uint32_t y = 0u; y < size.height; y++)
        {
            if (src->a != 0)
            {
                return x;
            }
            src += size.width;
        }
    }

    return size.width;
}

uint32_t cTrim::findRigth(const cBitmap& input) const
{
    auto& size = input.getSize();
    for (uint32_t x = 0; x < size.width; x++)
    {
        uint32_t offset = size.width - x - 1;
        auto src = input.getData() + offset;
        for (uint32_t y = 0; y < size.height; y++)
        {
            if (src->a != 0)
            {
                return offset;
            }
            src += size.width;
        }
    }

    return 0;
}

uint32_t cTrim::findTop(const cBitmap& input) const
{
    auto& size = input.getSize();
    for (uint32_t y = 0; y < size.height; y++)
    {
        auto src = input.getData() + y * size.width;
        for (uint32_t x = 0; x < size.width; x++)
        {
            if (src->a != 0)
            {
                return y;
            }
            src++;
        }
    }

    return size.height;
}

uint32_t cTrim::findBottom(const cBitmap& input) const
{
    auto& size = input.getSize();
    for (uint32_t y = 0; y < size.height; y++)
    {
        uint32_t offset = size.height - y - 1;
        auto src = input.getData() + offset * size.width;
        for (uint32_t x = 0; x < size.width; x++)
        {
            if (src->a != 0)
            {
                return offset;
            }
            src++;
        }
    }

    return 0;
}

cTrimRigthBottom::cTrimRigthBottom(const sConfig& config)
    : cTrim()
    , m_config(config)
{
}

bool cTrimRigthBottom::trim(const char* /*path*/, const cBitmap& input)
{
    m_bitmap.clear();

    const auto border = m_config.border;

    const auto width = cAtlasSize::FixSize(findRigth(input) + border, m_config.pot);
    const auto height = cAtlasSize::FixSize(findBottom(input) + border, m_config.pot);

    auto& size = input.getSize();
    if (width == size.width && height == size.height)
    {
        return false;
    }

    // cLog::Debug("Trim {} {} x {} -> {} x {}", path, size.width, size.height, width, height);

    auto src = input.getData();

    if (m_bitmap.createBitmap({ width, height }) == false)
    {
        return false;
    }
    auto dst = m_bitmap.getData();

    for (uint32_t y = 0; y < height; y++)
    {
        for (uint32_t x = 0; x < width; x++)
        {
            *dst++ = src[x];
        }
        src += size.width;
    }

    return true;
}
