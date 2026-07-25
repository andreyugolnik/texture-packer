/**********************************************\
*
*  Andrey A. Ugolnik
*  http://www.ugolnik.info
*  andrey@ugolnik.info
*
\**********************************************/

#pragma once

#include "Types/Bitmap.h"

#include <memory>
#include <string>

class cTrim;

class cImage final
{
public:
    static bool IsImage(const char* path);

public:
    ~cImage();

    void clear();

    bool load(const char* path, uint32_t trimPath, cTrim* trim);

    const cBitmap& getBitmap() const
    {
        return m_bitmap;
    }

    const sSize& getOriginalSize() const
    {
        return m_originalSize;
    }

    const sOffset& getOffset() const
    {
        return m_offset;
    }

    const std::string& getName() const
    {
        return m_name;
    }

    const std::string& getSpriteId() const
    {
        return m_spriteId;
    }

private:
    // stb buffers must be released with stbi_image_free, not delete.
    struct StbDeleter
    {
        void operator()(uint8_t* data) const;
    };

private:
    std::string m_name;
    std::string m_spriteId;

    sSize m_originalSize;
    sOffset m_offset;

    std::unique_ptr<uint8_t, StbDeleter> m_stbImageData;
    cBitmap m_bitmap;
};
