# XexTool

<p align="center"><b>Inspect, extract and patch Xbox 360 XEX executables</b></p>

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
```

| option | |
|---|---|
| `-l` | print extended info about the xex |
| `-p <xexp>` | patch the xex with a title update |
| `-b <file>` | dump the basefile |
| `-i <file>` | dump basefile info to an IDC script |
| `-d <dir>` | dump all resources to a directory |
| `-o <xex>` | write the result to a new file rather than altering the input |
| `-a <path>` | add a bounding path |
| `-u` | fix a patched xex so it no longer needs the separate patch file |
| `-s <flags>` | apply title-specific patches; `0` lists what is available |
| `-r <flags>` | remove limitations -- media, region, region locks, console id, dates and others; `a` removes all |
| `-m d\|r` | force devkit or retail |
| `-c u\|c\|b` | force uncompressed, compressed or binary |
| `-e u\|e` | force unencrypted or encrypted |
| `-x <flags>` | extract metadata as XML -- title, title id, icon, media id, regions and more |
| `-z g\|s <file>` | get or set xex info |

Options combine, for example `-m d -r mrl`. With no options a short info list is
printed. Without `-o` the input file is modified in place.

```
XexTool -l default.xex
XexTool -b basefile.bin default.xex
XexTool -p update.xexp -o patched.xex default.xex
XexTool -r a -o unlocked.xex default.xex
```

## Building

```
git clone --recurse-submodules https://github.com/Team-Resurgent/XexTool.git
```

Open `msvc/XexTool.sln` in Visual Studio 2022 or later, or build from a command
prompt:

```
msbuild msvc/XexTool.sln /p:Configuration=Release /p:Platform=x64
```

Binaries are written to `msvc/build/<platform>/<configuration>/`.

## Dependencies

| | |
|---|---|
| [XeCrypt](https://github.com/Team-Resurgent/XeCrypt) | AES, SHA, HMAC and RSA |
| [libLZX](https://github.com/Team-Resurgent/libLZX) | LZX compression and decompression |
| [tinyxml2](https://github.com/leethomason/tinyxml2) | XML output |

All three are submodules, so remember `--recurse-submodules` when cloning.

## Credits

XexTool is xorloser's work, 2006-2017. XeCrypt, libLZX and tinyxml2 carry their
own licences; see their respective directories.
