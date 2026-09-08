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

| direction | call sites | status |
|---|---|---|
| decompress | `XexPacker::unpackCompressed` | **libmspack** |
| decompress | `XexPatcher::XexpDeltaDecompress` | **libmspack** |
| compress | `XexPacker::packCompressed` | ldic; nothing exists to replace it |

`unpackCompressed` is converted and verified. XexTool's 16-bit sizes are its own
framing: the payloads they delimit form one continuous LZX stream whose window
carries across them, so the stream is gathered and decompressed in a single
pass. Decompressing per chunk would reset the window and produce garbage.

It is verified two ways, both differential -- a round trip through libmspack
alone would pass even if the stream format had been misread.

**Against ldic's compressor.** Compress with ldic, decompress with libmspack:

```
$ XexTool -c c -o compressed.xex dash.xex     # ldic compressor
$ XexTool -b out.bin compressed.xex           # libmspack decompressor
16941056 bytes, sha256 06A8446849184331DC1B513A894C08AD3F160DCEC8473F70CDCAE9F54B7C86F9
```

identical to the basefile dumped straight from the uncompressed original.

**Against ldic's decompressor, over real retail XEXs.** Building the previous
commit as a reference binary and dumping the basefile of every XEX in the
September 2013 XDK recovery with both:

```
identical : 35
different : 0
skipped   : 0
```

30 of those 35 are compressed, so the decoder is genuinely exercised --
`AvatarEditor.xex` at 16 MB, `dash.xex`, `xam.xex`, `xshell.xex` and the rest
of the dashboard.

All three decode paths are on libmspack; only the compressor still uses ldic.

The delta paths took a fix in the libmspack fork. `lzxd_set_reference_data`
refused any stream not created with `is_delta`, but that flag conflates two
things: it selects the **LZX DELTA bitstream**, which carries a chunk_size field
and extended match lengths, and it restricts the window to 2^17..2^25. XEX
delta blocks are not LZX DELTA -- they are ordinary LZX decoded over a window
seeded with the region being patched, in a 32KiB window DELTA does not permit.
Since reference data's only effect on decoding is to widen one match-offset
bounds check, the restriction was dropped.

The second half was how much window to seed. ldic's `LZX_DecodeInsertDictionary`
seeds the **whole** window:

```c
memcpy(ctx_ptr + ctx_size - dictSize, dictData, dictSize);
if (dictSize < ctx_size)
    memset(ctx_ptr, 0, ctx_size - dictSize);
```

reference at the end, zeros before it. So the full 32KiB is addressable history
and the encoder emits offsets into the zeroed region. Seeding only the reference
bytes made libmspack reject those as `match offset beyond LZX stream`. Building
the same full-window dictionary fixed it.

### Verification

Against a reference binary built from the last full-ldic commit:

```
basefile dump, all 35 XEXs in the XDK recovery : identical
GTA IV title update applied to its base xex    : identical
                                                 sha256 8268FDC9...
```

30 of those 35 are compressed and the update is delta-compressed, so both the
plain and delta paths are genuinely exercised.

## Layout

```
src/                    XexTool sources
tools/                  helper scripts
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
