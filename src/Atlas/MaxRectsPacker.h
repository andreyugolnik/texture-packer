/**********************************************\
*
*  Andrey A. Ugolnik
*  http://www.ugolnik.info
*  andrey@ugolnik.info
*
\**********************************************/

#pragma once

#include "AtlasPacker.h"
#include "Types/Types.h"

#include <vector>

/**
 * MaxRects bin packer (Jylanki, "A Thousand Ways to Pack the Bin").
 *
 * Keeps a list of maximal free rectangles and places each sprite into the
 * free rect that leaves the smallest leftover short side (Best Short Side
 * Fit). After a placement the overlapped free rects are split into maximal
 * pieces and non-maximal rects are pruned. Denser than the K-D tree on
 * mixed sprite sizes at the cost of higher packing time.
 */
class MaxRectsPacker final : public AtlasPacker
{
public:
    explicit MaxRectsPacker(const sConfig& config);
    ~MaxRectsPacker() override;

    bool setSize(const sSize& size) override;
    bool add(const cImage* image) override;
    void makeAtlas(bool overlay) override;

    uint32_t getRectsCount() const override;
    const cImage* getImageByIndex(uint32_t idx) const override;
    const sRect& getRectByIndex(uint32_t idx) const override;

private:
    bool findPosition(uint32_t width, uint32_t height, sRect& out) const;
    void placeFootprint(const sRect& used);
    void pruneFreeRects();

    struct Piece
    {
        const cImage* image;
        sRect rect;
    };
    std::vector<Piece> m_placed;
    std::vector<sRect> m_freeRects;
};
