//
// applyxml: overlay an imagexex-style <xex> XML onto a packed XEX.
//
// Observed from imagexex.exe on a legacy PE: /titleid, /lankey, /xapiheap,
// /workspace, /privilege:N, /exportnames, /section:NAME=file, and /config:<xex>
// XML write the same optional headers. Privilege N is bit N of
// IMAGEKEY_SYSTEM_FLAGS (N<32) or SYSTEM_FLAGS2 (N>=32). Privilege 2
// (No ODD Mapping / Optical Disc Drive Mapping) is incompatible with
// DVD/CD, DVD-X2, and XGD2 media (imagexex IM1069); applyxml drops those
// bits so the XEX matches a pack that imagexex accepts. <mediatypes>
// children use the official imagexex names (harddisk, cd, network, …).
// <section> blobs are stuffed into leftover PE virtual space (0x80-aligned
// after the last section, same VA imagexex /section: uses) and registered
// as XEX resources. Overflow grows 64KB Header/Resource pages. Matches
// MultiDisc's <section file= name= memory="RO_ENCRYPTED"/>.
//

#include "ElfPacker.h"
#include "PEParser.h"
#include "Xex.h"
#include "XexReader.h"
#include "XexWriter.h"
#include "tinyxml2.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

static const char APPLYXML_USAGE[] =
	"Usage:    XexTool applyxml <input.xex> --xml <file.xml> [-o <output.xex>]\n"
	"\n"
	"Apply an imagexex-style <xex> XML onto an existing XEX (title id, privileges,\n"
	"LAN key, heap/workspace sizes, export-by-name, additional sections).\n"
	"Unspecified fields are left alone. If -o is omitted the input file is overwritten.\n";

static int hexByte(char c)
{
	if (c >= '0' && c <= '9') return c - '0';
	if (c >= 'a' && c <= 'f') return c - 'a' + 10;
	if (c >= 'A' && c <= 'F') return c - 'A' + 10;
	return -1;
}

static bool parseHexBytes(const char* s, u8* out, int want)
{
	if (!s) return false;
	while (*s == ' ' || *s == '\t') s++;
	if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) s += 2;
	int n = 0;
	while (*s) {
		if (*s == ' ' || *s == '\t') { s++; continue; }
		int hi = hexByte(*s++);
		if (hi < 0 || !*s) return false;
		int lo = hexByte(*s++);
		if (lo < 0) return false;
		if (n >= want) return false;
		out[n++] = (u8)((hi << 4) | lo);
	}
	return n == want;
}

static const char* elemAttrOrText(tinyxml2::XMLElement* e, const char* a1, const char* a2 = NULL)
{
	const char* v = e->Attribute(a1);
	if (v && *v) return v;
	if (a2) {
		v = e->Attribute(a2);
		if (v && *v) return v;
	}
	v = e->GetText();
	return v;
}

static void applyPrivilege(Xex& xex, u32 id)
{
	if (id < 32)
		xex.setSystemFlags(xex.getSystemFlags() | (1u << id));
	else if (id < 64)
		xex.setSystemFlags2(xex.getSystemFlags2() | (1u << (id - 32)));
	else
		printf("applyxml: ignoring privilege id %u (out of range)\n", id);
}

// Observed from imagexex /config XML: <cd/>, <dvdx2/>, and <dvdxgd2/> with
// privilege 2 return IM1069. Other default types (harddisk, network, ram,
// livepackage, svod, …) are fine. Strip after the whole XML so a later
// <mediatypes> cannot put optical bits back.
static void stripOddIncompatibleMedia(Xex& xex)
{
	if (!xex.isNoODDMapping())
		return;
	u32 media = xex.getMediaTypes();
	u32 drop = MEDIATYPE_DVD_CD | MEDIATYPE_DVDX2;
	if (media & drop) {
		xex.setMediaTypes(media & ~drop);
		printf("applyxml: privilege 2: dropped DVD/CD media (imagexex IM1069)\n");
	}
	if (xex.isXGD2Only()) {
		xex.setXGD2Only(false);
		printf("applyxml: privilege 2: cleared XGD2-only (imagexex IM1069)\n");
	}
}

