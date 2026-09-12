// SPDX-License-Identifier: GPL-3.0-or-later
// Part of RXDK-360 - see LICENSE.md for the full GNU GPL v3.
//
// "genstubs" sub-command: generate XEX import thunks for the kernel functions a
// title calls. C++ port of tools/gen_import_stubs.py - reads the undefined
// symbols a title needs, resolves each against the XDK import libraries' short-
// import members (which carry the real console ordinals), and emits a PPC .s
// stub the title links against plus a JSON import manifest for the packer.
#pragma once

int runGenStubsCommand(int argc, char* argv[]);
