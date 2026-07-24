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
#include "Atlas/MaxRectsPacker.h"
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
#include <unordered_set>
#include <vector>

namespace
{
    using Comparator = bool (*)(const cImage*, const cImage*);

    // A packing algorithm paired with a sprite sort order to try. The search
    // evaluates every strategy of the selected algorithm (all algorithms for
    // Auto) and keeps whichever yields the smallest atlas.
    struct sStrategy
    {
        sConfig::Algorithm algorithm;
        Comparator comparator;
    };

    std::vector<sStrategy> getStrategies(sConfig::Algorithm algorithm)
    {
        static const sStrategy KDTree[] = {
            { sConfig::Algorithm::KDTree, KDTreePacker::Compare },
            { sConfig::Algorithm::KDTree, KDTreePacker::CompareAlt },
        };
        static const sStrategy MaxRects[] = {
            { sConfig::Algorithm::MaxRects, MaxRectsPacker::Compare },
            { sConfig::Algorithm::MaxRects, MaxRectsPacker::CompareAlt },
        };

        std::vector<sStrategy> result;
        if (algorithm == sConfig::Algorithm::KDTree || algorithm == sConfig::Algorithm::Auto)
        {
            result.insert(result.end(), std::begin(KDTree), std::end(KDTree));
        }
        if (algorithm == sConfig::Algorithm::MaxRects || algorithm == sConfig::Algorithm::Auto)
        {
            result.insert(result.end(), std::begin(MaxRects), std::end(MaxRects));
        }

        return result;
    }

    // Bounding box area of the packed sprites; approximates the atlas after its
    // right/bottom transparent margin is trimmed, so it ranks how tightly a
    // strategy actually packs (the search canvas can be equal for both).
    uint64_t contentArea(const AtlasPacker* packer)
    {
        uint32_t right = 0;
        uint32_t bottom = 0;
        for (uint32_t i = 0; i < packer->getRectsCount(); ++i)
        {
            auto& rc = packer->getRectByIndex(i);
            right = std::max(right, rc.right);
            bottom = std::max(bottom, rc.bottom);
        }

        return static_cast<uint64_t>(right) * bottom;
    }

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
    if (size.width == 0 || size.height == 0)
    {
        return Result::Empty;
    }
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
    if (writeXmlHeader(xmlFile, outputResName) == false)
    {
        return false;
    }

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

        if (packer->buildAtlas() == false)
        {
            success = false;
            break;
        }

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
        const std::unordered_set<const cImage*> packed(packedImages.begin(), packedImages.end());
        remainingImages.erase(
            std::remove_if(remainingImages.begin(), remainingImages.end(),
                           [&packed](const cImage* img) {
                               return packed.count(img) != 0;
                           }),
            remainingImages.end());

        if (remainingImages.size() == prevCount)
        {
            cLog::Error("Failed to pack any images into atlas #{}.", atlasIndex);
            success = false;
            break;
        }

        atlasSize = finalSize;
        atlasIndex++;
    }

    if (writeXmlFooter(xmlFile, outputResName) == false)
    {
        success = false;
    }

    return success;
}

bool cImageList::packSingleAtlas(const char* desiredAtlasName, const char* outputResName,
                                 const char* resPathPrefix, sSize& atlasSize)
{
    atlasSize = m_size.calcSize();

    auto startTime = getCurrentTime();

    auto algorithm = m_config.algorithm;
    bool sized = m_size.isGood(atlasSize);
    if (sized)
    {
        const sSize maxSize{ m_config.maxAtlasSize, m_config.maxAtlasSize };
        sized = findBestStrategy(m_images, atlasSize, maxSize, atlasSize, algorithm);
    }

    if (sized == false)
    {
        cLog::Error("Cannot fit images within the maximum atlas size {} x {}.",
                    m_config.maxAtlasSize, m_config.maxAtlasSize);
        return false;
    }

    auto packer = AtlasPacker::createPacker(algorithm, m_config);

    cLog::Info("Packing atlas:");
    cLog::Info(" - size: {} x {}", atlasSize.width, atlasSize.height);

    if (prepareSize(packer.get(), atlasSize, m_images) == false)
    {
        cLog::Error("Cannot pack images into atlas {} x {}.", atlasSize.width, atlasSize.height);
        return false;
    }

    auto spritesArea = m_size.getArea();
    if (packer->buildAtlas() == false)
    {
        return false;
    }

    cFile xmlFile;
    if (writeXmlHeader(xmlFile, outputResName) == false)
    {
        return false;
    }

    auto success = saveAtlas(packer.get(), desiredAtlasName, resPathPrefix, xmlFile,
                             atlasSize, spritesArea, startTime);

    if (writeXmlFooter(xmlFile, outputResName) == false)
    {
        success = false;
    }

    return success;
}

