/**********************************************\
*
*  Andrey A. Ugolnik
*  http://www.ugolnik.info
*  andrey@ugolnik.info
*
\**********************************************/

#include "ImageList.h"
#include "Atlas/AtlasPacker.h"
#include "Atlas/KDTreePacker.h"
#include "Config.h"
#include "File.h"
#include "Image.h"
#include "ImageSaver.h"
#include "Log.h"
#include "Trim.h"
#include "Utils.h"

#include <algorithm>
#include <cassert>
#include <fmt/core.h>
#include <iterator>
#include <limits>
#include <vector>

namespace
{
    // Try both sort orders and pick the one that produces the smaller atlas
    using Comparator = bool (*)(const cImage*, const cImage*);
    constexpr Comparator Comparators[] = {
        KDTreePacker::Compare,
        KDTreePacker::CompareAlt
    };

    std::string GenerateAtlasName(const char* baseName, uint32_t index)
    {
        std::string name(baseName);

        // Find the last dot for the extension
        auto dotPos = name.rfind('.');
        if (dotPos != std::string::npos)
        {
            // Insert index before the extension
            auto base = name.substr(0, dotPos);
            auto ext = name.substr(dotPos);

            return index == 0
                ? base + ext
                : fmt::format("{}_{}{}", base, index, ext);
        }

        return index == 0
            ? name
            : fmt::format("{}_{}", name, index);
    }

    cAtlasSize GetAtlasSize(const sConfig& config, const ImageList& images)
    {
        cAtlasSize packedSize(config);
        for (auto img : images)
        {
            packedSize.addRect(img->getBitmap().getSize());
        }

        return packedSize;
    }

} // namespace

cImageList::cImageList(const sConfig& config, uint32_t reserve)
    : m_config(config)
    , m_size(config)
    , m_trim(config.trimSprite
                 ? new cTrim()
                 : nullptr)
{
    m_images.reserve(reserve);
}

cImageList::~cImageList()
{
    for (auto img : m_images)
    {
        delete img;
    }

    delete m_trim;
}

cImageList::Result cImageList::loadImage(const std::string& path, uint32_t trimCount)
{
    if (cImage::IsImage(path.c_str()) == false)
    {
        return Result::NotAnImage;
    }

    std::unique_ptr<cImage> image(new cImage());

    if (image->load(path.c_str(), trimCount, m_trim) == false)
    {
        return Result::CannotOpen;
    }

    auto& bmp = image->getBitmap();
    auto& size = bmp.getSize();
    if (m_size.isFitToMaxSize(size) == false)
    {
        return Result::TooBig;
    }

    m_size.addRect(size);

    m_images.push_back(image.release());

    return Result::OK;
}

bool cImageList::doPacking(const char* desiredAtlasName, const char* outputResName,
                           const char* resPathPrefix, sSize& atlasSize)
{
    if (m_images.empty())
    {
        return true;
    }

    return m_config.enableMultiAtlas
        ? packMultiAtlas(desiredAtlasName, outputResName, resPathPrefix, atlasSize)
        : packSingleAtlas(desiredAtlasName, outputResName, resPathPrefix, atlasSize);
}

bool cImageList::packMultiAtlas(const char* desiredAtlasName, const char* outputResName,
                                const char* resPathPrefix, sSize& atlasSize)
{
    auto remainingImages = m_images;
    uint32_t atlasIndex = 0;

    cFile xmlFile;
    writeXmlHeader(xmlFile, outputResName);

    const sSize maxSize{ m_config.maxAtlasSize, m_config.maxAtlasSize };
    atlasSize = maxSize;

    bool success = true;
    while (remainingImages.empty() == false)
    {
        ImageList packedImages;

        if (packImagesToMaxSize(remainingImages, maxSize, packedImages) == false)
        {
            cLog::Error("Cannot fit any images into atlas #{}.", atlasIndex);
            success = false;
            break;
        }

        const auto startTime = getCurrentTime();

        sSize finalSize;
        std::unique_ptr<AtlasPacker> packer;
        if (optimizeAtlasSize(packedImages, maxSize, finalSize, packer) == false)
        {
            cLog::Error("Cannot optimize size for atlas #{}.", atlasIndex);
            success = false;
            break;
        }

        packer->buildAtlas();

        cAtlasSize packedSize = GetAtlasSize(m_config, packedImages);
        const auto spritesArea = packedSize.getArea();

        const auto atlasName = GenerateAtlasName(desiredAtlasName, atlasIndex);
        if (saveAtlas(packer.get(), atlasName.c_str(), resPathPrefix, xmlFile,
                      finalSize, spritesArea, startTime)
            == false)
        {
            success = false;
            break;
        }

        // Remove packed images from the remaining list
        const auto prevCount = remainingImages.size();
        for (auto img : packedImages)
        {
            remainingImages.erase(std::remove(remainingImages.begin(), remainingImages.end(), img),
                                  remainingImages.end());
        }

        if (remainingImages.size() == prevCount)
        {
            cLog::Error("Failed to pack any images into atlas #{}.", atlasIndex);
            success = false;
            break;
        }

        atlasSize = finalSize;
        atlasIndex++;
    }

    writeXmlFooter(xmlFile, outputResName);

    return success;
}

