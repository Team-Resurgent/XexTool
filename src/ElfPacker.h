//
// ElfPacker - wrap a linked PPC32 ELF executable in a XEX2 container.
//
// The XexTool-side port of the RXDK-360 elf2xex.py reference packer: it takes
// an ELF the linker produced at a XEX load address and emits an uncompressed,
// unencrypted devkit XEX (the form a debug kit loads without a signature).
//

#ifndef _ELF_PACKER_H_
#define _ELF_PACKER_H_

#include <string>
#include "types.h"

struct ElfPackOptions
{
	bool        hasBase;			// override the ELF's load base
	u32         base;
	std::string importManifest;		// JSON manifest path, or empty for no imports

	ElfPackOptions() : hasBase(false), base(0) {}
};

// Pack elfPath into outPath. Prints a short summary on success. On failure
// returns false and fills err.
bool packElfToXex(const std::string& elfPath, const std::string& outPath,
                  const ElfPackOptions& opt, std::string& err);

// "pack" sub-command entry point: parses its own argv (with --help) and runs.
int runPackCommand(int argc, char* argv[]);

#endif // _ELF_PACKER_H_
