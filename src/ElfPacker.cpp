//
// ElfPacker - see ElfPacker.h. Faithful C++ port of tools/elf2xex.py from the
// RXDK-360 toolchain; the Python remains the reference/spec. XEX is big-endian
// throughout; the basefile is a little-endian PPC PE the loader maps as an image.
//

#include "ElfPacker.h"

#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <string>
#include <vector>
#include <algorithm>

typedef std::vector<u8> bytes;

// ---- constants (mirror elf2xex.py) -----------------------------------------

static const u32 PAGE = 0x1000;

static const u32 KEY_BASEFILE_FORMAT      = 0x000003FF;
static const u32 KEY_ENTRY_POINT          = 0x00010100;
static const u32 KEY_IMAGE_BASE_ADDRESS   = 0x00010201;
static const u32 KEY_IMPORT_LIBRARIES     = 0x000103FF;
static const u32 KEY_ORIGINAL_BASE_ADDRESS= 0x00010001;
static const u32 KEY_STACK_SIZE           = 0x00020200;
static const u32 DEFAULT_STACK_SIZE       = 0x40000;
static const u32 MODULEFLAG_TITLE_MODULE  = 0x00000001;

static const u32 SECTIONINFO_CODE     = 1;
static const u32 SECTIONINFO_DATA     = 2;
static const u32 SECTIONINFO_READONLY = 3;

static const u32 SHF_WRITE = 0x1, SHF_ALLOC = 0x2, SHF_EXECINSTR = 0x4;
static const u32 SHT_SYMTAB = 2, SHT_NOBITS = 8;
static const u32 PT_LOAD = 1;

static const u16 PE_MACHINE_POWERPCBE     = 0x01F2;
static const u16 PE_FILE_EXECUTABLE_IMAGE = 0x0002;
static const u16 PE_FILE_32BIT_MACHINE    = 0x0100;
static const u16 PE_OPTIONAL_MAGIC_PE32   = 0x010B;
static const u16 PE_SUBSYSTEM_XBOX        = 14;
static const u16 PE_SIZEOF_OPTIONAL_HEADER= 224;
static const u32 PE_SCN_CODE        = 0x00000020;
static const u32 PE_SCN_INIT_DATA   = 0x00000040;
static const u32 PE_SCN_UNINIT_DATA = 0x00000080;
static const u32 PE_SCN_MEM_EXECUTE = 0x20000000;
static const u32 PE_SCN_MEM_READ    = 0x40000000;
static const u32 PE_SCN_MEM_WRITE   = 0x80000000;
static const u32 FILE_ALIGN = 0x1000;

// ---- byte helpers ----------------------------------------------------------

static void put_le16(bytes& b, u16 v) { b.push_back(u8(v)); b.push_back(u8(v >> 8)); }
static void put_le32(bytes& b, u32 v) { for (int i = 0; i < 4; i++) b.push_back(u8(v >> (8 * i))); }
static void put_be16(bytes& b, u16 v) { b.push_back(u8(v >> 8)); b.push_back(u8(v)); }
static void put_be32(bytes& b, u32 v) { for (int i = 3; i >= 0; i--) b.push_back(u8(v >> (8 * i))); }
static void put_zeros(bytes& b, size_t n) { b.insert(b.end(), n, 0); }

static u16 rd_be16(const u8* p) { return u16((p[0] << 8) | p[1]); }
static u32 rd_be32(const u8* p) { return (u32(p[0]) << 24) | (u32(p[1]) << 16) | (u32(p[2]) << 8) | p[3]; }

static u32 align_up(u32 v, u32 a) { return (v + a - 1) & ~(a - 1); }

static u32 page_size_for(u32 base) { return base < 0x90000000 ? 0x10000 : 0x1000; }

// ---- ELF model -------------------------------------------------------------

struct ElfSection
{
	std::string name;
	u32         vaddr;
	u32         memsize;
	u32         flags;
	u32         typ;
	bytes       data;			// empty for SHT_NOBITS
};

