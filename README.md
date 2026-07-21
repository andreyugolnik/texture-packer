# Texture Packer

[![Build status: master](https://ci.appveyor.com/api/projects/status/44jt8fun84h63wyw/branch/master?svg=true)](https://ci.appveyor.com/project/reybits/texture-packer/branch/master "Branch: master")

![Texture Packer — combines multiple sprites into an optimized atlas](res/banner.png)

The Texture Packer tool efficiently combines multiple images into a single optimized texture atlas, minimizing memory usage and boosting rendering performance. By leveraging advanced packing algorithms, it maximizes space utilization and streamlines asset management for game development and UI design, enabling faster load times and simplified workflow.

## Key features

- Automatically discovers images from folders or accepts them via the command line.
- Supports input formats: JPEG, PNG, TGA, BMP, PSD, GIF, HDR, PIC, PNM.
- Exports to PNG (default), TGA, and BMP with accompanying XML metadata.
- Provides two packing algorithms: KD-Tree (default) and Classic.
- Supports multi-atlas output when images exceed the maximum atlas size.
- Trims transparent borders from input images to save space.
- Adds configurable border and padding around sprites.
- Supports power-of-two atlas dimensions.

## Usage

```sh
texpacker INPUT_IMAGE [INPUT_IMAGE] <OPTIONS> --atlas=PATH
  INPUT_IMAGE        Input image file or directory (space-separated)
  --algorithm=NAME   Packing algorithm (kdtree or classic, default: kdtree)
  --allow-dupes      Allow duplicate sprites (default: false)
  --anchor-only      Omit hotspot, keep anchor only (default: false)
  --atlas-size=SIZE  Maximum atlas size (default: 2048 px)
  --atlas=PATH       Output atlas file name (default: PNG)
  --border=SIZE      Add border around sprites (default: 0 px)
  --drop-ext         Remove file extension from sprite ID (default: false)
  --keep-float       Preserve float hotspot coordinates (default: false)
  --multi-atlas      Enable multi-atlas output (default: false)
  --no-recurse       Do not search subdirectories
  --overlay          Overlay sprites (default: false)
  --padding=SIZE     Add padding between sprites (default: 1 px)
  --pot              Make atlas dimensions power of two (default: false)
  --prefix=PREFIX    Add prefix to texture path
  --trim-id=COUNT    Remove COUNT characters from the start of sprite IDs (default: 0)
  --trim-sprite      Trim transparent borders from sprites (default: false)
  --xml=PATH         The output file path for the atlas description in XML format
```

## Download and build

Requires CMake 3.22+ and a C++17 compiler. Works on Linux, macOS, and BSD.

```sh
git clone https://github.com/reybits/texture-packer.git
cd texture-packer
make release
```

## Install with homebrew

```sh
brew tap reybits/homebrew-tap
brew install reybits/homebrew-tap/texture-packer
```

## Testing

Run the verification test suite to check atlas output against committed reference files:

```sh
./tests/verify.sh
```

This runs texpacker with several configurations (single atlas, multi-atlas, classic algorithm, power-of-two, border/padding) and compares the resulting atlas images and XML metadata byte-for-byte against reference output.

To regenerate reference files after an intentional change in packing behavior:

```sh
./tests/verify.sh --update
```

## Input format notes

- **JPEG** baseline & progressive (12 bpc/arithmetic not supported, same as stock IJG lib).
- **PNG** 1/2/4/8-bit-per-channel (16 bpc not supported).
- **TGA** (not sure what subset, if a subset).
- **BMP** non-1bpp, non-RLE.
- **PSD** (composited view only, no extra channels, 8/16 bit-per-channel).
- **GIF** (*comp always reports as 4-channel).
- **HDR** (radiance rgbE format).
- **PIC** (Softimage PIC).
- **PNM** (PPM and PGM binary only).

***

```
Copyright © 2017-2026 Andrey A. Ugolnik. All Rights Reserved.
https://github.com/reybits
and@reybits.dev
```
