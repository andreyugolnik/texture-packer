/**********************************************\
*
*  Andrey A. Ugolnik
*  http://www.ugolnik.info
*  andrey@ugolnik.info
*
\**********************************************/

#include "KDTreePacker.h"
#include "Config.h"
#include "Image.h"
#include "KDNode.h"

KDTreePacker::KDTreePacker(const sConfig& config)
    : AtlasPacker(config)
{
}

KDTreePacker::~KDTreePacker() = default;

bool KDTreePacker::setSize(const sSize& size)
{
    const auto border = m_config.border;

    m_root.reset();

    m_nodes.clear();
    m_atlas.setSize(size);

    if (size.width > border * 2 && size.height > border * 2)
    {
        m_root = std::make_unique<cKDNode>(sRect{ border, border, size.width - border, size.height - border }, m_config.padding);
        return true;
    }

    return false;
}

bool KDTreePacker::add(const cImage* image)
{
    auto& bmp = image->getBitmap();
    auto& size = bmp.getSize();
    auto node = m_root->add(size);
    if (node != nullptr)
    {
        m_nodes.push_back({ image, node });

        return true;
    }

    return false;
}

void KDTreePacker::makeAtlas(bool overlay)
{
    for (const auto& piece : m_nodes)
    {
        auto rc = piece.node->getRect();
        copyBitmap(rc, piece.image, overlay);
    }
}

uint32_t KDTreePacker::getRectsCount() const
{
    return static_cast<uint32_t>(m_nodes.size());
}

const cImage* KDTreePacker::getImageByIndex(uint32_t idx) const
{
    return m_nodes[idx].image;
}

const sRect& KDTreePacker::getRectByIndex(uint32_t idx) const
{
    return m_nodes[idx].node->getRect();
}