static bool readFile(const std::string& path, bytes& out, std::string& err)
{
	FILE* f = fopen(path.c_str(), "rb");
	if (!f) { err = "cannot open " + path; return false; }
	fseek(f, 0, SEEK_END);
	long n = ftell(f);
	fseek(f, 0, SEEK_SET);
	out.resize(n > 0 ? (size_t)n : 0);
	if (n > 0 && fread(&out[0], 1, (size_t)n, f) != (size_t)n) { fclose(f); err = "read failed: " + path; return false; }
	fclose(f);
	return true;
}

// section header field accessors (ELF32 big-endian, 40-byte shdr)
static u32 shField(const bytes& b, u32 shoff, u32 shentsize, u32 i, int field)
{
	return rd_be32(&b[shoff + i * shentsize + field * 4]);
}

static bool readElf(const bytes& b, u32& outBase, std::vector<ElfSection>& outSecs,
                    u32& outEntry, std::string& err)
{
	if (b.size() < 0x34 || b[0] != 0x7f || b[1] != 'E' || b[2] != 'L' || b[3] != 'F') { err = "not an ELF file"; return false; }
	if (b[4] != 1 || b[5] != 2) { err = "expected a 32-bit big-endian ELF (PPC)"; return false; }

	u32 e_entry = rd_be32(&b[0x18]);
	u32 e_phoff = rd_be32(&b[0x1C]);
	u32 e_shoff = rd_be32(&b[0x20]);
	u16 e_phentsize = rd_be16(&b[0x2A]);
	u16 e_phnum = rd_be16(&b[0x2C]);
	u16 e_shentsize = rd_be16(&b[0x2E]);
	u16 e_shnum = rd_be16(&b[0x30]);
	u16 e_shstrndx = rd_be16(&b[0x32]);

	// image base = lowest PT_LOAD vaddr (what --image-base set)
	bool haveBase = false; u32 imageBase = 0;
	for (u16 i = 0; i < e_phnum; i++) {
		u32 o = e_phoff + i * e_phentsize;
		if (o + 12 > b.size()) break;
		u32 p_type = rd_be32(&b[o]);
		u32 p_vaddr = rd_be32(&b[o + 8]);
		if (p_type == PT_LOAD) { if (!haveBase || p_vaddr < imageBase) { imageBase = p_vaddr; haveBase = true; } }
	}

	if (e_shoff == 0 || e_shstrndx >= e_shnum) { err = "ELF has no section headers"; return false; }
	u32 strtabOff = shField(b, e_shoff, e_shentsize, e_shstrndx, 4);   // sh_offset of shstrtab

	// .eh_frame / .eh_frame_hdr ARE carried across: the C++ exception runtime
	// (libunwind) reads .eh_frame at runtime. The linker script places it on its
	// own page well past the PE headers, so the old tiny-RVA collision is gone.
	static const char* SKIP[] = { ".comment", ".note", ".ARM." };

	for (u16 i = 0; i < e_shnum; i++) {
		u32 nm    = shField(b, e_shoff, e_shentsize, i, 0);
		u32 typ   = shField(b, e_shoff, e_shentsize, i, 1);
		u32 flags = shField(b, e_shoff, e_shentsize, i, 2);
		u32 addr  = shField(b, e_shoff, e_shentsize, i, 3);
		u32 off   = shField(b, e_shoff, e_shentsize, i, 4);
		u32 size  = shField(b, e_shoff, e_shentsize, i, 5);
		if (!(flags & SHF_ALLOC) || size == 0) continue;

		std::string secname;
		for (u32 c = strtabOff + nm; c < b.size() && b[c]; c++) secname.push_back((char)b[c]);

		bool skip = false;
		for (size_t s = 0; s < sizeof(SKIP) / sizeof(SKIP[0]); s++)
			if (secname.compare(0, strlen(SKIP[s]), SKIP[s]) == 0) { skip = true; break; }
		if (skip) continue;

		ElfSection sec;
		sec.name = secname; sec.vaddr = addr; sec.memsize = size; sec.flags = flags; sec.typ = typ;
		if (typ != SHT_NOBITS && off + size <= b.size())
			sec.data.assign(b.begin() + off, b.begin() + off + size);
		outSecs.push_back(sec);
	}
	if (outSecs.empty()) { err = "no allocatable sections"; return false; }
	std::stable_sort(outSecs.begin(), outSecs.end(),
	                 [](const ElfSection& a, const ElfSection& c) { return a.vaddr < c.vaddr; });

	outBase = haveBase ? imageBase : outSecs.front().vaddr;
	outEntry = e_entry;
	return true;
}

