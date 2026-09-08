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
| ldic | vendored LZX codec (47 files) | **to be replaced** (see below) |

### mbedtls was unused

Nothing in the original included it, and no project file referenced it --
`XexTool.vcxproj` links `XeCrypt`, `ldic` and `tinyxml` only. It accounted for
roughly three quarters of the source tree.

## Still to do

### Replace ldic

`ldic` is an LZX codec with both an encoder and a decoder. XexTool only ever
calls the decoder -- `LdicCreateDecompression`, `LdicSetWindowData`,
`LdicDecompress`, `LdicResetDecompression`, `LdicDestroyDecompression` -- from
`XexPacker.cpp` (`unpackCompressed`, `unpackDeltaCompressed`) and
`XexPatcher.cpp` (`XexpDeltaDecompress`). So the tool as it stands unpacks XEXs
but never creates compressed ones.

That matters for choosing a replacement:

- To preserve **current** behaviour, an LZX **decompressor** is enough.
- To **create** compressed XEXs later, an LZX **compressor** is needed, and
  that is the harder half to find.

The intended replacement was "mspack", but `github.com/fhanau/mspack` is a
mass-spectrometry data compressor, unrelated to Stuart Caie's libmspack -- a
name collision. The correct library still needs to be identified before this
swap can happen.

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
