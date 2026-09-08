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
| decompress | `XexPacker::unpackCompressed` | **on libmspack** |
| decompress | `XexPacker::unpackDeltaCompressed`, `XexPatcher::XexpDeltaDecompress` | still ldic |
| compress | `XexPacker::packCompressed` | still ldic; nothing to replace it |

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

The two delta paths are **not** converted, and the attempt is worth recording
because the obvious mapping does not work.

`tools/stfs_extract.py` unpacks a title update to get a fixture: a GTA IV
update yields a `default.xexp` that XexTool reports as `Delta Compressed`, and
the base `default.xex` comes out of the disc image with `xdvdfs copy-out`.
Patching with ldic gives a 12857344-byte result, sha256 `8268FDC9...`, so there
is something to compare against.

libmspack does have `lzxd_set_reference_data`, the counterpart of
`LdicSetWindowData`. But it is gated behind the `is_delta` flag, and that flag
is not a neutral switch:

- it restricts the window to 2^17..2^25, while XEX delta blocks use 32KiB (2^15);
- it selects the **LZX DELTA bitstream**, which carries a chunk_size field and
  extended match lengths that plain LZX does not have.

So XEX delta blocks are not LZX DELTA. They are ordinary LZX streams decoded
over a window pre-seeded with the region being patched -- two different things
that libmspack conflates, since the only effect reference data has on decoding
is to widen one match-offset bounds check.

Removing that gate lets the stream initialise, but decoding then fails with
`match offset beyond LZX stream`. The likely cause is header framing: libmspack
reads the one-time "intel filesize" header on each newly created stream, whereas
ldic resets between blocks without re-reading one, so the leading bits are
consumed as a header that is not there and every subsequent offset is wrong.
Confirming that means reading ldic's reset path against libmspack's block
header handling.

Until that is resolved the delta paths stay on ldic, which works.

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
