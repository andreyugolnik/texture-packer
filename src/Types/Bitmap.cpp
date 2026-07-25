/**********************************************\
*
*  Andrey A. Ugolnik
*  http://www.ugolnik.info
*  andrey@ugolnik.info
*
\**********************************************/

#include "Bitmap.h"

#include <algorithm>
#include <new>

cBitmap::~cBitmap()
{
    clear();
}

void cBitmap::clear()
{
    m_size = { 0u, 0u };

    if (m_manageData)
    {
        m_manageData = false;
        delete[] m_data;
    }

    m_data = nullptr;
}

bool cBitmap::createBitmap(const sSize& size)
{
    clear();

    const auto pixelCount = static_cast<size_t>(size.width) * size.height;
    m_data = new (std::nothrow) Pixel[pixelCount];
    if (m_data == nullptr)
    {
        return false;
    }

    m_size = size;
    m_manageData = true;

    // Initialize all pixels to transparent black
    std::fill(m_data, m_data + pixelCount, Pixel{ 0, 0, 0, 0 });

    return true;
}

void cBitmap::setSize(const sSize& size)
{
    clear();

    m_size = size;
}

void cBitmap::setBitmap(const sSize& size, void* data)
{
    clear();

    m_size = size;

    m_data = static_cast<Pixel*>(data);
}

cBitmap::cBitmap(cBitmap&& other) noexcept
{
    moveAndSet(m_size, other.m_size, {});
    moveAndSet(m_manageData, other.m_manageData, false);
    moveAndSet(m_data, other.m_data, static_cast<Pixel*>(nullptr));
}

cBitmap& cBitmap::operator=(cBitmap&& other) noexcept
{
    if (this != &other)
    {
        clear();

        moveAndSet(m_size, other.m_size, {});
        moveAndSet(m_manageData, other.m_manageData, false);
        moveAndSet(m_data, other.m_data, static_cast<Pixel*>(nullptr));
    }

    return *this;
}
