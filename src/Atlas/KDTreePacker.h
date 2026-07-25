/**********************************************\
*
*  Andrey A. Ugolnik
*  http://www.ugolnik.info
*  andrey@ugolnik.info
*
\**********************************************/

#pragma once

#include "AtlasPacker.h"

#include <vector>

class cKDNode;

/**
 * 2D K-D Tree packer for rectangle packing.
 *
 * Algorithm:
 * - Recursively splits available space after placing each image
 * - Splits horizontally or vertically based on remaining space
 * - Optimized for long/thin sprites
 *
 * Time Complexity: O(n * h) where h is tree height (typically log n)
 * Space Complexity: O(n) for tree nodes
 */
class KDTreePacker final : public AtlasPacker
{
public:
    explicit KDTreePacker(const sConfig& config);
    ~KDTreePacker() override;

    KDTreePacker(const KDTreePacker&) = delete;
    KDTreePacker& operator=(const KDTreePacker&) = delete;

    bool setSize(const sSize& size) override;
    bool add(const cImage* image) override;
    void makeAtlas(bool overlay) override;

    uint32_t getRectsCount() const override;
    const cImage* getImageByIndex(uint32_t idx) const override;
    const sRect& getRectByIndex(uint32_t idx) const override;

private:
    cKDNode* m_root = nullptr;

    struct Piece
    {
        const cImage* image;
        cKDNode* node;
    };
    std::vector<Piece> m_nodes;
};
