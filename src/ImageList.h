/**********************************************\
*
*  Andrey A. Ugolnik
*  http://www.ugolnik.info
*  andrey@ugolnik.info
*
\**********************************************/

#pragma once

#include "Atlas/AtlasSize.h"
#include "Config.h"

#include <memory>
#include <string>

class AtlasPacker;
class cFile;
class cTrim;

class cImageList final
{
public:
    cImageList(const sConfig& config, uint32_t reserve);
    ~cImageList();

    cImageList(const cImageList&) = delete;
    cImageList& operator=(const cImageList&) = delete;

    enum class Result
    {
        OK,
        NotAnImage,
        CannotOpen,
        TooBig,
        Empty,
    };
    Result loadImage(const std::string& path, uint32_t trimCount);

    bool doPacking(const char* desiredAtlasName, const char* outputResName,
                   const char* resPathPrefix, sSize& atlasSize);

    const ImageList& getList() const
    {
        return m_images;
    }

private:
    bool packSingleAtlas(const char* desiredAtlasName, const char* outputResName,
                         const char* resPathPrefix, sSize& atlasSize);

    bool packMultiAtlas(const char* desiredAtlasName, const char* outputResName,
                        const char* resPathPrefix, sSize& atlasSize);

    bool packImagesToMaxSize(ImageList& remainingImages, const sSize& maxSize, ImageList& outPackedImages);
    bool optimizeAtlasSize(ImageList& packedImages, const sSize& maxSize, sSize& outFinalSize,
                           std::unique_ptr<AtlasPacker>& packer);

    bool saveAtlas(AtlasPacker* packer, const char* atlasName,
                   const char* resPathPrefix, cFile& xmlFile,
                   const sSize& atlasSize, uint64_t spritesArea, uint64_t startTime);

    bool findBestStrategy(ImageList& images, const sSize& startSize, const sSize& maxSize,
                          sSize& outSize, sConfig::Algorithm& outAlgorithm);
    bool findMinimalAtlasSize(AtlasPacker* packer, ImageList& images, const sSize& startSize, sSize& outSize);
    bool prepareSize(AtlasPacker* packer, const sSize& atlasSize, const ImageList& images);
    bool writeXmlHeader(cFile& xmlFile, const char* outputResName);
    bool writeXmlFooter(cFile& xmlFile, const char* outputResName);

private:
    const sConfig& m_config;
    cAtlasSize m_size;
    cTrim* m_trim;

private:
    ImageList m_images;
};