bool cImageList::packSingleAtlas(const char* desiredAtlasName, const char* outputResName,
                                 const char* resPathPrefix, sSize& atlasSize)
{
    atlasSize = m_size.calcSize();
    if (m_size.isGood(atlasSize) == false)
    {
        return false;
    }

    auto startTime = getCurrentTime();

    if (m_config.algorithm == sConfig::Algorithm::KDTree)
    {
        const sSize maxSize{ m_config.maxAtlasSize, m_config.maxAtlasSize };
        if (findBestSortAndSize(m_images, atlasSize, maxSize, atlasSize) == false)
        {
            return false;
        }
    }
    else
    {
        auto packer = AtlasPacker::create(m_images, m_config);

        if (findMinimalAtlasSize(packer.get(), m_images, atlasSize, atlasSize) == false)
        {
            return false;
        }
    }

    auto packer = AtlasPacker::createPacker(m_config);

    cLog::Info("Packing atlas:");
    cLog::Info(" - size: {} x {}", atlasSize.width, atlasSize.height);

    if (prepareSize(packer.get(), atlasSize, m_images) == false)
    {
        cLog::Error("Cannot pack images into atlas {} x {}.", atlasSize.width, atlasSize.height);
        return false;
    }

    cFile xmlFile;
    writeXmlHeader(xmlFile, outputResName);

    auto spritesArea = m_size.getArea();
    packer->buildAtlas();

    if (saveAtlas(packer.get(), desiredAtlasName, resPathPrefix, xmlFile,
                  atlasSize, spritesArea, startTime)
        == false)
    {
        return false;
    }

    writeXmlFooter(xmlFile, outputResName);

    return true;
}

bool cImageList::packImagesToMaxSize(ImageList& remainingImages, const sSize& maxSize, ImageList& outPackedImages)
{
    outPackedImages.clear();

    if (m_config.algorithm == sConfig::Algorithm::KDTree)
    {
        ImageList bestPacked;
        auto bestIdx = std::numeric_limits<size_t>::max();

        for (size_t i = 0u; i < std::size(Comparators); i++)
        {
            auto sorted = remainingImages;
            std::stable_sort(sorted.begin(), sorted.end(), Comparators[i]);

            auto packer = AtlasPacker::createPacker(m_config);
            packer->setSize(maxSize);

            ImageList packed;
            for (auto img : sorted)
            {
                if (packer->add(img))
                {
                    packed.push_back(img);
                }
            }

            if (packed.size() > bestPacked.size())
            {
                bestPacked = std::move(packed);
                bestIdx = i;
            }
        }

        if (bestIdx == std::numeric_limits<size_t>::max())
        {
            return false;
        }

        // Apply the winning sort to remainingImages for correct downstream order
        std::stable_sort(remainingImages.begin(), remainingImages.end(), Comparators[bestIdx]);
        outPackedImages = std::move(bestPacked);
    }
    else
    {
        auto packer = AtlasPacker::create(remainingImages, m_config);
        packer->setSize(maxSize);

        for (auto img : remainingImages)
        {
            if (packer->add(img))
            {
                outPackedImages.push_back(img);
            }
        }
    }

    return outPackedImages.empty() == false;
}

bool cImageList::optimizeAtlasSize(ImageList& packedImages, const sSize& maxSize,
                                   sSize& outFinalSize, std::unique_ptr<AtlasPacker>& packer)
{
    cAtlasSize packedSize = GetAtlasSize(m_config, packedImages);
    const auto optimalSize = packedSize.calcSize();

    auto startSize = packedSize.isGood(optimalSize)
        ? optimalSize
        : maxSize;

    if (m_config.algorithm == sConfig::Algorithm::KDTree)
    {
        if (findBestSortAndSize(packedImages, startSize, maxSize, outFinalSize) == false)
        {
            return false;
        }

        packer = AtlasPacker::createPacker(m_config);
    }
    else
    {
        packer = AtlasPacker::create(packedImages, m_config);

        if (findMinimalAtlasSize(packer.get(), packedImages, startSize, outFinalSize) == false)
        {
            // Growth may step over maxSize; try maxSize as a fallback
            outFinalSize = maxSize;
            if (prepareSize(packer.get(), maxSize, packedImages) == false)
            {
                return false;
            }
            return true;
        }
    }

    return prepareSize(packer.get(), outFinalSize, packedImages);
}

