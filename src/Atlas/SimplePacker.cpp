/**********************************************\
*
*  Andrey A. Ugolnik
*  http://www.ugolnik.info
*  andrey@ugolnik.info
*
\**********************************************/

#include "SimplePacker.h"
#include "Config.h"
#include "Image.h"
#include "Types/Types.h"

SimplePacker::SimplePacker(const sConfig& config)
    : AtlasPacker(config)
{
}

SimplePacker::~SimplePacker()
{
}

bool SimplePacker::Compare(const cImage* a, const cImage* b)
{
    auto& bmpa = a->getBitmap();
    auto& bmpb = b->getBitmap();
    auto& sizea = bmpa.getSize();
    auto& sizeb = bmpb.getSize();
    return (sizea.width * sizea.height > sizeb.width * sizeb.height)
        && (sizea.width + sizea.height > sizeb.width + sizeb.height);
}

bool SimplePacker::add(const cImage* image)
{
    const auto& bmp = image->getBitmap();

    const auto border = m_config.border;
    const auto padding = m_config.padding;

    auto& atlasSize = m_atlas.getSize();
    auto& bmpSize = bmp.getSize();

    // Reject oversized sprites; otherwise the unsigned math below
    // underflows and an out-of-bounds rect gets accepted.
    if (bmpSize.width + border > atlasSize.width
        || bmpSize.height + border > atlasSize.height)
    {
        return false;
    }

    const auto width = atlasSize.width - bmpSize.width - border;
    const auto height = atlasSize.height - bmpSize.height - border;

    sRect imgRc;

    for (uint32_t y = border; y < height;)
    {
        imgRc.top = y;
        imgRc.bottom = y + bmpSize.height;

        for (uint32_t x = border; x < width;)
        {
            imgRc.left = x;
            imgRc.right = x + bmpSize.width;

            const auto rc = checkRegion(imgRc);
            if (rc == nullptr)
            {
                // merge this region into the used region's vector
                m_images.push_back({ image, imgRc });

                return true;
            }

            x += rc->width() + padding * 2;
        }
        y++;
    }

    return false;
}

const sRect* SimplePacker::checkRegion(const sRect& region) const
{
    const auto padding = m_config.padding;
    for (const auto& img : m_images)
    {
        const auto& rc = img.rc;
        if (region.left < rc.right + padding * 2
            && region.right + padding * 2 > rc.left
            && region.top < rc.bottom + padding * 2
            && region.bottom + padding * 2 > rc.top)
        {
            return &rc;
        }
    }

    return nullptr;
}

void SimplePacker::setSize(const sSize& size)
{
    m_images.clear();
    m_atlas.setSize(size);
}

void SimplePacker::makeAtlas(bool overlay)
{
    for (const auto& img : m_images)
    {
        copyBitmap(img.rc, img.image, overlay);
    }
}

uint32_t SimplePacker::getRectsCount() const
{
    return (uint32_t)m_images.size();
}

const cImage* SimplePacker::getImageByIndex(uint32_t idx) const
{
    return m_images[idx].image;
}

const sRect& SimplePacker::getRectByIndex(uint32_t idx) const
{
    return m_images[idx].rc;
}