// name -> virtual address, from the ELF symtab (defined globals)
static void readSymbolAddrs(const bytes& b, std::vector<std::pair<std::string, u32> >& out)
{
	u32 e_shoff = rd_be32(&b[0x20]);
	u16 e_shentsize = rd_be16(&b[0x2E]);
	u16 e_shnum = rd_be16(&b[0x30]);
	u32 symOff = 0, symSize = 0, symEnt = 0, strOff = 0; bool have = false;
	for (u16 i = 0; i < e_shnum; i++) {
		if (shField(b, e_shoff, e_shentsize, i, 1) == SHT_SYMTAB) {
			symOff = shField(b, e_shoff, e_shentsize, i, 4);
			symSize = shField(b, e_shoff, e_shentsize, i, 5);
			u32 link = shField(b, e_shoff, e_shentsize, i, 6);   // sh_link -> strtab
			symEnt = shField(b, e_shoff, e_shentsize, i, 9);
			strOff = shField(b, e_shoff, e_shentsize, link, 4);
			have = true;
		}
	}
	if (!have || symEnt == 0) return;
	for (u32 k = 0; k < symSize / symEnt; k++) {
		u32 o = symOff + k * symEnt;
		u32 st_name = rd_be32(&b[o]);
		u32 st_value = rd_be32(&b[o + 4]);
		if (st_name && st_value) {
			std::string n;
			for (u32 c = strOff + st_name; c < b.size() && b[c]; c++) n.push_back((char)b[c]);
			out.push_back(std::make_pair(n, st_value));
		}
	}
}

// ---- little-endian PPC PE32 basefile ---------------------------------------

static u32 peSectionFlags(const ElfSection& s)
{
	u32 f = PE_SCN_MEM_READ;
	if (s.flags & SHF_EXECINSTR) f |= PE_SCN_CODE | PE_SCN_MEM_EXECUTE;
	else if (s.typ == SHT_NOBITS) f |= PE_SCN_UNINIT_DATA;
	else f |= PE_SCN_INIT_DATA;
	if (s.flags & SHF_WRITE) f |= PE_SCN_MEM_WRITE;
	return f;
}

