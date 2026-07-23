/**********************************************\
*
*  Andrey A. Ugolnik
*  http://www.ugolnik.info
*  andrey@ugolnik.info
*
\**********************************************/

#include "Config.h"
#include "FileList.h"
#include "ImageList.h"
#include "Log.h"
#include "Utils.h"

#include <cstdlib>
#include <cstring>
#include <string>

void showHelp(const char* name, const sConfig& config);

int main(int argc, char* argv[])
{
    sConfig config;

    cLog::Info("Texture Packer v1.4.0");
    cLog::Info("Copyright (c) 2017-2026 Andrey A. Ugolnik.");
    cLog::Info("");
    if (argc < 3)
    {
        showHelp(argv[0], config);
        return 0;
    }

    const char* outputAtlasName = nullptr;
    const char* outputResName = nullptr;
    const char* resPathPrefix = nullptr;

    cFileList fileList;

    uint32_t trimCount = 0u;
    auto recurse = true;

    for (int i = 1; i < argc; i++)
    {
        const char* arg = argv[i];

        if (arg[0] != '-')
        {
            fileList.addPath(trimCount, arg, recurse);
        }
        else if (isOption(arg, "--atlas="))
        {
            outputAtlasName = arg + litLen("--atlas=");
        }
        else if (isOption(arg, "--xml="))
        {
            outputResName = arg + litLen("--xml=");
        }
        else if (isOption(arg, "--prefix="))
        {
            resPathPrefix = arg + litLen("--prefix=");
        }
        else if (isOption(arg, "--border="))
        {
            if (parseUint(arg + litLen("--border="), config.border) == false)
            {
                cLog::Error("Invalid value for --border.");
                return -1;
            }
        }
        else if (isOption(arg, "--padding="))
        {
            if (parseUint(arg + litLen("--padding="), config.padding) == false)
            {
                cLog::Error("Invalid value for --padding.");
                return -1;
            }
        }
        else if (isOption(arg, "--atlas-size="))
        {
            if (parseUint(arg + litLen("--atlas-size="), config.maxAtlasSize) == false
                || config.maxAtlasSize == 0)
            {
                cLog::Error("Invalid value for --atlas-size.");
                return -1;
            }
        }
        else if (isOption(arg, "--trim-id="))
        {
            if (parseUint(arg + litLen("--trim-id="), trimCount) == false)
            {
                cLog::Error("Invalid value for --trim-id.");
                return -1;
            }
        }
        else if (isOption(arg, "--pot"))
        {
            config.pot = true;
        }
        else if (isOption(arg, "--multi-atlas"))
        {
            config.enableMultiAtlas = true;
        }
        else if (isOption(arg, "--keep-float"))
        {
            config.keepFloat = true;
        }
        else if (isOption(arg, "--anchor-only"))
        {
            config.anchorOnly = true;
        }
        else if (isOption(arg, "--trim-sprite"))
        {
            config.trimSprite = true;
        }
        else if (isOption(arg, "--allow-dupes"))
        {
            config.alowDupes = true;
        }
        else if (isOption(arg, "--algorithm="))
        {
            auto value = arg + litLen("--algorithm=");
            config.algorithm = sConfig::ToAlgorithm(value);
        }
        else if (isOption(arg, "--overlay"))
        {
            config.overlay = true;
        }
        else if (isOption(arg, "--no-recurse"))
        {
            recurse = false;
        }
        else
        {
            cLog::Warning("Unknown option: '{}'.", arg);
        }
    }

    if (outputAtlasName == nullptr)
    {
        cLog::Info("No output name defined.");
        return -1;
    }

    config.dump();
    if (resPathPrefix != nullptr)
    {
        cLog::Info("Path prefix:        {}.", resPathPrefix);
    }
    cLog::Info("");

    auto startTime = getCurrentTime();

    // sort and remove dupes
    if (config.alowDupes == false)
    {
        fileList.removeDupes();
    }

    // load images
    auto& files = fileList.getList();
    cImageList imageList(config, files.size());

    for (const auto& f : files)
    {
        auto result = imageList.loadImage(f.path, f.trimCount);
        switch (result)
        {
        case cImageList::Result::OK:
            break;

        case cImageList::Result::NotAnImage:
            break;

        case cImageList::Result::CannotOpen:
            cLog::Warning("File '{}' not loaded.", f.path);
            break;

        case cImageList::Result::Empty:
            cLog::Warning("Image '{}' is empty and was skipped.", f.path);
            break;

        case cImageList::Result::TooBig:
            cLog::Error("Image '{}' is too large for the atlas (max size: {} x {}).",
                        f.path,
                        config.maxAtlasSize, config.maxAtlasSize);

            return -1;
        }
    }

    auto& images = imageList.getList();

    cLog::Info("Loaded {} ({}) images in {:.2f} ms.",
               static_cast<uint32_t>(images.size()),
               static_cast<uint32_t>(files.size()),
               (getCurrentTime() - startTime) * 0.001f);

    // packing
    sSize atlasSize;
    if (imageList.doPacking(outputAtlasName, outputResName, resPathPrefix, atlasSize) == false)
    {
        cLog::Info("");
        cLog::Info("Desired atlas size {} x {}, but maximum {} x {}.",
                   atlasSize.width, atlasSize.height,
                   config.maxAtlasSize, config.maxAtlasSize);

        return -1;
    }

    return 0;
}

void showHelp(const char* name, const sConfig& config)
{
    cLog::Info("Usage:");
    auto p = ::strrchr(name, '/');
    name = p != nullptr
        ? p + 1
        : name;
    cLog::Info("  {} INPUT_IMAGE [INPUT_IMAGE] <OPTIONS> --atlas=PATH", name);
    cLog::Info("");
    cLog::Info("  INPUT_IMAGE        Input image file or directory (space-separated)");
    cLog::Info("  --algorithm=NAME   Packing algorithm (kdtree or classic, default: {})", sConfig::ToName(config.algorithm));
    cLog::Info("  --allow-dupes      Allow duplicate sprites (default: {})", toString(config.alowDupes));
    cLog::Info("  --anchor-only      Omit hotspot, keep anchor only (default: {})", toString(config.anchorOnly));
    cLog::Info("  --atlas-size=SIZE  Maximum atlas size (default: {} px)", config.maxAtlasSize);
    cLog::Info("  --atlas=PATH       Output atlas file name (default: PNG)");
    cLog::Info("  --border=SIZE      Add border around sprites (default: {} px)", config.border);
    cLog::Info("  --keep-float       Preserve float hotspot coordinates (default: {})", toString(config.keepFloat));
    cLog::Info("  --multi-atlas      Enable multi-atlas output (default: {})", toString(config.enableMultiAtlas));
    cLog::Info("  --no-recurse       Do not search subdirectories");
    cLog::Info("  --overlay          Overlay sprites (default: {})", toString(config.overlay));
    cLog::Info("  --padding=SIZE     Add padding between sprites (default: {} px)", config.padding);
    cLog::Info("  --pot              Make atlas dimensions power of two (default: {})", toString(config.pot));
    cLog::Info("  --prefix=PREFIX    Add prefix to texture path");
    cLog::Info("  --trim-id=COUNT    Remove COUNT characters from the start of sprite IDs (default: 0)");
    cLog::Info("  --trim-sprite      Trim transparent borders from sprites (default: {})", toString(config.trimSprite));
    cLog::Info("  --xml=PATH         The output file path for the atlas description in XML format");
}
