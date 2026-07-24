/**********************************************\
*
*  Andrey A. Ugolnik
*  http://www.ugolnik.info
*  andrey@ugolnik.info
*
\**********************************************/

#pragma once

class cImage;

// Sprite sort orders for the packing search, independent of the packing
// algorithm. The search tries several orders and keeps the one that packs
// tightest, so an order is a property of the search, not of any one packer.
namespace SortOrder
{
    bool byLongestSide(const cImage* a, const cImage* b);
    bool byLongestSideThenArea(const cImage* a, const cImage* b);

} // namespace SortOrder
