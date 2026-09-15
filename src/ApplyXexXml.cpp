//
// applyxml: overlay an imagexex-style <xex> XML onto a packed XEX.
//
// Observed from imagexex.exe on a legacy PE: /titleid, /lankey, /xapiheap,
// /workspace, /privilege:N, /exportnames, and /config:<xex> XML write the same
// optional headers. Privilege N is bit N of IMAGEKEY_SYSTEM_FLAGS (N<32) or
// SYSTEM_FLAGS2 (N>=32). Additional <section> blobs are not applied here —
// imagexex grows the PE; this pass only edits headers.
//

#include "ElfPacker.h"
#include "Xex.h"
#include "XexReader.h"
#include "XexWriter.h"
#include "tinyxml2.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

static const char APPLYXML_USAGE[] =
	"Usage:    XexTool applyxml <input.xex> --xml <file.xml> [-o <output.xex>]\n"
	"\n"
	"Apply an imagexex-style <xex> XML onto an existing XEX (title id, privileges,\n"
	"LAN key, heap/workspace sizes, export-by-name). Unspecified fields are left\n"
	"alone. If -o is omitted the input file is overwritten.\n";

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
		else if (STRICMP(name, "section") == 0) {
			printf("applyxml: skipping <section> (extra XEX resources need a PE rebuild)\n");
		}
		else if (STRICMP(name, "baseaddr") == 0) {
			printf("applyxml: skipping <baseaddr> (load address is fixed at link time)\n");
		}
		else {
			printf("applyxml: ignoring unknown tag <%s>\n", name);
		}
	}
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