static bool buildPeBasefile(u32 base, const std::vector<ElfSection>& secs, u32 entry,
                            bytes& outImage, u32& outImageSize, std::string& err)
{
	u32 e_lfanew = 0x40;
	u32 nsec = (u32)secs.size();
	u32 headers_end = e_lfanew + 4 + 20 + PE_SIZEOF_OPTIONAL_HEADER + nsec * 40;
	u32 size_of_headers = align_up(headers_end, FILE_ALIGN);

	u32 page = page_size_for(base);
	u32 image_end = base;
	for (size_t i = 0; i < secs.size(); i++) image_end = std::max(image_end, secs[i].vaddr + secs[i].memsize);
	u32 size_of_image = align_up(image_end - base, page);

	u32 code_rva = size_of_headers, size_of_code = 0, size_of_data = 0;
	bool haveCode = false;
	for (size_t i = 0; i < secs.size(); i++) {
		if (secs[i].flags & SHF_EXECINSTR) { if (!haveCode) { code_rva = secs[i].vaddr - base; haveCode = true; } size_of_code += (u32)secs[i].data.size(); }
		else size_of_data += (u32)secs[i].data.size();
	}

	// a kept section starting inside the PE header region cannot be represented
	for (size_t i = 0; i < secs.size(); i++) {
		if ((secs[i].vaddr - base) < size_of_headers) {
			char buf[256];
			snprintf(buf, sizeof(buf), "section %s at RVA 0x%X overlaps the PE headers (0x%X); link with headroom below the first section",
			         secs[i].name.c_str(), secs[i].vaddr - base, size_of_headers);
			err = buf; return false;
		}
	}

	// DOS header: first dword must be 0x905A4D ("MZ\x90\x00")
	bytes dos(e_lfanew, 0);
	dos[0] = 'M'; dos[1] = 'Z'; dos[2] = 0x90; dos[3] = 0x00;
	dos[0x3C] = u8(e_lfanew); dos[0x3D] = u8(e_lfanew >> 8); dos[0x3E] = u8(e_lfanew >> 16); dos[0x3F] = u8(e_lfanew >> 24);

	bytes file_hdr;
	put_le32(file_hdr, 0x00004550);                          // "PE\0\0"
	put_le16(file_hdr, PE_MACHINE_POWERPCBE);
	put_le16(file_hdr, (u16)nsec);
	put_le32(file_hdr, 0); put_le32(file_hdr, 0); put_le32(file_hdr, 0);
	put_le16(file_hdr, PE_SIZEOF_OPTIONAL_HEADER);
	put_le16(file_hdr, PE_FILE_EXECUTABLE_IMAGE | PE_FILE_32BIT_MACHINE);

	bytes opt;
	put_le16(opt, PE_OPTIONAL_MAGIC_PE32); opt.push_back(0); opt.push_back(0);
	put_le32(opt, size_of_code); put_le32(opt, size_of_data); put_le32(opt, 0);
	put_le32(opt, entry - base); put_le32(opt, code_rva); put_le32(opt, 0);   // BaseOfCode, BaseOfData
	put_le32(opt, base);                                     // ImageBase
	put_le32(opt, page); put_le32(opt, FILE_ALIGN);          // Section/File alignment
	put_le16(opt, 4); put_le16(opt, 0); put_le16(opt, 0); put_le16(opt, 0); put_le16(opt, 0); put_le16(opt, 0);
	put_le32(opt, 0);                                        // Win32VersionValue
	put_le32(opt, size_of_image); put_le32(opt, size_of_headers);
	put_le32(opt, 0);                                        // CheckSum
	put_le16(opt, PE_SUBSYSTEM_XBOX); put_le16(opt, 0);
	put_le32(opt, 0x40000); put_le32(opt, 0x1000); put_le32(opt, 0x100000); put_le32(opt, 0x1000);
	put_le32(opt, 0); put_le32(opt, 16);                     // LoaderFlags, NumberOfRvaAndSizes
	put_zeros(opt, 16 * 8);                                  // data directories
	if (opt.size() != PE_SIZEOF_OPTIONAL_HEADER) { err = "internal: optional header size"; return false; }

	bytes sec_hdrs;
	for (size_t i = 0; i < secs.size(); i++) {
		const ElfSection& s = secs[i];
		char nm[8]; memset(nm, 0, 8); memcpy(nm, s.name.c_str(), std::min<size_t>(8, s.name.size()));
		u32 rva = s.vaddr - base;
		u32 raw_size = align_up((u32)s.data.size(), FILE_ALIGN);
		sec_hdrs.insert(sec_hdrs.end(), nm, nm + 8);
		put_le32(sec_hdrs, s.memsize); put_le32(sec_hdrs, rva);
		put_le32(sec_hdrs, raw_size); put_le32(sec_hdrs, rva);      // SizeOfRawData, PointerToRawData=RVA
		put_le32(sec_hdrs, 0); put_le32(sec_hdrs, 0);
		put_le16(sec_hdrs, 0); put_le16(sec_hdrs, 0);
		put_le32(sec_hdrs, peSectionFlags(s));
	}

	outImage.assign(size_of_image, 0);
	memcpy(&outImage[0], &dos[0], dos.size());
	u32 o = e_lfanew;
	memcpy(&outImage[o], &file_hdr[0], file_hdr.size()); o += (u32)file_hdr.size();
	memcpy(&outImage[o], &opt[0], opt.size()); o += (u32)opt.size();
	if (!sec_hdrs.empty()) memcpy(&outImage[o], &sec_hdrs[0], sec_hdrs.size());
	for (size_t i = 0; i < secs.size(); i++) {
		u32 rva = secs[i].vaddr - base;
		if (!secs[i].data.empty()) memcpy(&outImage[rva], &secs[i].data[0], secs[i].data.size());
	}
	outImageSize = size_of_image;
	return true;
}

