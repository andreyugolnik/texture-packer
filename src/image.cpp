/**********************************************\
*
*  Andrey A. Ugolnik
*  http://www.ugolnik.info
*  andrey@ugolnik.info
*
\**********************************************/

#include "image.h"
#include "file.h"
#include "trim.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

#include <algorithm>
#include <cstring>

bool cImage::IsImage(const char* /*path*/)
{
    return true;
}

cImage::~cImage()
{
    clear();
}

void cImage::clear()
{
    if (m_stbImageData != nullptr)
    {
        stbi_image_free(m_stbImageData);
        m_stbImageData = nullptr;
    }

    m_bitmap.clear();
}

bool cImage::load(const char* path, uint32_t trimPath, cTrim* trim)
{
    clear();

    m_name = path;

    m_spriteId = path;

    m_spriteId.erase(0, trimPath);

    auto pos = m_spriteId.find_last_of(".");
    if (pos != std::string::npos)
    {
        m_spriteId.erase(pos);
    }

    if (m_spriteId.substr(0, 2) == "./")
    {
        m_spriteId.erase(0, 2);
    }

    std::replace(m_spriteId.begin(), m_spriteId.end(), '/', '_');

    if (m_spriteId.length() == 0)
    {
        ::printf("(EE) Trim value too big for path '%s'\n", path);
    }

    //     N=#comp     components
    //       1           grey
    //       2           grey, alpha
    //       3           red, green, blue
    //       4           red, green, blue, alpha
    int width;
    int height;
    int bpp;
    m_stbImageData = stbi_load(path, &width, &height, &bpp, 4);

    m_bitmap.setBitmap(width, height, m_stbImageData);

    if (m_stbImageData != nullptr && trim != nullptr)
    {
        if (trim->trim(path, m_bitmap))
        {
            m_bitmap = std::move(trim->getBitmap());
        }
    }

    return m_stbImageData != nullptr;
}