// Official imagexex <mediatypes> replaces allowed media. Child names taken
// from imagexex.exe and ArcadeSample xex-dev.xml; flags= is XexTool -x info.
static bool applyMediaTypes(Xex& xex, tinyxml2::XMLElement* e)
{
	const char* flagsAttr = e->Attribute("flags");
	if (flagsAttr && *flagsAttr) {
		xex.setMediaTypes((u32)strtoul(flagsAttr, NULL, 0));
		return true;
	}

	u32 flags = 0;
	bool xgd2 = false;
	for (tinyxml2::XMLElement* c = e->FirstChildElement(); c; c = c->NextSiblingElement()) {
		const char* n = c->Name();
		if (!n) continue;
		if (STRICMP(n, "harddisk") == 0) flags |= MEDIATYPE_HARD_DISK;
		else if (STRICMP(n, "cd") == 0) flags |= MEDIATYPE_DVD_CD;
		else if (STRICMP(n, "dvdx2") == 0) flags |= MEDIATYPE_DVDX2;
		else if (STRICMP(n, "dvdxgd2") == 0) { flags |= MEDIATYPE_DVD_CD; xgd2 = true; }
		else if (STRICMP(n, "flash") == 0) flags |= MEDIATYPE_SYS_FLASH;
		else if (STRICMP(n, "mu") == 0) flags |= MEDIATYPE_MEM_UNIT;
		else if (STRICMP(n, "usbmass") == 0) flags |= MEDIATYPE_MASS_STORAGE;
		else if (STRICMP(n, "network") == 0) flags |= MEDIATYPE_SMB;
		else if (STRICMP(n, "ram") == 0) flags |= MEDIATYPE_RAM;
		else if (STRICMP(n, "ramdrive") == 0) flags |= MEDIATYPE_RAM_DRIVE;
		else if (STRICMP(n, "svod") == 0) flags |= MEDIATYPE_SECURE_VIRT_OD;
		else if (STRICMP(n, "livepackage") == 0) flags |= MEDIATYPE_LIVESIGN_PKG;
		else if (STRICMP(n, "signedpackage") == 0) flags |= MEDIATYPE_XBOX_PKG;
		else if (STRICMP(n, "localpackage") == 0) flags |= MEDIATYPE_LOCALSIGN_PKG;
		else if (STRICMP(n, "consolepackage") == 0) flags |= MEDIATYPE_SAVEGAME_PKG;
		else if (STRICMP(n, "allpackages") == 0)
			flags |= MEDIATYPE_INSECURE_PKG | MEDIATYPE_SAVEGAME_PKG |
			         MEDIATYPE_LOCALSIGN_PKG | MEDIATYPE_LIVESIGN_PKG |
			         MEDIATYPE_XBOX_PKG | 0xE0000000u;
		else
			printf("applyxml: ignoring unknown mediatype <%s>\n", n);
	}
	xex.setMediaTypes(flags);
	xex.setXGD2Only(xgd2);
	return true;
}

static std::string dirnameOf(const char* path)
{
	std::string p = path ? path : "";
	size_t slash = p.find_last_of("\\/");
	if (slash == std::string::npos) return ".";
	return p.substr(0, slash);
}

static std::string joinPath(const std::string& dir, const char* file)
{
	if (!file || !*file) return "";
	if ((file[0] == '/' || file[0] == '\\') ||
	    (file[0] && file[1] == ':'))
		return file;
	if (dir.empty() || dir == ".") return file;
	char sep = '\\';
	if (dir.find('/') != std::string::npos && dir.find('\\') == std::string::npos)
		sep = '/';
	return dir + sep + file;
}

static u32 alignUp(u32 v, u32 a)
{
	if (a == 0) return v;
	u32 r = v % a;
	return r ? v + (a - r) : v;
}

