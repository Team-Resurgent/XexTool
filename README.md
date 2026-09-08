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
| ldic | vendored LZX codec (47 files) | libmspack, `third_party/mspack` (8 files) |

### mbedtls was unused

Nothing in the original included it, and no project file referenced it --
`XexTool.vcxproj` links `XeCrypt`, `ldic` and `tinyxml` only. It accounted for
roughly three quarters of the source tree.

## Replacing ldic with libmspack

`ldic` is a 47-file LZX codec with both an encoder and a decoder, but XexTool
only ever calls the decoder, from `XexPacker.cpp` (`unpackCompressed`,
`unpackDeltaCompressed`) and `XexPatcher.cpp` (`XexpDeltaDecompress`).

The libmspack subset under `third_party/mspack` covers every one of those calls
in 8 files:

| ldic | libmspack |
|---|---|
| `LdicCreateDecompression` | `lzxd_init` |
| `LdicSetWindowData` | `lzxd_set_reference_data` |
| `LdicDecompress` | `lzxd_decompress` |
| `LdicDestroyDecompression` | `lzxd_free` |
| `LdicResetDecompression` | re-initialise; no direct equivalent |

`lzxd_set_reference_data` is the one that matters -- seeding the LZX window is
what XEXP delta patches need, and without it the patcher could not be ported.

`src/lzx/XexUnpack.*` is the in-memory wrapper from libXexUnpack, which adapts
mspack's file callbacks to plain buffers and exposes
`LZXUnpack(in, inSize, out, outSize, windowSize, &error)`.

Note this is `lzxd.c` only: **decompression**. Creating compressed XEXs, which
RXDK-360 will need in order to turn a linked image into a XEX, still requires an
LZX compressor from somewhere.

## Still to do

### Rewire the ldic call sites

`XexPacker.cpp` and `XexPatcher.cpp` still call the `Ldic*` API and include
`Ldic.h`; they need to move onto the wrapper above.

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