// ---- XEX2 structures -------------------------------------------------------

static bytes buildBasefileFormat(u32 imageSize, u32 zeroSize)
{
	bytes b;
	put_be32(b, 8 + 8);            // infoSize
	put_be16(b, 0);               // encType (unencrypted)
	put_be16(b, 1);               // compType (uncompressed)
	put_be32(b, imageSize); put_be32(b, zeroSize);
	return b;
}

// each descriptor is (page_count, info); keep >=1 CODE page for xenia's hash
static void buildPageDescriptors(u32 base, const std::vector<ElfSection>& secs, u32 imageSize,
                                 u32 pageSize, std::vector<std::pair<u32, u32> >& out,
                                 std::vector<std::string>& notes)
{
	u32 numPages = imageSize / pageSize;
	std::vector<u32> kinds;
	bool sharedCodeWrite = false;
	for (u32 p = 0; p < numPages; p++) {
		u32 lo = p * pageSize, hi = lo + pageSize;
		bool hasExec = false, hasWrite = false;
		for (size_t i = 0; i < secs.size(); i++) {
			u32 sLo = secs[i].vaddr - base, sHi = sLo + secs[i].memsize;
			if (sHi <= lo || sLo >= hi) continue;
			if (secs[i].flags & SHF_EXECINSTR) hasExec = true;
			else if (secs[i].flags & SHF_WRITE) hasWrite = true;
		}
		if (hasWrite && hasExec) sharedCodeWrite = true;
		u32 info = hasWrite ? SECTIONINFO_DATA : (hasExec ? SECTIONINFO_CODE : SECTIONINFO_READONLY);
		kinds.push_back(info);
	}
	bool forced = false, haveCode = false;
	for (size_t i = 0; i < kinds.size(); i++) if (kinds[i] == SECTIONINFO_CODE) haveCode = true;
	if (!haveCode && !kinds.empty()) { kinds[0] = SECTIONINFO_CODE; forced = true; }

	for (size_t i = 0; i < kinds.size(); i++) {
		if (!out.empty() && out.back().second == kinds[i]) out.back().first++;
		else out.push_back(std::make_pair(1u, kinds[i]));
	}
	if (sharedCodeWrite)
		notes.push_back("a writable section shares a page with code; align the writable region (.data/.bss) to the page size to map it read-write");
	if (forced)
		notes.push_back("no page held only code, so page 0 was forced CODE for the code hash; writable data on that page stays read-only");
}

static bytes buildSecurityInfo(u32 imageSize, u32 loadAddress,
                               const std::vector<std::pair<u32, u32> >& sections)
{
	bytes imageInfo;
	put_zeros(imageInfo, 256);            // signature
	put_be32(imageInfo, 0x174);           // infoSize
	put_be32(imageInfo, 0);               // imageFlags
	put_be32(imageInfo, loadAddress);
	put_zeros(imageInfo, 20);             // imageHash
	put_be32(imageInfo, 0);               // importTableCount
	put_zeros(imageInfo, 20);             // importHash
	put_zeros(imageInfo, 16);             // mediaId
	put_zeros(imageInfo, 16);             // imageKey
	put_be32(imageInfo, 0);               // exportTableAddress
	put_zeros(imageInfo, 20);             // headerHash
	put_be32(imageInfo, 0xFFFFFFFF);      // gameRegion

	bytes secBytes;
	for (size_t i = 0; i < sections.size(); i++) {
		put_be32(secBytes, (sections[i].first << 4) | (sections[i].second & 0xF));
		put_zeros(secBytes, 20);
	}

	u32 total = 0x184 + (u32)secBytes.size();
	bytes out;
	put_be32(out, total); put_be32(out, imageSize);
	out.insert(out.end(), imageInfo.begin(), imageInfo.end());
	put_be32(out, 0xFFFFFFFF);            // allowedMediaTypes
	put_be32(out, (u32)sections.size());  // sectionCount
	out.insert(out.end(), secBytes.begin(), secBytes.end());
	return out;
}