bool cImageList::packImagesToMaxSize(ImageList& remainingImages, const sSize& maxSize, ImageList& outPackedImages)
{
    outPackedImages.clear();

    ImageList bestPacked;
    Comparator bestComparator = nullptr;

    for (const auto& strategy : getStrategies(m_config.algorithm))
    {
        auto sorted = remainingImages;
        std::stable_sort(sorted.begin(), sorted.end(), strategy.comparator);

        auto packer = AtlasPacker::createPacker(strategy.algorithm, m_config);
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
            bestComparator = strategy.comparator;
        }
    }

    if (bestComparator == nullptr)
    {
        return false;
    }

    // Apply the winning sort to remainingImages for correct downstream order
    std::stable_sort(remainingImages.begin(), remainingImages.end(), bestComparator);
    outPackedImages = std::move(bestPacked);

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

    auto algorithm = m_config.algorithm;
    if (findBestStrategy(packedImages, startSize, maxSize, outFinalSize, algorithm) == false)
    {
        return false;
    }

    packer = AtlasPacker::createPacker(algorithm, m_config);

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
        if (packer->generateResFile(xmlFile, atlasPath) == false)
        {
            cLog::Error("Error writing atlas description for '{}'.", outputAtlasName);
            return false;
        }
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

// Try every strategy (algorithm + sort order) of the configured algorithm
// (all algorithms for Auto) and keep the one that produces the smallest atlas.
// The winning algorithm is returned so the caller can build the matching
// packer; images are left sorted in the winning order.
bool cImageList::findBestStrategy(ImageList& images, const sSize& startSize, const sSize& maxSize,
                                  sSize& outSize, sConfig::Algorithm& outAlgorithm)
{
    // Copy the start size: the caller may pass the same variable as outSize.
    const sSize start = startSize;

    sSize bestSize{ 0, 0 };
    auto bestArea = std::numeric_limits<uint64_t>::max();
    auto bestAlgorithm = m_config.algorithm;
    Comparator bestComparator = nullptr;
    bool found = false;

    for (const auto& strategy : getStrategies(m_config.algorithm))
    {
        auto sorted = images;
        std::stable_sort(sorted.begin(), sorted.end(), strategy.comparator);

        auto packer = AtlasPacker::createPacker(strategy.algorithm, m_config);
        sSize foundSize;
        if (findMinimalAtlasSize(packer.get(), sorted, start, foundSize) == false)
        {
            // Growth may step over maxSize; try maxSize as a fallback
            if (prepareSize(packer.get(), maxSize, sorted) == false)
            {
                continue;
            }
            foundSize = maxSize;
        }

        // Rank by the packed content bounds, not the search canvas: two
        // algorithms often need the same canvas, but the tighter one trims to a
        // smaller final atlas.
        prepareSize(packer.get(), foundSize, sorted);
        const auto area = contentArea(packer.get());
        if (area < bestArea)
        {
            bestArea = area;
            bestSize = foundSize;
            bestAlgorithm = strategy.algorithm;
            bestComparator = strategy.comparator;
            found = true;
        }
    }

    if (found == false)
    {
        return false;
    }

    std::stable_sort(images.begin(), images.end(), bestComparator);
    outSize = bestSize;
    outAlgorithm = bestAlgorithm;
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

    if (found == false)
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

bool cImageList::writeXmlHeader(cFile& xmlFile, const char* outputResName)
{
    if (outputResName == nullptr)
    {
        return true;
    }

    assert(xmlFile.isOpened() == false);

    if (xmlFile.open(outputResName, "w") == false)
    {
        cLog::Error("Error writing atlas description '{}'.", outputResName);
        return false;
    }

    std::string out = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<atlas>\n";
    return xmlFile.write(out.c_str(), out.length()) == out.length();
}

bool cImageList::writeXmlFooter(cFile& xmlFile, const char* outputResName)
{
    if (xmlFile.isOpened() == false)
    {
        return true;
    }

    std::string out = "</atlas>\n";
    if (xmlFile.write(out.c_str(), out.length()) != out.length())
    {
        cLog::Error("Error writing atlas description '{}'.", outputResName);
        return false;
    }

    if (outputResName != nullptr)
    {
        cLog::Info("Atlas description '{}' was created.", outputResName);
    }

    return true;
}
