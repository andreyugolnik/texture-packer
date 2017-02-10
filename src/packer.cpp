/**********************************************\
*
*  Andrey A. Ugolnik
*  http://www.ugolnik.info
*  andrey@ugolnik.info
*
\**********************************************/

#include "packer.h"
#include "image.h"
#include "file.h"

#include <climits>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sstream>

cPacker::cPacker(size_t count, unsigned border, unsigned padding)
    : m_border(border)
    , m_padding(padding)
{
    m_images.reserve(count);
}

cPacker::~cPacker()
{
    clear();
}

void cPacker::clear()
{
    m_atlas.clear();
    m_images.clear();
}

void cPacker::setSize(unsigned width, unsigned height)
{
    clear();
    m_atlas.setSize(width, height);
}

bool cPacker::add(const cImage* image)
{
    const auto& bmp = image->getBitmap();

    const auto border = m_border;
    const auto padding = m_padding;

    const auto width = m_atlas.width - bmp.width - border;
    const auto height = m_atlas.height - bmp.height - border;

    sRect imgRc;

    for (unsigned y = border; y < height;)
    {
        imgRc.top = y;
        imgRc.bottom = y + bmp.height;

        for (unsigned x = border; x < width;)
        {
            imgRc.left = x;
            imgRc.right = x + bmp.width;

            const auto rc = checkRegion(imgRc);
            if (rc == nullptr)
            {
                // merge this region into the used region's vector
                m_images.push_back({ image, imgRc });

                return true;
            }

            x += rc->width() + padding;
        }
        y++;
    }

    return false;
}

const sRect* cPacker::checkRegion(const sRect& region) const
{
    const auto padding = m_padding;
    for (const auto& img : m_images)
    {
        const auto& rc = img.rc;
        if (region.left < rc.right + padding
            && region.right > rc.left + padding
            && region.top < rc.bottom + padding
            && region.bottom > rc.top + padding)
        {
            return &rc;
        }
    }

    return nullptr;
}

void cPacker::fillTexture(bool overlay)
{
    for (const auto& img : m_images)
    {
        copyBitmap(img.rc, img.image, overlay);
    }
}

void cPacker::copyBitmap(const sRect& rc, const cImage* image, bool overlay)
{
    auto& bmp = image->getBitmap();
    const auto width = bmp.width;
    const auto height = bmp.height;

    const auto offx = rc.left;
    const auto offy = rc.top;
    const auto pitch = m_atlas.width;

    auto src = bmp.data.data();

    for (unsigned y = 0; y < height; y++)
    {
        auto dst = m_atlas.data.data() + (y + offy) * pitch + offx;
        for (unsigned x = 0; x < width; x++)
        {
            *dst++ = *src++;
        }
    }

    if (overlay)
    {
        const float sR = 0.0f;
        const float sG = 0.0f;
        const float sB = 1.0f;
        const float sA = 0.6f;
        const float inv = 1.0f / 255.0f;

        for (unsigned y = 0; y < height; y++)
        {
            auto dst = m_atlas.data.data() + (y + offy) * pitch + offx;
            for (unsigned x = 0; x < width; x++)
            {
                const float dR = dst->r * inv;
                const float dG = dst->g * inv;
                const float dB = dst->b * inv;
                const float dA = dst->a * inv;

                const float r = sA * (sR - dR) + dR;
                const float g = sA * (sG - dG) + dG;
                const float b = sA * (sB - dB) + dB;
                const float a = dA * (1.0f - sA) + sA;

                *dst++ =
                {
                    static_cast<unsigned char>(r * 255.0f),
                    static_cast<unsigned char>(g * 255.0f),
                    static_cast<unsigned char>(b * 255.0f),
                    static_cast<unsigned char>(a * 255.0f)
                };
            }
        }
    }
}

bool cPacker::generateResFile(const char* name, const char* atlasName)
{
    cFile file;
    if (file.open(name, "w"))
    {
        std::stringstream out;
        out << "<objects>\n";
        for (const auto& img : m_images)
        {
            auto tx = img.rc.left;
            auto ty = img.rc.top;
            auto tw = img.rc.width();
            auto th = img.rc.height();
            out << "    ";
            out << "<sprite id=\"" << img.image->getName() << "\" texture=\"" << atlasName << "\" ";
            out << "rect=\"" << tx << " " << ty << " " << tw << " " << th << "\" ";
            out << "hotspot=\"" << (tw * 0.5f) << " " << (th * 0.5f) << "\" />\n";
        }
        out << "</objects>\n";
        file.write((void*)out.str().c_str(), out.str().length());

        return true;
    }

    return false;
}
