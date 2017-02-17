/**********************************************\
*
*  Andrey A. Ugolnik
*  http://www.ugolnik.info
*  andrey@ugolnik.info
*
\**********************************************/

#include "KDTreePacker.h"
#include "../image.h"
#include "../types/rect.h"
#include "../types/size.h"

class cKDNode final
{
public:
    cKDNode(const sRect& area, unsigned padding)
        : m_used(false)
        , m_area(area)
        , m_padding(padding)
        , m_childA(nullptr)
        , m_childB(nullptr)
    {
        m_rect = area;
        m_rect.right -= padding;
        m_rect.bottom -= padding;
    }

    ~cKDNode()
    {
        delete m_childA;
        delete m_childB;
    }

    cKDNode* add(const sSize& size)
    {
        if (isLeaf())
        {
            // end of the tree, no more room
            if (m_used)
            {
                return nullptr;
            }

            const auto padding = m_padding;

            const auto imgWidth = size.width + padding;
            const auto imgHeight = size.height + padding;

            const auto nodeWidth = m_area.width();
            const auto nodeHeight = m_area.height();

            // size matches exactly
            if (imgWidth == nodeWidth && imgHeight == nodeHeight)
            {
                m_used = true;
                return this;
            }

            // rect is too big for this node
            if (imgWidth > nodeWidth || imgHeight > nodeHeight)
            {
                return nullptr;
            }

            // split this node in two
            const auto subwidth  = nodeWidth - imgWidth;
            const auto subheight = nodeHeight - imgHeight;

            const auto x = m_area.left;
            const auto y = m_area.top;

            // static int last = -1;
            if (subwidth <= subheight)
            {
                // if (last != 0)
                // {
                // last = 0;
                // printf("-"); fflush(nullptr);
                // }
                // split --
                m_childA = new cKDNode({ x, y, x + nodeWidth, y + imgHeight }, padding);
                m_childB = new cKDNode({ x, y + imgHeight, x + nodeWidth, y + imgHeight + subheight }, padding);
            }
            else
            {
                // if (last != 1)
                // {
                // last = 1;
                // printf("|"); fflush(nullptr);
                // }
                // split |
                m_childA = new cKDNode({ x, y, x + imgWidth, y + nodeHeight }, padding);
                m_childB = new cKDNode({ x + imgWidth, y, x + imgWidth + subwidth, y + nodeHeight }, padding);
            }

            return m_childA->add(size);
        }
        else if (m_childA != nullptr)
        {
            auto node = m_childA->add(size);
            if (node != nullptr)
            {
                return node;
            }
            else if (m_childB != nullptr)
            {
                return m_childB->add(size);
            }
        }

        return nullptr;
    }

    const sRect& getRect() const
    {
        return m_rect;
    }

private:
    bool isLeaf() const
    {
        return m_childA == nullptr || m_childB == nullptr;
    }

private:
    bool m_used;
    sRect m_area;
    unsigned m_padding;

    cKDNode* m_childA; // left or top
    cKDNode* m_childB; // right or bottom

    sRect m_rect;
};




KDTreePacker::KDTreePacker(unsigned border, unsigned padding)
    : AtlasPacker(border, padding)
    , m_root(nullptr)
{
}

KDTreePacker::~KDTreePacker(void)
{
    delete m_root;
}

bool KDTreePacker::compare(const cImage* a, const cImage* b) const
{
    auto& bmpa = a->getBitmap();
    auto& bmpb = b->getBitmap();
    return (bmpa.width > bmpb.width) || (bmpa.width * bmpa.height > bmpb.width * bmpb.height);
}

void KDTreePacker::setSize(const sSize& size)
{
    const auto border = m_border;

    delete m_root;
    m_root = new cKDNode({ border, border, size.width - border * 2, size.height - border * 2 }, m_padding);

    m_nodes.clear();
    m_atlas.setSize(size.width, size.height);
}

bool KDTreePacker::add(const cImage* image)
{
    auto& bmp = image->getBitmap();
    auto node = m_root->add({ bmp.width, bmp.height });
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
    return (uint32_t)m_nodes.size();
}

const cImage* KDTreePacker::getImageByIndex(uint32_t idx) const
{
    return m_nodes[idx].image;
}

const sRect& KDTreePacker::getRectByIndex(uint32_t idx) const
{
    return m_nodes[idx].node->getRect();
}