// imports: list of (module name, [record virtual address, ...])
static bytes buildImportLibraries(const std::vector<std::pair<std::string, std::vector<u32> > >& imports)
{
	bytes nameData;
	std::vector<std::string> names;
	for (size_t i = 0; i < imports.size(); i++) {
		names.push_back(imports[i].first);
		const std::string& n = imports[i].first;
		nameData.insert(nameData.end(), n.begin(), n.end());
		nameData.push_back(0);
		while (nameData.size() % 4) nameData.push_back(0);
	}
	bytes libs;
	for (size_t i = 0; i < imports.size(); i++) {
		const std::vector<u32>& records = imports[i].second;
		u32 count = (u32)records.size();
		put_be32(libs, 0x28 + count * 4);   // size
		put_zeros(libs, 0x14);              // next_import_digest
		put_be32(libs, 0);                  // id
		put_be32(libs, 0);                  // version_value
		put_be32(libs, 0);                  // version_min_value
		put_be16(libs, (u16)i);             // name_index
		put_be16(libs, (u16)count);         // count
		for (u32 r = 0; r < count; r++) put_be32(libs, records[r]);
	}
	u32 total = 12 + (u32)nameData.size() + (u32)libs.size();
	bytes out;
	put_be32(out, total); put_be32(out, (u32)nameData.size()); put_be32(out, (u32)names.size());
	out.insert(out.end(), nameData.begin(), nameData.end());
	out.insert(out.end(), libs.begin(), libs.end());
	return out;
}

// ---- minimal import-manifest JSON reader -----------------------------------
// Parses {"libraries":[{"module":"m","records":["a","b",...]},...]} as emitted
// by the RXDK-360 gen_import_stubs.py. Tolerant of whitespace; not a general
// JSON parser -- it only understands this shape.

static bool nextQuoted(const std::string& s, size_t& pos, std::string& out)
{
	size_t q = s.find('"', pos);
	if (q == std::string::npos) return false;
	size_t e = s.find('"', q + 1);
	if (e == std::string::npos) return false;
	out = s.substr(q + 1, e - q - 1);
	pos = e + 1;
	return true;
}

static bool parseManifest(const std::string& path,
                          std::vector<std::pair<std::string, std::vector<std::string> > >& out,
                          std::string& err)
{
	bytes raw; if (!readFile(path, raw, err)) return false;
	std::string s(raw.begin(), raw.end());
	size_t pos = 0;
	while (true) {
		size_t m = s.find("\"module\"", pos);
		if (m == std::string::npos) break;
		size_t colon = s.find(':', m + 8);
		if (colon == std::string::npos) { err = "manifest: malformed module"; return false; }
		size_t p = colon;
		std::string module;
		if (!nextQuoted(s, p, module)) { err = "manifest: malformed module name"; return false; }

		size_t r = s.find("\"records\"", p);
		if (r == std::string::npos) { err = "manifest: library has no records"; return false; }
		size_t lb = s.find('[', r), rb = s.find(']', lb == std::string::npos ? r : lb);
		if (lb == std::string::npos || rb == std::string::npos) { err = "manifest: malformed records array"; return false; }
		std::vector<std::string> records;
		size_t q = lb + 1;
		std::string rec;
		while (q < rb && nextQuoted(s.substr(0, rb), q, rec)) records.push_back(rec);
		out.push_back(std::make_pair(module, records));
		pos = rb + 1;
	}
	if (out.empty()) { err = "manifest: no libraries found in " + path; return false; }
	return true;
}

// ---- pack ------------------------------------------------------------------

