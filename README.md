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

## XEX uses raw LZX, not CAB

Worth stating because it governs which libraries are even the right shape:
XexTool contains no reference to CAB. The compression callback receives raw LZX
blocks and XexTool applies its own framing -- a big-endian 16-bit compressed
length per block, then the whole stream split into 0x10000 hashed blocks with
`XexHash` headers. The window is 0x8000 and input is fed in 0x8000 blocks.

So what is needed is a **raw LZX codec**, buffer in and buffer out. A CAB-level
compressor would be the wrong shape even if one existed, since the LZX stream
would be wrapped in a container we do not want. This is also why libmspack's
decoder works here at all: `libXexUnpack` drives `lzxd_init` and
`lzxd_decompress` directly, with no CAB layer.

## ldic cannot simply be replaced

`ldic` is a 47-file LZX codec with both an encoder and a decoder, and XexTool
uses **both**:

| direction | ldic calls | call sites |
|---|---|---|
| decompress | `LdicCreateDecompression`, `LdicSetWindowData`, `LdicDecompress`, `LdicResetDecompression`, `LdicDestroyDecompression` | `XexPacker::unpackCompressed`, `unpackDeltaCompressed`, `XexPatcher::XexpDeltaDecompress` |
| compress | `LdicCreateCompression`, `LdicCompress`, `LdicFlushCompressorOutput`, `LdicDestroyCompression` | `XexPacker::packCompressed` |

libmspack (`third_party/libmspack`, submodule of kyz/libmspack) covers the
first row completely, including `lzxd_set_reference_data` -- the equivalent of
`LdicSetWindowData` that XEXP delta patching depends on.

It cannot cover the second row. `lzxc.c` upstream is a stub:

```c
/* LZX compression implementation */
#include <system.h>
#include <lzx.h>

/* todo */
```

Eighteen lines, and `qtmc.c` and `mszipc.c` are the same. libmspack decompresses
the Microsoft formats; it was never given compressors.

So the shape of the answer is fixed: **libmspack can replace ldic's decoder, and
nothing can currently replace its encoder.** Two workable options:

1. **Keep ldic** for both halves. The swap buys nothing.
2. **Hybrid**: libmspack for decompression, ldic's encoder for compression.
   Sheds most of ldic's 47 files, keeps a maintained decoder, and leaves the
   encoder as the only piece of legacy code.

`src/lzx/XexUnpack.*` is the in-memory wrapper from libXexUnpack, which adapts
mspack's file callbacks to plain buffers and exposes
`LZXUnpack(in, inSize, out, outSize, windowSize, &error)`. It is kept for option
2 but nothing uses it yet.

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
third_party/libmspack   submodule: github.com/kyz/libmspack
third_party/tinyxml     vendored
```

## Submodules

```
git clone --recurse-submodules <this repo>
```
