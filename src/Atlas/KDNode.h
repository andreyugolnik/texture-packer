/**********************************************\
*
*  Andrey A. Ugolnik
*  http://www.ugolnik.info
*  andrey@ugolnik.info
*
\**********************************************/

#pragma once

#include "Types/Types.h"

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

private:
    const sRect m_area;
    const uint32_t m_padding;

private:
    bool m_used = false;
    cKDNode* m_childA = nullptr; // left or top
    cKDNode* m_childB = nullptr; // right or bottom

    sRect m_rect{ 0u, 0u, 0u, 0u };
};