bool packElfToXex(const std::string& elfPath, const std::string& outPath,
                  const ElfPackOptions& opt, std::string& err)
{
	bytes blob;
	if (!readFile(elfPath, blob, err)) return false;

	u32 loadBase, entry;
	std::vector<ElfSection> secs;
	if (!readElf(blob, loadBase, secs, entry, err)) return false;

	if (opt.hasBase && opt.base != loadBase) {
		char buf[160];
		snprintf(buf, sizeof(buf), "ELF is linked at 0x%08X, not 0x%08X; link it at the target base instead of overriding here", loadBase, opt.base);
		err = buf; return false;
	}

	// resolve import records (symbol name -> address) if a manifest was given
	bytes importBlock; bool haveImports = false;
	if (!opt.importManifest.empty()) {
		std::vector<std::pair<std::string, std::vector<std::string> > > libs;
		if (!parseManifest(opt.importManifest, libs, err)) return false;
		std::vector<std::pair<std::string, u32> > syms;
		readSymbolAddrs(blob, syms);
		std::vector<std::pair<std::string, std::vector<u32> > > resolved;
		for (size_t i = 0; i < libs.size(); i++) {
			std::vector<u32> vas;
			for (size_t j = 0; j < libs[i].second.size(); j++) {
				const std::string& want = libs[i].second[j];
				bool found = false;
				for (size_t k = 0; k < syms.size(); k++) if (syms[k].first == want) { vas.push_back(syms[k].second); found = true; break; }
				if (!found) { err = "import symbol '" + want + "' not found in " + elfPath; return false; }
			}
			resolved.push_back(std::make_pair(libs[i].first, vas));
		}
		importBlock = buildImportLibraries(resolved);
		haveImports = true;
	}

	bytes image; u32 imageSize;
	if (!buildPeBasefile(loadBase, secs, entry, image, imageSize, err)) return false;

	u32 pages = imageSize / page_size_for(loadBase);
	bytes basefileFormat = buildBasefileFormat((u32)image.size(), 0);

	std::vector<std::pair<u32, u32> > descriptors;
	std::vector<std::string> notes;
	buildPageDescriptors(loadBase, secs, imageSize, page_size_for(loadBase), descriptors, notes);
	u32 descPages = 0; for (size_t i = 0; i < descriptors.size(); i++) descPages += descriptors[i].first;
	if (descPages != pages) { err = "internal: page descriptor count mismatch"; return false; }
	bytes security = buildSecurityInfo(imageSize, loadBase, descriptors);

	// optional-header directory: inline-value keys and offset blocks
	struct Inline { u32 key, val; };
	Inline inlines[4] = {
		{ KEY_ENTRY_POINT, entry },
		{ KEY_IMAGE_BASE_ADDRESS, loadBase },
		{ KEY_ORIGINAL_BASE_ADDRESS, loadBase },
		{ KEY_STACK_SIZE, DEFAULT_STACK_SIZE },
	};
	struct OffBlock { u32 key; const bytes* data; };
	std::vector<OffBlock> offBlocks;
	OffBlock ob0; ob0.key = KEY_BASEFILE_FORMAT; ob0.data = &basefileFormat; offBlocks.push_back(ob0);
	if (haveImports) { OffBlock ob1; ob1.key = KEY_IMPORT_LIBRARIES; ob1.data = &importBlock; offBlocks.push_back(ob1); }

	u32 nEntries = 4 + (u32)offBlocks.size();
	u32 headerSize = 0x18 + nEntries * 8;
	u32 secOff = headerSize;
	u32 cur = secOff + (u32)security.size();
	std::vector<u32> blockOffsets(offBlocks.size());
	for (size_t i = 0; i < offBlocks.size(); i++) { blockOffsets[i] = cur; cur += (u32)offBlocks[i].data->size(); }
	u32 basefileOff = align_up(cur, PAGE);

	bytes out;
	// image header (XexImageHeader)
	out.push_back('X'); out.push_back('E'); out.push_back('X'); out.push_back('2');
	put_be32(out, MODULEFLAG_TITLE_MODULE);
	put_be32(out, basefileOff);           // sizeOfHeaders
	put_be32(out, 0);                     // sizeOfDiscardableHeaders
	put_be32(out, secOff);                // securityInfoOffset
	put_be32(out, nEntries);
	// directory
	for (int i = 0; i < 4; i++) { put_be32(out, inlines[i].key); put_be32(out, inlines[i].val); }
	for (size_t i = 0; i < offBlocks.size(); i++) { put_be32(out, offBlocks[i].key); put_be32(out, blockOffsets[i]); }
	if (out.size() != secOff) { err = "internal: header size"; return false; }
	out.insert(out.end(), security.begin(), security.end());
	for (size_t i = 0; i < offBlocks.size(); i++) out.insert(out.end(), offBlocks[i].data->begin(), offBlocks[i].data->end());
	put_zeros(out, basefileOff - (u32)out.size());
	out.insert(out.end(), image.begin(), image.end());

	FILE* f = fopen(outPath.c_str(), "wb");
	if (!f) { err = "cannot write " + outPath; return false; }
	if (!out.empty()) fwrite(&out[0], 1, out.size(), f);
	fclose(f);

	printf("wrote %s: base 0x%08X entry 0x%08X image %u bytes (%u pages), file %u bytes\n",
	       outPath.c_str(), loadBase, entry, (u32)image.size(), pages, (u32)out.size());
	printf("  pages:");
	for (size_t i = 0; i < descriptors.size(); i++) {
		const char* n = descriptors[i].second == SECTIONINFO_CODE ? "CODE" :
		                descriptors[i].second == SECTIONINFO_DATA ? "RWDATA" : "RODATA";
		printf("%s %ux%s", i ? "," : "", descriptors[i].first, n);
	}
	printf("\n");
	for (size_t i = 0; i < notes.size(); i++) printf("  note: %s\n", notes[i].c_str());
	return true;
}