bool cImageList::saveAtlas(AtlasPacker* packer, const char* desiredAtlasName,
                           const char* resPathPrefix, cFile& xmlFile,
                           const sSize& atlasSize, uint64_t spritesArea, uint64_t startTime)
{
    auto& atlas = packer->getBitmap();
    cImageSaver saver(atlas, desiredAtlasName);

    if (saver.save() == false)
    {
        cLog::Error("Error writing atlas '{}' ({} x {})",
                    desiredAtlasName,
                    atlasSize.width, atlasSize.height);
        return false;
    }

    const auto outputAtlasName = saver.getAtlasName();

    // Write XML entry
    if (xmlFile.isOpened())
    {
        std::string atlasPath = resPathPrefix != nullptr
            ? resPathPrefix
            : "";
        atlasPath += outputAtlasName;
        packer->generateResFile(xmlFile, atlasPath);
    }

    const auto atlasArea = static_cast<size_t>(atlasSize.width) * atlasSize.height;
    const auto percent = static_cast<uint32_t>(100.0 * spritesArea / atlasArea);

    cLog::Info("Atlas '{}' ({} x {}, fill: {}%) was created in {:.2f} ms.",
               outputAtlasName,
               atlasSize.width, atlasSize.height,
               percent,
               (getCurrentTime() - startTime) * 0.001f);

    return true;
}

// Try each KDTree comparator, find the minimal atlas size for each,
// and pick the sort order that produces the smallest atlas area.
// Falls back to maxSize if the growth loop steps over it.
bool cImageList::findBestSortAndSize(ImageList& images, const sSize& startSize, const sSize& maxSize, sSize& outSize)
{
    sSize bestSize{ 0, 0 };
    auto bestArea = std::numeric_limits<uint64_t>::max();
    auto bestIdx = std::numeric_limits<size_t>::max();

    for (size_t i = 0u; i < std::size(Comparators); i++)
    {
        auto sorted = images;
        std::stable_sort(sorted.begin(), sorted.end(), Comparators[i]);

        auto packer = AtlasPacker::createPacker(m_config);
        sSize foundSize;
        if (findMinimalAtlasSize(packer.get(), sorted, startSize, foundSize) == false)
        {
            // Growth may step over maxSize; try maxSize as a fallback
            if (prepareSize(packer.get(), maxSize, sorted) == false)
            {
                continue;
            }
            foundSize = maxSize;
        }

        auto area = static_cast<uint64_t>(foundSize.width) * foundSize.height;
        if (area < bestArea)
        {
            bestArea = area;
            bestSize = foundSize;
            bestIdx = i;
        }
    }

    if (bestIdx == std::numeric_limits<size_t>::max())
    {
        return false;
    }

    std::stable_sort(images.begin(), images.end(), Comparators[bestIdx]);
    outSize = bestSize;
    return true;
}

// Find the smallest atlas size that fits all images.
// Precomputes candidate sizes from startSize to maxSize, then binary
// searches for the first size that packs successfully (monotonic property:
// if packing succeeds at size N, it succeeds at any larger size).
// Returns false if no valid size was found within the limit.
bool cImageList::findMinimalAtlasSize(AtlasPacker* packer, ImageList& images, const sSize& startSize, sSize& outSize)
{
    // Build candidate sizes
    std::vector<sSize> candidates;
    sSize s = startSize;
    while (m_size.isGood(s))
    {
        candidates.push_back(s);
        s = m_size.nextSize(s, 8u);
    }

    if (candidates.empty())
    {
        return false;
    }

    // Binary search: find the smallest index where packing succeeds
    size_t lo = 0;
    size_t hi = candidates.size();
    bool found = false;

    while (lo < hi)
    {
        auto mid = lo + (hi - lo) / 2;
        if (prepareSize(packer, candidates[mid], images))
        {
            hi = mid;
            found = true;
        }
        else
        {
            lo = mid + 1;
        }
    }

    if (!found)
    {
        return false;
    }

    outSize = candidates[lo];
    return true;
}

bool cImageList::prepareSize(AtlasPacker* packer, const sSize& atlasSize, const ImageList& images)
{
    packer->setSize(atlasSize);
    for (auto img : images)
    {
        if (packer->add(img) == false)
        {
            return false;
        }
    }

    return true;
}

void cImageList::writeXmlHeader(cFile& xmlFile, const char* outputResName)
{
    if (outputResName == nullptr)
    {
        return;
    }

    assert(xmlFile.isOpened() == false);

    if (xmlFile.open(outputResName, "w"))
    {
        std::string out = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<atlas>\n";
        xmlFile.write(out.c_str(), out.length());
    }
    else
    {
        cLog::Error("Error writing atlas description '{}'.", outputResName);
    }
}

void cImageList::writeXmlFooter(cFile& xmlFile, const char* outputResName)
{
    if (xmlFile.isOpened())
    {
        std::string out = "</atlas>\n";
        xmlFile.write(out.c_str(), out.length());

        if (outputResName != nullptr)
        {
            cLog::Info("Atlas description '{}' was created.", outputResName);
        }
    }
}
