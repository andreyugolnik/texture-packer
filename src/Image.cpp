/**********************************************\
*
*  Andrey A. Ugolnik
*  http://www.ugolnik.info
*  andrey@ugolnik.info
*
\**********************************************/

#include "Image.h"
#include "Log.h"
#include "Trim.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

#include <algorithm>
#include <cctype>
#include <cstring>

namespace
{

    std::string TrimPath(const char* path, uint32_t trimPath)
    {
        std::string res;
        if (path != nullptr)
        {
            res = path;

            res.erase(0, trimPath);

            auto pos = res.find_last_of(".");
            if (pos != std::string::npos)
            {
                res.erase(pos);
            }

            if (res.substr(0, 2) == "./")
            {
                res.erase(0, 2);
            }

            std::replace(res.begin(), res.end(), '/', '_');
        }

        return res;
    }

} // namespace

bool cImage::IsImage(const char* path)
{
    if (path == nullptr)
    {
        return false;
    }

    std::string str = path;
    auto dot = str.find_last_of('.');
    if (dot == std::string::npos)
    {
        return false;
    }

    auto ext = str.substr(dot);
    for (auto& c : ext)
    {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }

    return ext == ".bmp"
        || ext == ".gif"
        || ext == ".hdr"
        || ext == ".jpeg"
        || ext == ".jpg"
        || ext == ".pbm"
        || ext == ".pgm"
        || ext == ".pic"
        || ext == ".png"
        || ext == ".pnm"
        || ext == ".ppm"
        || ext == ".psd"
        || ext == ".tga";
}

void cImage::StbDeleter::operator()(uint8_t* data) const
{
    stbi_image_free(data);
}

cImage::~cImage()
{
    clear();
}

void cImage::clear()
{
    m_stbImageData.reset();
    m_bitmap.clear();
}

bool cImage::load(const char* path, uint32_t trimPath, cTrim* trim)
{
    clear();

    m_name = path;

    m_spriteId = TrimPath(path, trimPath);
    if (m_spriteId.length() == 0)
    {
        cLog::Error("Trim value '{}' too big for path '{}'.", trimPath, path);
        return false;
    }

    // N=#comp | components
    // --------+------------------------
    // 1       | grey
    // 2       | grey, alpha
    // 3       | red, green, blue
    // 4       | red, green, blue, alpha
    int width = 0;
    int height = 0;
    int bpp = 0;
    m_stbImageData.reset(stbi_load(path, &width, &height, &bpp, 4));
    if (m_stbImageData == nullptr)
    {
        return false;
    }

    m_originalSize = {
        static_cast<uint32_t>(width),
        static_cast<uint32_t>(height)
    };

    m_bitmap.setBitmap(m_originalSize, m_stbImageData.get());

    if (trim != nullptr && trim->trim(path, m_bitmap))
    {
        m_bitmap = std::move(trim->getBitmap());
        m_offset = trim->getOffset();
    }

    return true;
}
