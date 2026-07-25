/**********************************************\
*
*  Andrey A. Ugolnik
*  http://www.ugolnik.info
*  andrey@ugolnik.info
*
\**********************************************/

#pragma once

#include "Types/Types.h"

class cBitmap final
{
public:
    cBitmap() = default;
    ~cBitmap();

    cBitmap(const cBitmap&) = delete;
    cBitmap& operator=(const cBitmap&) = delete;

    cBitmap(cBitmap&& other) noexcept;
    cBitmap& operator=(cBitmap&& other) noexcept;

    void clear();

    bool createBitmap(const sSize& size);
    void setSize(const sSize& size);
    void setBitmap(const sSize& size, void* data);

    const sSize& getSize() const
    {
        return m_size;
    }

    uint32_t getPitch() const
    {
        return m_size.width;
    }

    struct Pixel
    {
        uint8_t r;
        uint8_t g;
        uint8_t b;
        uint8_t a;
    };

    Pixel* getData()
    {
        return m_data;
    }

    const Pixel* getData() const
    {
        return m_data;
    }

private:
    template <typename T>
    void moveAndSet(T& me, T& other, T value)
    {
        me = other;
        other = value;
    }

    sSize m_size;

    bool m_manageData = false;
    Pixel* m_data = nullptr;
};
