# XexTool

<p align="center"><b>Inspect, extract, patch and pack Xbox 360 XEX executables</b></p>

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

## Usage

```
XexTool <options> <xex filename>
XexTool pack <input.elf> -o <output.xex> [options]
XexTool genstubs --xdk <lib dir> --names <a,b,..> -o <stubs.s> [--manifest <json>]
XexTool applyxml <input.xex> --xml <file.xml> [-o <output.xex>]
```

Run `XexTool` with no arguments for the same list the binary prints. Options
combine, for example `-m d -r mrl`. Without `-o` the input file is modified in
place.

### Inspect / patch a xex

| option | |
|---|---|
| `-l` | print extended info |
| `-p <xexp>` | patch with a title update |
| `-b <file>` | dump the basefile |
| `-i <file>` | dump basefile info to an IDC script |
| `-d <dir>` | dump all resources (`.` is allowed) |
| `-o <xex>` | write a new file instead of altering the input |
| `-a <path>` | add a bounding path |
| `-u` | fix a patched xex so it no longer needs the separate patch file |
| `-s <flags>` | title-specific patches (bitflags; `0` lists them, `-1` does all) |
| `-r <flags>` | remove limitations (see below) |
| `-m d\|r` | force devkit or retail (`0`=`d`, `1`=`r`) |
| `-c u\|c\|b` | force uncompressed, compressed or binary (`0`=`u`, `1`=`c`) |
| `-e u\|e` | force unencrypted or encrypted (`0`=`u`, `1`=`e`) |
| `-x <flags>` | extract metadata as XML (see below) |
| `-z g\|s <file>` | get or set xex info |

`-r` letters: `a` all (`mrbdiyvklcz`), `m` media, `r` region, `b` bounding path,
`d` bounding device id, `i` console id, `y` dates, `v` keyvault privileges,
`k` signed keyvault only, `l` minimum library versions, `c` revocation check,
`s` disc-swap checks, `z` zero the media id.

`-x` letters: `a` everything, `b` basefile type, `d` media id, `i` icon,
`m` media, `n` name, `p` bounding path, `r` regions, `t` title id,
`x` machine format (retail/devkit).

```
XexTool -l default.xex
XexTool -b basefile.bin default.xex
XexTool -p update.xexp -o patched.xex default.xex
XexTool -r a -o unlocked.xex default.xex
XexTool -m d -c u -e u -o devkit.xex default.xex
XexTool -x a default.xex
```

### Pack a modern (clang) title

These subcommands sit in front of the option parser. RXDK-360's clang toolset
uses them instead of imagexex: `genstubs` then a final link, `pack` the ELF,
`applyxml` for Image Conversion metadata, then `-m d` to sign as a devkit xex.

| command | |
|---|---|
| `pack <elf> -o <xex>` | wrap a linked PPC32 ELF in an uncompressed, unencrypted XEX2 |
| `pack --base <addr>` | load address (default: from the ELF, e.g. `0x82000000`) |
| `pack --import-manifest <json>` | import records from `genstubs` |
| `genstubs --xdk <lib dir>` | XDK `lib` dir with short-import `.lib` members |
| `genstubs --names A,B,...` | kernel symbols the title needs |
| `genstubs -o <stubs.s>` | PPC import thunks to assemble and link |
| `genstubs --manifest <json>` | JSON for `pack --import-manifest` |
| `applyxml <xex> --xml <file>` | overlay imagexex-style `<xex>` XML (title id, privileges, LAN key, heap/workspace) |
| `applyxml -o <xex>` | write a new file; omitted overwrites the input |

```
XexTool pack title.elf -o unsigned.xex --base 0x82000000 --import-manifest stubs.imports.json
XexTool genstubs --xdk "C:\Xbox\lib" --names DbgPrint,KeBugCheck -o stubs.s --manifest stubs.imports.json
XexTool applyxml unsigned.xex --xml title.xex.xml -o unsigned.xex
XexTool -m d -o title.xex unsigned.xex
```

```
XexTool pack --help
XexTool genstubs --help
XexTool applyxml --help
```

## Building

```
git clone --recurse-submodules https://github.com/Team-Resurgent/XexTool.git
```

Windows, Linux and macOS, x64 and arm64:

```
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
ctest --test-dir build --build-config Release --output-on-failure
```

On Windows you can instead open `msvc/XexTool.sln` in Visual Studio 2022 or
later, which writes to `msvc/build/<platform>/<configuration>/`. CMake and VS
output dirs (`build/`, `msvc/build/`, `build-ref/`) are gitignored.

CI runs the same six-way matrix (Windows/Linux/macOS × x64/arm64).

## Dependencies

| | |
|---|---|
| [XeCrypt](https://github.com/Team-Resurgent/XeCrypt) | AES, SHA, HMAC and RSA, by cOz |
| [libLZX](https://github.com/Team-Resurgent/libLZX) | LZX compression and decompression |
| [tinyxml2](https://github.com/leethomason/tinyxml2) | XML output |

All three are submodules, so remember `--recurse-submodules` when cloning.

## Credits

XexTool is xorloser's work, 2006-2017. XeCrypt by cOz, libLZX and
tinyxml2 carry their own licences; see their respective directories.