// imagexex /section: places the blob at last PE (VA+VirtualSize) rounded up
// to 0x80, then 0x80-aligns after each previous resource. Observed on Game11
// (.reloc ends 0x20DE22 -> DiscID at 0x8220DE80). PE headers are left alone.
static u32 nextSectionRva(const Xex& xex, const DataBlock& bf)
{
	u32 cursor = 0;
	if (xex.isBasefilePE()) {
		PEParser pe(bf);
		s32 n = pe.getNumSections();
		for (s32 i = 0; i < n; i++) {
			u32 addr = 0, size = 0;
			if (pe.getSectionAddr(i, addr) && pe.getSectionSize(i, size)) {
				u32 end = addr + size;
				if (end > cursor) cursor = end;
			}
		}
	}
	u32 load = xex.getLoadAddress();
	s32 nr = xex.numResources();
	for (s32 i = 0; i < nr; i++) {
		u32 addr = 0;
		s32 size = 0;
		char name[9];
		if (xex.getResource(i, addr, size, name) && addr >= load) {
			u32 end = (addr - load) + (u32)size;
			if (end > cursor) cursor = end;
		}
	}
	if (cursor == 0)
		cursor = (u32)bf.size();
	return alignUp(cursor, 0x80);
}

// Observed from MultiDisc DiscN_XEX.xml: <section file="N.bin" name="DiscID"
// memory="RO_ENCRYPTED"/>. IMAGEKEY_RESOURCE_SECTION name is 8 chars in the XEX.
static bool applySection(Xex& xex, const char* secName, const char* file,
                         const char* xmlPath, std::string& err)
{
	if (!secName || !*secName) { err = "<section> missing name"; return false; }
	if (!file || !*file) { err = "<section> missing file"; return false; }

	std::string path = joinPath(dirnameOf(xmlPath), file);
	FILE* f = fopen(path.c_str(), "rb");
	if (!f) { err = std::string("cannot read section file ") + path; return false; }
	if (fseek(f, 0, SEEK_END) != 0) { fclose(f); err = "cannot size " + path; return false; }
	long sz = ftell(f);
	if (sz <= 0) { fclose(f); err = "section file is empty: " + path; return false; }
	if (fseek(f, 0, SEEK_SET) != 0) { fclose(f); err = "cannot rewind " + path; return false; }
	std::vector<u8> blob((size_t)sz);
	if (sz > 0 && fread(&blob[0], 1, (size_t)sz, f) != (size_t)sz) {
		fclose(f); err = "cannot read " + path; return false;
	}
	fclose(f);

	s32 page = xex.getPageSize();
	if (page <= 0) page = 0x10000;
	DataBlock bf;
	xex.getBasefile(bf);
	s32 oldSize = bf.size();
	u32 rva = nextSectionRva(xex, bf);
	s32 need = (s32)rva + (s32)sz;
	s32 newSize = oldSize;
	if (need > newSize) {
		newSize = need;
		if (newSize % page)
			newSize += page - (newSize % page);
		bf.fill(0, oldSize, newSize - oldSize);
	}
	if (sz > 0)
		bf.set(&blob[0], (s32)rva, (s32)sz);
	if (!xex.setBasefile(bf)) {
		err = "failed to grow basefile for <section>";
		return false;
	}
	u32 va = xex.getLoadAddress() + rva;
	xex.addResource(va, (s32)sz, secName);
	s32 extra = xex.getImageSize() - oldSize;
	if (extra > 0)
		xex.addSection(extra, (u8)SECTIONINFO_READONLY);
	printf("applyxml: section %s %s -> 0x%08X (%ld bytes)\n",
	       secName, path.c_str(), va, sz);
	return true;
}

