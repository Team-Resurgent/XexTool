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

Four projects: `XexTool`, `mspack` (libmspack's LZX decoder), `ldic`, and a
reference to the XeCrypt submodule's own project. tinyxml2 is a single
translation unit and is compiled directly into `XexTool`.

## What changed

| component | before | now |
|---|---|---|
| sources | `xex_stuff/XexTool/src` + `distro/common` | `src/` |
| XeCrypt | vendored copy | submodule, `third_party/XeCrypt` |
| tinyxml | vendored, unmaintained | tinyxml2, submodule |
| mbedtls | vendored, 449 files | **removed** |
| ldic | vendored LZX codec | still present; see below |

### mbedtls was unused

Nothing included it and no project file referenced it -- the original linked
`XeCrypt`, `ldic` and `tinyxml` only. It was roughly three quarters of the
source tree.

### tinyxml to tinyxml2

The two are not API-compatible in general, but the usage here was three
declarations in `main.cpp` reading the `-i` info file. `LoadFile`,
`RootElement`, `Value`, `FirstChildElement`, `NextSiblingElement` and
`Attribute` all exist in tinyxml2 under the same names, with `Value()`
inherited from `XMLNode`, so the port was the type names, the namespace, and
`LoadFile` returning `XMLError` rather than `bool`.

### Porting to this XeCrypt

The team-Resurgent XeCrypt's API differs from the copy that was vendored:

| previously | here |
|---|---|
| `XeShaContext` | `XECRYPT_SHA_STATE` |
| `XeHmacShaContext` | `XECRYPT_HMAC_SHA_STATE` |
| `XeAesContext` | `XECRYPT_AES_STATE` |
| `XeRsaKey` | `XECRYPT_RSA` -- identical layout: `u32` count, `u32` exponent, `u64` reserved |
| signature buffer as `u64*` | `PXECRYPT_SIG`, a 256-byte layout over the same buffer |
| `XE_CRYPT_ENC` / `XE_CRYPT_DEC` | a `BOOL fEncrypt`; the original enum was `DEC = 0`, `ENC = 1`, so the mapping is direct |
| `XeCryptBnQwNeModExp` | present, declared in `xecryptBn.h` rather than `xecrypt.h` |
| `XeCryptHmacShaInit` / `Update` / `Final` | contributed to the XeCrypt fork -- only the three-buffer one-shot existed |

Two further wrinkles:

- `src/types.h` defined `s8` as `signed char` and `xecryptTypes.h` as `char`.
  Those are distinct types in C++ even where `char` is signed, so they
  collided; `types.h` now matches.
- This XeCrypt takes mutable input buffers where XexTool passes const ones.
  `src/XeCryptCompat.h` supplies const-qualified overloads that forward, rather
  than casting at each call site.

### XGetopt

The original compiled `$(COMMON_PATH)\XGetopt.c`, which is absent from the
source this was taken from -- only the header survived. `src/XGetopt.c` is a
fresh implementation of the same interface following POSIX getopt semantics.

## XEX uses raw LZX, not CAB

Worth stating because it governs which libraries are the right shape. There is
no CAB anywhere: the compression callback receives raw LZX blocks and XexTool
applies its own framing -- a big-endian 16-bit compressed length per block, then
the stream split into 0x10000 hashed blocks with `XexHash` headers, over a
0x8000 window.

So what is needed is a raw LZX codec, buffer in and buffer out. A CAB-level
compressor would be the wrong shape even if one existed. It is also why
libmspack's decoder suits: `src/lzx/XexUnpack.*` drives `lzxd_init` and
`lzxd_decompress` directly, with no CAB layer.

## The remaining ldic dependency

`ldic` is a 47-file LZX codec with an encoder and a decoder, and both are used:

| direction | call sites |
|---|---|
| decompress | `XexPacker::unpackCompressed`, `unpackDeltaCompressed`, `XexPatcher::XexpDeltaDecompress` |
| compress | `XexPacker::packCompressed` |

libmspack covers the first row completely, including `lzxd_set_reference_data`,
the equivalent of `LdicSetWindowData` that XEXP delta patching depends on. It
cannot cover the second: `lzxc.c` upstream is eighteen lines of licence header
and `/* todo */`, as are `qtmc.c` and `mszipc.c`, and
`mspack_create_cab_compressor()` returns `NULL`. libmspack declares the
compressor API and never implemented it.

The plan is to implement LZX compression in
[Team-Resurgent/libmspack](https://github.com/Team-Resurgent/libmspack), giving
one cross-platform codec for both directions and removing `ldic` entirely.
`ldic` is deliberately a separate project rather than folded into `XexTool`, so
removing it later is deleting one project, one solution entry and one reference.

### Verifying a replacement compressor

`ldic` stays meanwhile as a reference. Output need not be byte-identical -- two
LZX encoders may make different valid choices -- so the checks are behavioural:

1. **Round trip**: our compressor then our decompressor returns the input.
2. **Cross-check both ways**: our compressor into ldic's decompressor, and
   ldic's compressor into our decompressor. This is the one that catches
   stream-format mistakes a self-consistent round trip would hide.
3. **Real data**: both over actual XEX basefiles, comparing decompressed
   output rather than compressed size.

## Layout

```
src/                    XexTool sources
src/lzx/                buffer shims over libmspack's lzxd
msvc/                   Visual Studio solution and projects
third_party/XeCrypt     submodule: github.com/Team-Resurgent/XeCrypt
third_party/libmspack   submodule: github.com/Team-Resurgent/libmspack
third_party/tinyxml2    submodule: github.com/leethomason/tinyxml2
third_party/ldic        LZX codec, to be replaced
```

## Credits

XexTool is xorloser's work, 2006-2017. XeCrypt, libmspack, tinyxml2 and ldic
carry their own licences; see their respective directories.
