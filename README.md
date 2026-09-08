# XexTool V2

A tidied XexTool: the same C/C++ tool with Visual Studio projects, with its
vendored dependencies replaced by submodules and its dead weight removed.

## What changed from the original

| component | original | here |
|---|---|---|
| XexTool sources | `xex_stuff/XexTool/src` + `distro/common` | `src/` |
| XeCrypt | vendored copy (14 files) | submodule, `third_party/XeCrypt` |
| tinyxml | vendored, built as a separate lib | tinyxml2, submodule, `third_party/tinyxml2` |
| mbedtls | vendored (449 files) | **removed** |
| ldic | vendored LZX codec (47 files) | **kept** -- see below |

### tinyxml to tinyxml2

tinyxml1 is unmaintained, so this moves to tinyxml2 as a submodule. The two are
not API-compatible in general, but XexTool's usage was narrow enough that it did
not matter: three declarations in `main.cpp`, reading the `-i` info file.

`LoadFile`, `RootElement`, `Value`, `FirstChildElement`, `NextSiblingElement`
and `Attribute` all exist in tinyxml2 under the same names -- `Value()` is
inherited from `XMLNode` -- so the port was the type names, the namespace, and
`LoadFile` returning `XMLError` rather than `bool`.

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

## The LZX plan

`ldic` is a 47-file LZX codec with both an encoder and a decoder, and XexTool
uses both:

| direction | ldic calls | call sites |
|---|---|---|
| decompress | `LdicCreateDecompression`, `LdicSetWindowData`, `LdicDecompress`, `LdicResetDecompression`, `LdicDestroyDecompression` | `XexPacker::unpackCompressed`, `unpackDeltaCompressed`, `XexPatcher::XexpDeltaDecompress` |
| compress | `LdicCreateCompression`, `LdicCompress`, `LdicFlushCompressorOutput`, `LdicDestroyCompression` | `XexPacker::packCompressed` |

`third_party/libmspack` is Team-Resurgent's fork, currently identical to
upstream at `55d5019`. Its decoder covers the first row completely, including
`lzxd_set_reference_data` -- the equivalent of `LdicSetWindowData` that XEXP
delta patching depends on.

Its encoder does not exist. `lzxc.c` upstream is eighteen lines:

```c
/* LZX compression implementation */
#include <system.h>
#include <lzx.h>

/* todo */
```

`qtmc.c` and `mszipc.c` are the same, and `mspack_create_cab_compressor()`
returns `NULL`. libmspack declares the compressor API and never implemented it.

**So the plan is to implement LZX compression in the fork.** That keeps one
cross-platform codec for both directions and removes the dependency on ldic,
which is Windows-oriented legacy code.

The fork has a working clone at `D:\Git\libmspack`. Compressor work happens
there and is pushed to Team-Resurgent/libmspack; this repository then just moves
its submodule pointer.

### Verifying a new compressor

`ldic` stays in the tree initially as a reference implementation to check
against. Output need not be byte-identical -- two LZX encoders may make
different valid choices -- so the checks are behavioural:

1. **Round trip**: our compressor then our decompressor returns the input.
2. **Cross-check both ways**: our compressor into ldic's decompressor, and
   ldic's compressor into our decompressor. This is the one that catches
   stream-format mistakes a self-consistent round trip would hide.
3. **Real data**: run both over actual XEX basefiles and compare
   decompressed output, not compressed size.

## Building

`msvc/XexTool.sln`, Visual Studio 2022 format (2026 opens it), Debug/Release for
Win32 and x64. Four projects:

| project | what |
|---|---|
| `XexTool` | the tool, with tinyxml2 compiled in as a single translation unit |
| `mspack` | libmspack's LZX decoder |
| `ldic` | **temporary** -- the LZX compressor, until libmspack has one |
| `XeCrypt` | referenced from the submodule's own `XeCrypt.2019.vcxproj` |

`ldic` is deliberately its own project rather than folded into `XexTool`, so
removing it later is deleting one project, one solution entry and one reference.

### Porting to this XeCrypt

It builds and runs: `msvc/build/x64/Release/XexTool.exe`, reporting
`XexTool v7.0`.

Getting there meant reconciling XexTool with the team-Resurgent XeCrypt, whose
API differs from the copy the original vendored:

| what XexTool used | here |
|---|---|
| `XeShaContext` | `XECRYPT_SHA_STATE` |
| `XeHmacShaContext` | `XECRYPT_HMAC_SHA_STATE` |
| `XeAesContext` | `XECRYPT_AES_STATE` |
| `XeRsaKey` | `XECRYPT_RSA` -- identical layout, `u32` count, `u32` exponent, `u64` reserved |
| signature buffer as `u64*` | `PXECRYPT_SIG`, a 256-byte layout over the same buffer |
| `XE_CRYPT_ENC` / `XE_CRYPT_DEC` | a `BOOL fEncrypt`; the original enum was `DEC = 0`, `ENC = 1`, so the mapping is direct |
| `XeCryptBnQwNeModExp` | present, but declared in `xecryptBn.h` rather than `xecrypt.h` |
| `XeCryptHmacShaInit` / `Update` / `Final` | **added to the fork** -- only the three-buffer one-shot existed |

Two further wrinkles:

- `src/types.h` defined `s8` as `signed char` and `xecryptTypes.h` as `char`.
  Those are distinct types in C++ even where `char` is signed, so they
  collided; `types.h` now matches.
- This XeCrypt takes mutable input buffers where XexTool passes const ones.
  `src/XeCryptCompat.h` supplies const-qualified overloads that forward, rather
  than casting at each call site.

### XGetopt

The original project compiled `$(COMMON_PATH)\XGetopt.c`, which is not present
in the source it was taken from -- only the header survived. `src/XGetopt.c` is
a fresh implementation of the same interface following POSIX getopt semantics.

### The XeCrypt dependency is not published yet

The streaming HMAC is committed to `D:\Git\XeCrypt` but not pushed, so the
submodule still points at a commit without it. Until that is pushed and the
pointer moved, a fresh clone will not link.

## Still to do

### Move the decoder onto libmspack

`XexPacker.cpp` and `XexPatcher.cpp` still include `Ldic.h`, and `ldic` has not
been copied into this tree yet. The decode side can move first, since libmspack
already covers it.

### Implement LZX compression in the fork

The remaining dependency on ldic, and the piece RXDK-360 needs in order to turn
a linked image into a XEX.

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
