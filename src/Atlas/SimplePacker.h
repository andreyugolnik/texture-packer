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

class SimplePacker final : public AtlasPacker
{
public:
    SimplePacker(const sConfig& config);
    ~SimplePacker() override;

    static bool Compare(const cImage* a, const cImage* b);

    void setSize(const sSize& size) override;
    bool add(const cImage* image) override;
    void makeAtlas(bool overlay) override;

    uint32_t getRectsCount() const override;
    const cImage* getImageByIndex(uint32_t idx) const override;
    const sRect& getRectByIndex(uint32_t idx) const override;

private:
    const sRect* checkRegion(const sRect& region) const;

private:
    struct Piece
    {
        const cImage* image;
        sRect rc;
    };
    std::vector<Piece> m_images;
};