// ---- "pack" sub-command ----------------------------------------------------

static const char PACK_USAGE[] =
	"Usage:    XexTool pack <input.elf> -o <output.xex> [options]\n"
	"\n"
	"Wrap a linked PPC32 ELF executable in an uncompressed, unencrypted devkit\n"
	"XEX2 (the form a debug kit loads without a signature).\n"
	"\n"
	"Options:\n"
	"          -o, --out <file>          output xex (required)\n"
	"          --base <addr>             load base, e.g. 0x82000000 (default: from the ELF)\n"
	"          --import-manifest <file>  JSON import manifest (module -> records),\n"
	"                                    as produced by gen_import_stubs.py / mktitle.py\n"
	"          -h, --help                show this help\n";

int runPackCommand(int argc, char* argv[])
{
	std::string input, out, manifest;
	ElfPackOptions opt;

	for (int i = 2; i < argc; i++) {
		std::string a = argv[i];
		if (a == "-h" || a == "--help") { printf("%s", PACK_USAGE); return 0; }
		else if (a == "-o" || a == "--out") { if (i + 1 >= argc) { printf("%s", PACK_USAGE); return 1; } out = argv[++i]; }
		else if (a == "--base") { if (i + 1 >= argc) { printf("%s", PACK_USAGE); return 1; } opt.base = (u32)strtoul(argv[++i], NULL, 0); opt.hasBase = true; }
		else if (a == "--import-manifest") { if (i + 1 >= argc) { printf("%s", PACK_USAGE); return 1; } manifest = argv[++i]; }
		else if (!a.empty() && a[0] == '-') { printf("Unknown option: %s\n\n%s", a.c_str(), PACK_USAGE); return 1; }
		else if (input.empty()) input = a;
		else { printf("Unexpected argument: %s\n\n%s", a.c_str(), PACK_USAGE); return 1; }
	}

	if (input.empty() || out.empty()) {
		if (input.empty()) printf("error: no input ELF given\n\n");
		else printf("error: no output given (use -o)\n\n");
		printf("%s", PACK_USAGE);
		return 1;
	}

	opt.importManifest = manifest;
	std::string err;
	if (!packElfToXex(input, out, opt, err)) { printf("pack failed: %s\n", err.c_str()); return 1; }
	return 0;
}
