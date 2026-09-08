# XexTool

<p align="center"><b>Xbox 360 XEX inspection, extraction and patching — cross-platform dependencies, modern toolchain</b></p>

<p align="center">
  <a href="https://github.com/Team-Resurgent/XexTool/blob/main/LICENSE.md"><img src="https://img.shields.io/badge/License-GPLv3-blue.svg" alt="License: GPL v3"></a>
  <a href="https://github.com/Team-Resurgent/XexTool/actions/workflows/build-xextool.yml"><img src="https://github.com/Team-Resurgent/XexTool/actions/workflows/build-xextool.yml/badge.svg" alt="Build"></a>
  <a href="https://discord.gg/VcdSfajQGK"><img src="https://img.shields.io/badge/chat-on%20discord-7289da.svg?logo=discord" alt="Discord"></a>
</p>

<p align="center">
  <a href="https://ko-fi.com/J3J7L5UMN"><img src="https://ko-fi.com/img/githubbutton_sm.svg" alt="ko-fi"></a>
  <a href="https://www.patreon.com/teamresurgent"><img src="https://img.shields.io/badge/Patreon-F96854?style=for-the-badge&logo=patreon&logoColor=white" alt="Patreon"></a>
</p>

<p align="center">
  <a href="https://github.com/Team-Resurgent/XexTool/releases/latest"><img src="https://img.shields.io/badge/download-latest-brightgreen.svg?style=for-the-badge&logo=github" alt="Download"></a>
</p>

---

XexTool reads, dumps and patches Xbox 360 XEX executables. It originates with
xorloser's tool and keeps its behaviour and command line; what has changed is
everything underneath.

## Building

```
git clone --recurse-submodules https://github.com/Team-Resurgent/XexTool.git
```

Open `msvc/XexTool.sln` in Visual Studio 2022 or later, or:

```
msbuild msvc/XexTool.sln /p:Configuration=Release /p:Platform=x64
```

Three projects: `XexTool`, `lzx` (libLZX) and `xecrypt`. tinyxml2 is a single
translation unit and is compiled into `XexTool` directly.

## What changed

| component | before | now |
|---|---|---|
| sources | `xex_stuff/XexTool/src` + `distro/common` | `src/` |
| XeCrypt | vendored copy | submodule |
| tinyxml | vendored, unmaintained | tinyxml2, submodule |
| mbedtls | vendored, 449 files | **removed**, unused |
| ldic | vendored LZX codec, 47 files | **removed**, replaced by libLZX |
| libmspack | -- | briefly used for decoding, then replaced by libLZX |

Compression and decompression now both go through
[libLZX](https://github.com/Team-Resurgent/libLZX), which needed two additions:

- `lzx_create_compression_window()` / `lzx_create_decompression_window()`, since
  both sides were pinned to `LZX_WINDOW_SIZE` of 128KiB while a XEX records a
  32KiB window, and a decoder must use the window the encoder did;
- `lzx_set_window_data()`, which seeds the decoder's window the way ldic's
  `LZX_DecodeInsertDictionary` did -- reference at the end, zeros before it --
  which is what XEX delta patches need.

### mbedtls was unused

Nothing included it and no project file referenced it. It was roughly three
quarters of the original source tree.

### tinyxml to tinyxml2

The two are not API-compatible in general, but the usage here was three
declarations in `main.cpp`. `LoadFile`, `RootElement`, `Value`,
`FirstChildElement`, `NextSiblingElement` and `Attribute` all exist in tinyxml2
under the same names, so the port was the type names, the namespace, and
`LoadFile` returning `XMLError`.

### Porting to this XeCrypt

The team-Resurgent XeCrypt's API differs from the copy that was vendored:

| previously | here |
|---|---|
| `XeShaContext` | `XECRYPT_SHA_STATE` |
| `XeHmacShaContext` | `XECRYPT_HMAC_SHA_STATE` |
| `XeAesContext` | `XECRYPT_AES_STATE` |
| `XeRsaKey` | `XECRYPT_RSA` -- identical layout |
| signature buffer as `u64*` | `PXECRYPT_SIG`, the same 256 bytes |
| `XE_CRYPT_ENC` / `XE_CRYPT_DEC` | a `BOOL fEncrypt`; the enum was `DEC = 0`, `ENC = 1` |
| `XeCryptHmacShaInit` / `Update` / `Final` | contributed to the XeCrypt fork |

`src/types.h` defined `s8` as `signed char` and `xecryptTypes.h` as `char`,
which are distinct types in C++; `types.h` now matches. `src/XeCryptCompat.h`
supplies const-qualified overloads where this XeCrypt takes mutable buffers.

### XGetopt

The original compiled `$(COMMON_PATH)\XGetopt.c`, absent from the source this
was taken from. `src/XGetopt.c` is a fresh implementation of the same interface.

## XEX uses raw LZX, not CAB

There is no CAB anywhere: the compressor emits raw LZX blocks and XexTool
applies its own framing -- a big-endian 16-bit compressed length per block, then
the stream split into 0x10000 hashed blocks with `XexHash` headers, over a
32KiB window. libLZX frames its own output as
`{ uint16 compressed; uint16 uncompressed; }` little-endian, so the compressor
output is reframed on the way out.

## Verification

Against a reference binary built from the last commit that still used ldic:

```
basefile dump, all 35 XEXs in the September 2013 XDK recovery : 35 identical
GTA IV title update applied to its base xex                   : identical
compress with libLZX, decompress, compare to the original     : identical
```

30 of those 35 are compressed and the update is delta-compressed, so the plain
and delta decode paths and the compressor are all exercised. `dash.xex`
compresses to 6418432 bytes against ldic's 6416384.

## Layout

```
src/                    XexTool sources
tools/                  helper scripts
src/lzx/                LZX decode wrapper over libLZX
msvc/                   Visual Studio solution and projects
third_party/XeCrypt     submodule: github.com/Team-Resurgent/XeCrypt
third_party/libLZX      submodule: github.com/Team-Resurgent/libLZX
third_party/tinyxml2    submodule: github.com/leethomason/tinyxml2
```

## Credits

XexTool is xorloser's work, 2006-2017. XeCrypt, libLZX and tinyxml2 carry their
own licences; see their respective directories.
