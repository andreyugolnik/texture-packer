# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [1.5.1] - 2026-07-25

### Added

- Warn when the output atlas is fully transparent (all source sprites are
  empty) instead of silently writing a blank image.

### Fixed

- Heap buffer overflow (crash) when packing with `--algorithm=kdtree` or
  `--algorithm=auto` together with `--border`, when the atlas ended up smaller
  than the border. The default `maxrects` algorithm was not affected.
- Right/bottom atlas trim clipped one row/column of content and could emit
  sprite rectangles that reached past the atlas bounds; most visible with
  `--padding=0`. `rect` values now always stay inside the atlas. (Atlases may
  become a pixel or two larger where they were previously clipped, so
  regenerate them.)
- Possible stack overflow on very large sprite sets; K-D tree packing and
  cleanup are now iterative. Packing results are byte-for-byte identical.
- The `texture="..."` XML attribute is now escaped (`&`, `<`, `>`, `"`), so
  atlas paths containing those characters produce valid XML.
- The "atlas created" log line now reports the actual saved dimensions and
  fill percentage instead of the pre-trim size.

### Changed

- Internal: memory ownership modernized with `std::unique_ptr`, the pixel
  buffer made move-only, and dead code removed. No behavior change.

## [1.5.0] - 2026-07-24

### Added

- MaxRects and `auto` packing algorithms, replacing the classic algorithm.

[1.5.1]: https://github.com/reybits/texture-packer/compare/v1.5.0...v1.5.1
[1.5.0]: https://github.com/reybits/texture-packer/releases/tag/v1.5.0
