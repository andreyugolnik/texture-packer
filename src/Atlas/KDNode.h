/**********************************************\
*
*  Andrey A. Ugolnik
*  http://www.ugolnik.info
*  andrey@ugolnik.info
*
\**********************************************/

#pragma once

#include "Types/Types.h"

#include <memory>

class cKDNode final
{
public:
    cKDNode(const sRect& area, uint32_t padding);
    ~cKDNode();

    cKDNode(const cKDNode&) = delete;
    cKDNode& operator=(const cKDNode&) = delete;

    cKDNode* add(const sSize& size);

    const sRect& getRect() const
    {
        return m_rect;
    }

private:
    bool isLeaf() const
    {
        return m_childA == nullptr && m_childB == nullptr;
    }

    cKDNode* placeInLeaf(const sSize& size);

private:
    const sRect m_area;
    const uint32_t m_padding;

private:
    bool m_used = false;
    std::unique_ptr<cKDNode> m_childA; // left or top
    std::unique_ptr<cKDNode> m_childB; // right or bottom

    sRect m_rect{ 0u, 0u, 0u, 0u };
};
