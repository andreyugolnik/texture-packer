/**********************************************\
*
*  Andrey A. Ugolnik
*  http://www.ugolnik.info
*  andrey@ugolnik.info
*
\**********************************************/

#include "SortOrder.h"
#include "Image.h"

#include <algorithm>

namespace SortOrder
{
    // Mode 6: optimized for long and thin sprites, both vertical and horizontal,
    // ensuring efficient packing regardless of orientation or aspect ratio.

    // ./test.sh test-wz -overlay -max 3000
    // Out of a total of 34 files, 29 packed better + 2 unchanged, 3 packed worse.
    // The total pixel difference across all files is -4,673,840.

    // ./test.sh test-wh -overlay -max 3000
    // Out of a total of 33 files, 30 packed better + 0 unchanged, 3 packed worse.
    // The total pixel difference across all files is -9,441,708.
    bool byLongestSide(const cImage* a, const cImage* b)
    {
        auto& sizea = a->getBitmap().getSize();
        auto& sizeb = b->getBitmap().getSize();

        auto maxa = std::max(sizea.width, sizea.height);
        auto maxb = std::max(sizeb.width, sizeb.height);

        return maxa > maxb;
    }

    // Alternative order (Mode 3): longest side -> area -> height. Wins on some
    // inputs where byLongestSide loses; both are tried and the tighter pack is
    // kept.
    bool byLongestSideThenArea(const cImage* a, const cImage* b)
    {
        auto& sizea = a->getBitmap().getSize();
        auto& sizeb = b->getBitmap().getSize();

        auto maxa = std::max(sizea.width, sizea.height);
        auto maxb = std::max(sizeb.width, sizeb.height);
        if (maxa != maxb)
        {
            return maxa > maxb;
        }

        auto areaa = sizea.width * sizea.height;
        auto areab = sizeb.width * sizeb.height;
        if (areaa != areab)
        {
            return areaa > areab;
        }

        return sizea.height > sizeb.height;
    }

} // namespace SortOrder

// Alternative sprite orders explored during tuning, disabled but kept for
// reference as separate functions. The benchmark notes record how each
// performed on sample sets relative to the others.
#if 0
namespace SortOrder
{
    // Mode 1

    // ./test.sh test-wz -overlay -max 3000
    // Out of a total of 34 files, 30 packed better + 1 unchanged, 3 packed worse.
    // The total pixel difference across all files is -3,992,040.

    // ./test.sh test-wh -overlay -max 3000
    // Out of a total of 33 files, 33 packed better + 0 unchanged, 0 packed worse.
    // The total pixel difference across all files is -8,527,984.
    bool byWidthOrArea(const cImage* a, const cImage* b)
    {
        auto& sizea = a->getBitmap().getSize();
        auto& sizeb = b->getBitmap().getSize();

        return (sizea.width > sizeb.height)
            || (sizea.width * sizea.height > sizeb.width * sizeb.height);
    }

    // Mode 2

    // ./test.sh test-wz -overlay -max 3000
    // Out of a total of 34 files, 31 packed better + 2 unchanged, 1 packed worse.
    // The total pixel difference across all files is -4,725,088.

    // ./test.sh test-wh -overlay -max 3000
    // Out of a total of 33 files, 30 packed better + 0 unchanged, 3 packed worse.
    // The total pixel difference across all files is -9,487,340.
    bool byLongestSideThenHeightThenArea(const cImage* a, const cImage* b)
    {
        auto& sizea = a->getBitmap().getSize();
        auto& sizeb = b->getBitmap().getSize();

        auto maxa = std::max(sizea.width, sizea.height);
        auto maxb = std::max(sizeb.width, sizeb.height);

        if (maxa > maxb)
        {
            return true;
        }
        if (maxb < maxa)
        {
            return false;
        }

        if (sizea.height > sizeb.height)
        {
            return true;
        }
        if (sizea.height < sizeb.height)
        {
            return false;
        }

        auto areaa = sizea.width * sizea.height;
        auto areab = sizeb.width * sizeb.height;

        return areaa > areab;
    }

    // Mode 4

    // ./test.sh test-wz -overlay -max 3000
    // Out of a total of 34 files, 28 packed better + 2 unchanged, 4 packed worse.
    // The total pixel difference across all files is -4,127,476.

    // ./test.sh test-wh -overlay -max 3000
    // Out of a total of 33 files, 29 packed better + 0 unchanged, 4 packed worse.
    // The total pixel difference across all files is -8,066,584.
    bool byWidthThenArea(const cImage* a, const cImage* b)
    {
        auto& sizea = a->getBitmap().getSize();
        auto& sizeb = b->getBitmap().getSize();

        if (sizea.width > sizeb.width)
        {
            return true;
        }
        if (sizea.width < sizeb.width)
        {
            return false;
        }

        auto areaa = sizea.width * sizea.height;
        auto areab = sizeb.width * sizeb.height;

        return areaa > areab;
    }

    // Mode 5

    // ./test.sh test-wz -overlay -max 3000
    // Out of a total of 34 files, 30 packed better + 2 unchanged, 2 packed worse.
    // The total pixel difference across all files is -4,921,888.

    // ./test.sh test-wh -overlay -max 3000
    // Out of a total of 33 files, 32 packed better + 0 unchanged, 1 packed worse.
    // The total pixel difference across all files is -8,943,940.
    bool byLongestSideOrArea(const cImage* a, const cImage* b)
    {
        auto& sizea = a->getBitmap().getSize();
        auto& sizeb = b->getBitmap().getSize();

        auto maxa = std::max(sizea.width, sizea.height);
        auto maxb = std::max(sizeb.width, sizeb.height);

        if (maxa > maxb)
        {
            return true;
        }

        auto areaa = sizea.width * sizea.height;
        auto areab = sizeb.width * sizeb.height;

        if (areaa > areab)
        {
            return true;
        }

        return maxb < maxa;
    }

} // namespace SortOrder
#endif