static bool applyXexXml(Xex& xex, const char* xmlPath, std::string& err)
{
	tinyxml2::XMLDocument doc;
	if (doc.LoadFile(xmlPath) != tinyxml2::XML_SUCCESS) {
		err = std::string("cannot read XML ") + xmlPath + ": " + (doc.ErrorStr() ? doc.ErrorStr() : "");
		return false;
	}
	tinyxml2::XMLElement* root = doc.RootElement();
	if (!root || STRICMP(root->Name(), "xex") != 0) {
		err = "XML root must be <xex>";
		return false;
	}

	for (tinyxml2::XMLElement* e = root->FirstChildElement(); e; e = e->NextSiblingElement()) {
		const char* name = e->Name();
		if (STRICMP(name, "titleid") == 0) {
			const char* id = elemAttrOrText(e, "id");
			if (!id) { err = "<titleid> missing id"; return false; }
			ExecutionId exec = {0};
			xex.getExecutionId(exec);
			exec.titleId = (u32)strtoul(id, NULL, 0);
			xex.setExecutionId(exec);
		}
		else if (STRICMP(name, "alttitleid") == 0) {
			const char* id = elemAttrOrText(e, "id");
			if (!id) { err = "<alttitleid> missing id"; return false; }
			xex.addAltTitleId((u32)strtoul(id, NULL, 0));
		}
		else if (STRICMP(name, "privilege") == 0) {
			const char* id = elemAttrOrText(e, "id");
			if (!id) { err = "<privilege> missing id"; return false; }
			applyPrivilege(xex, (u32)strtoul(id, NULL, 0));
		}
		else if (STRICMP(name, "lankey") == 0) {
			const char* id = elemAttrOrText(e, "id");
			if (!id) { err = "<lankey> missing id"; return false; }
			LANKey key = {0};
			if (!parseHexBytes(id, key.data, (int)sizeof(key.data))) {
				err = "invalid <lankey> hex (need 16 bytes)";
				return false;
			}
			xex.setLANKey(key);
		}
		else if (STRICMP(name, "xapiheap") == 0 || STRICMP(name, "heap") == 0) {
			const char* sz = elemAttrOrText(e, "size", "id");
			if (!sz) { err = "<xapiheap> missing size"; return false; }
			xex.setHeapSize((s32)strtoul(sz, NULL, 0));
		}
		else if (STRICMP(name, "workspace") == 0) {
			const char* sz = elemAttrOrText(e, "size", "id");
			if (!sz) { err = "<workspace> missing size"; return false; }
			xex.setWorkspaceSize((s32)strtoul(sz, NULL, 0));
		}
		else if (STRICMP(name, "exportnames") == 0) {
			xex.setTitleExports(true);
		}
		else if (STRICMP(name, "mediatypes") == 0) {
			applyMediaTypes(xex, e);
		}
		else if (STRICMP(name, "section") == 0) {
			const char* secName = elemAttrOrText(e, "name");
			const char* file = e->Attribute("file");
			if (!file || !*file) file = e->Attribute("path");
			if (!applySection(xex, secName, file, xmlPath, err))
				return false;
		}
		else if (STRICMP(name, "baseaddr") == 0) {
			printf("applyxml: skipping <baseaddr> (load address is fixed at link time)\n");
		}
		else {
			printf("applyxml: ignoring unknown tag <%s>\n", name);
		}
	}
	stripOddIncompatibleMedia(xex);
	return true;
}

int runApplyXmlCommand(int argc, char* argv[])
{
	std::string input, xml, out;
	for (int i = 2; i < argc; i++) {
		std::string a = argv[i];
		if (a == "-h" || a == "--help") { printf("%s", APPLYXML_USAGE); return 0; }
		else if (a == "-o" || a == "--out") { if (i + 1 >= argc) { printf("%s", APPLYXML_USAGE); return 1; } out = argv[++i]; }
		else if (a == "--xml") { if (i + 1 >= argc) { printf("%s", APPLYXML_USAGE); return 1; } xml = argv[++i]; }
		else if (!a.empty() && a[0] == '-') { printf("Unknown option: %s\n\n%s", a.c_str(), APPLYXML_USAGE); return 1; }
		else if (input.empty()) input = a;
		else { printf("Unexpected argument: %s\n\n%s", a.c_str(), APPLYXML_USAGE); return 1; }
	}
	if (input.empty() || xml.empty()) {
		printf("%s", APPLYXML_USAGE);
		return 1;
	}
	if (out.empty()) out = input;

	Xex xex;
	XexReader reader;
	if (!reader.read(xex, input.c_str())) {
		printf("applyxml: failed to read %s\n", input.c_str());
		return 1;
	}
	std::string err;
	if (!applyXexXml(xex, xml.c_str(), err)) {
		printf("applyxml: %s\n", err.c_str());
		return 1;
	}
	XexWriter writer;
	if (!writer.write(xex, out.c_str())) {
		printf("applyxml: failed to write %s\n", out.c_str());
		return 1;
	}
	printf("applyxml: wrote %s\n", out.c_str());
	return 0;
}
