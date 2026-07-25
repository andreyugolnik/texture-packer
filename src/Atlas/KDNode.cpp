/**********************************************\
*
*  Andrey A. Ugolnik
*  http://www.ugolnik.info
*  andrey@ugolnik.info
*
\**********************************************/

#include "KDNode.h"

#include <vector>

cKDNode::cKDNode(const sRect& area, uint32_t padding)
    : m_area(area)
    , m_padding(padding)
{
}

cKDNode::~cKDNode()
{
    // Iterative teardown: a default destructor would let the owning unique_ptr
    // children destruct recursively, and a degenerate (deep) tree could then
    // overflow the stack. Move each node's children out before it is destroyed
    // so its own destructor finds none.
    std::vector<std::unique_ptr<cKDNode>> pending;
    if (m_childA != nullptr)
    {
        pending.push_back(std::move(m_childA));
    }
    if (m_childB != nullptr)
    {
        pending.push_back(std::move(m_childB));
    }

    while (pending.empty() == false)
    {
        auto node = std::move(pending.back());
        pending.pop_back();

        if (node->m_childA != nullptr)
        {
            pending.push_back(std::move(node->m_childA));
        }
        if (node->m_childB != nullptr)
        {
            pending.push_back(std::move(node->m_childB));
        }
        // node is destroyed here with its children already moved out.
    }
}

cKDNode* cKDNode::add(const sSize& size)
{
    // Iterative depth-first search, child A before child B (matching the old
    // recursion), so a deep tree cannot overflow the stack.
    std::vector<cKDNode*> pending;
    pending.push_back(this);

    while (pending.empty() == false)
    {
        auto node = pending.back();
        pending.pop_back();

        if (node->isLeaf())
        {
            auto placed = node->placeInLeaf(size);
            if (placed != nullptr)
            {
                return placed;
            }
        }
        else
        {
            // Push B first so A is popped and explored first.
            if (node->m_childB != nullptr)
            {
                pending.push_back(node->m_childB.get());
            }
            if (node->m_childA != nullptr)
            {
                pending.push_back(node->m_childA.get());
            }
        }
    }

    return nullptr;
}

// Place the sprite into this empty leaf, splitting as needed. The recursion
// here is bounded to two levels: each split fixes one dimension exactly.
cKDNode* cKDNode::placeInLeaf(const sSize& size)
{
    // end of the tree, no more room
    if (m_used)
    {
        return nullptr;
    }

    const auto padding = m_padding;

    const auto imgWidth = size.width + padding * 2;
    const auto imgHeight = size.height + padding * 2;

    const auto nodeWidth = m_area.width();
    const auto nodeHeight = m_area.height();

    const auto x = m_area.left;
    const auto y = m_area.top;
    m_rect = { x, y, x + size.width, y + size.height };

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
    const auto subwidth = nodeWidth - imgWidth;
    const auto subheight = nodeHeight - imgHeight;

    if (subwidth <= subheight)
    {
        // split --
        m_childA = std::make_unique<cKDNode>(sRect{ x, y, x + nodeWidth, y + imgHeight }, padding);
        m_childB = std::make_unique<cKDNode>(sRect{ x, y + imgHeight, x + nodeWidth, y + imgHeight + subheight }, padding);
    }
    else
    {
        // split |
        m_childA = std::make_unique<cKDNode>(sRect{ x, y, x + imgWidth, y + nodeHeight }, padding);
        m_childB = std::make_unique<cKDNode>(sRect{ x + imgWidth, y, x + imgWidth + subwidth, y + nodeHeight }, padding);
    }

    return m_childA->placeInLeaf(size);
}
