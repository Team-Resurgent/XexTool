# XexTool V2

A tidied XexTool: the same C/C++ tool with Visual Studio projects, with its
vendored dependencies replaced by submodules and its dead weight removed.

## What changed from the original

| component | original | here |
|---|---|---|
| XexTool sources | `xex_stuff/XexTool/src` + `distro/common` | `src/` |
| XeCrypt | vendored copy (14 files) | submodule, `third_party/XeCrypt` |
| tinyxml | vendored, built as a separate lib | vendored, `third_party/tinyxml` |
| mbedtls | vendored (449 files) | **removed** |
| ldic | vendored LZX codec (47 files) | **kept** -- see below |

### mbedtls was unused

Nothing in the original included it, and no project file referenced it --
`XexTool.vcxproj` links `XeCrypt`, `ldic` and `tinyxml` only. It accounted for
roughly three quarters of the source tree.

## ldic cannot simply be replaced

`ldic` is a 47-file LZX codec with both an encoder and a decoder, and XexTool
uses **both**:

| direction | ldic calls | call sites |
|---|---|---|
| decompress | `LdicCreateDecompression`, `LdicSetWindowData`, `LdicDecompress`, `LdicResetDecompression`, `LdicDestroyDecompression` | `XexPacker::unpackCompressed`, `unpackDeltaCompressed`, `XexPatcher::XexpDeltaDecompress` |
| compress | `LdicCreateCompression`, `LdicCompress`, `LdicFlushCompressorOutput`, `LdicDestroyCompression` | `XexPacker::packCompressed` |

The libmspack subset in `third_party/mspack` is `lzxd.c` -- decompression only.
It covers the first row completely, including `lzxd_set_reference_data`, which
is the equivalent of `LdicSetWindowData` that XEXP delta patching depends on.
It cannot cover the second row at all.

So swapping wholesale to libmspack would **remove** `packCompressed`, and with
it the ability to create compressed XEXs. Three options:

1. **Keep ldic.** It works and has both halves. The mspack swap buys nothing.
2. **Hybrid**: libmspack for decompression, ldic's encoder for compression.
   Sheds most of ldic's 47 files but keeps the encoder.
3. **Find a library with both.** libmspack upstream has no LZX compressor.

`third_party/mspack` and `src/lzx/XexUnpack.*` are kept in the tree for option 2,
but nothing uses them yet.

## Still to do

### Decide the ldic question

Nothing is rewired yet; `XexPacker.cpp` and `XexPatcher.cpp` still include
`Ldic.h`, and `ldic` itself has not been copied into this tree.

### Build files

The Visual Studio projects have not been brought across yet; the original ones
reference the old layout and the removed dependencies.

## Layout

```
src/                    XexTool sources
third_party/XeCrypt     submodule: github.com/team-Resurgent/XeCrypt
third_party/tinyxml     vendored
```

## Submodules

```
git clone --recurse-submodules <this repo>
```
