// SPDX-License-Identifier: GPL-3.0-or-later
// Part of RXDK-360 - see LICENSE.md for the full GNU GPL v3.
//
// C++ port of tools/gen_import_stubs.py. See XexGenStubs.h for the summary and
// that script for the detailed rationale of the two import forms (call thunk vs
// variable record) and the section layout.

#include "XexGenStubs.h"

#include <cstdio>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>
#include <unordered_map>
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

static const uint16_t IMAGE_FILE_MACHINE_POWERPCBE = 0x01F2;

// Kernel exports the console has but the public XDK import libs don't expose;
// real xboxkrnl.exe ordinals, used only as a fallback. (function, not variable)
struct Supp { const char* name; uint32_t ord; };
static const Supp SUPPLEMENTAL[] = {
    {"KeTlsAlloc", 0x152}, {"KeTlsFree", 0x153},
    {"KeTlsGetValue", 0x154}, {"KeTlsSetValue", 0x155},
};

// module (DLL) name to carry when a short-import's own DLL field is empty.
static std::string moduleForBase(const std::string& base)
{
    if (base == "xboxkrnl") return "xboxkrnl.exe";
    if (base == "xbdm")     return "xbdm.xex";
    return "";
}

struct Import { std::string module; uint32_t ordinal; bool isVar; };

static uint16_t rd16(const uint8_t* d) { return (uint16_t)(d[0] | (d[1] << 8)); }

static std::vector<uint8_t> readFile(const std::string& p)
{
    std::ifstream f(p, std::ios::binary);
    return std::vector<uint8_t>((std::istreambuf_iterator<char>(f)),
                                std::istreambuf_iterator<char>());
}

// Parse a COFF short-import member: symbol, dll, ordinal/hint, import type
// (0=CODE, 1=DATA, 2=CONST; DATA/CONST => a variable). Returns false if too small.
static bool parseShortImport(const uint8_t* d, size_t n, std::string& sym,
                             std::string& dll, uint16_t& ord, int& importType)
{
    if (n < 22) return false;
    ord = rd16(d + 16);
    importType = rd16(d + 18) & 0x3;
    const char* s = reinterpret_cast<const char*>(d + 20);
    size_t max = n - 20;
    size_t symLen = strnlen(s, max);
    if (symLen >= max) return false;
    sym.assign(s, symLen);
    const char* r = s + symLen + 1;
    size_t rmax = max - symLen - 1;
    dll.assign(r, strnlen(r, rmax));
    return true;
}

// Walk the members of a Microsoft (!<arch>) archive.
template <typename F>
static void forEachMember(const std::vector<uint8_t>& blob, F cb)
{
    if (blob.size() < 8 || memcmp(blob.data(), "!<arch>\n", 8) != 0) return;
    size_t pos = 8;
    while (pos + 60 <= blob.size())
    {
        const uint8_t* h = blob.data() + pos;
        if (h[58] != '`' || h[59] != '\n') break;
        char nameBuf[17]; memcpy(nameBuf, h, 16); nameBuf[16] = 0;
        std::string name(nameBuf);
        while (!name.empty() && name.back() == ' ') name.pop_back();
        char sizeBuf[11]; memcpy(sizeBuf, h + 48, 10); sizeBuf[10] = 0;
        size_t size = strtoul(sizeBuf, nullptr, 10);
        size_t dataOff = pos + 60;
        if (dataOff + size > blob.size()) break;
        cb(name, blob.data() + dataOff, size);
        pos = dataOff + size + (size & 1);   // members are 2-byte aligned
    }
}

// name -> (module, ordinal, isVar) across every *.lib in the XDK lib dir.
static std::unordered_map<std::string, Import> buildOrdinalIndex(const std::string& xdkLibDir)
{
    std::unordered_map<std::string, Import> index;
    for (const auto& s : SUPPLEMENTAL)
        index[s.name] = Import{ "xboxkrnl.exe", s.ord, false };

    std::error_code ec;
    for (auto& e : fs::directory_iterator(xdkLibDir, ec))
    {
        if (ec) break;
        if (!e.is_regular_file()) continue;
        auto path = e.path();
        std::string ext = path.extension().string();
        for (auto& c : ext) c = (char)tolower((unsigned char)c);
        if (ext != ".lib") continue;
        std::string base = path.stem().string();
        for (auto& c : base) c = (char)tolower((unsigned char)c);

        auto blob = readFile(path.string());
        forEachMember(blob, [&](const std::string& name, const uint8_t* data, size_t size)
        {
            if (name == "/" || name == "//") return;
            if (size >= 2 && rd16(data) == IMAGE_FILE_MACHINE_POWERPCBE) return; // real COFF obj
            std::string sym, dll; uint16_t ord; int itype;
            if (!parseShortImport(data, size, sym, dll, ord, itype)) return;
            if (!ord) return;
            std::string module = dll;
            size_t at = module.find('@');
            if (at != std::string::npos) module = module.substr(0, at);
            while (!module.empty() && (module.back() == ' ' || module.back() == '\t')) module.pop_back();
            if (module.empty()) module = moduleForBase(base);
            if (module.empty()) return;
            if (index.find(sym) == index.end())      // setdefault: first wins
                index[sym] = Import{ module, ord, (itype == 1 || itype == 2) };
        });
    }
    return index;
}

struct Entry { std::string name; uint32_t ordinal; bool isVar; };

int runGenStubsCommand(int argc, char* argv[])
{
    std::string xdk, names, out = "stubs.s", manifest;
    for (int i = 2; i < argc; i++)
    {
        std::string a = argv[i];
        auto next = [&]() -> std::string { return (i + 1 < argc) ? argv[++i] : std::string(); };
        if (a == "--xdk") xdk = next();
        else if (a == "--names") names = next();
        else if (a == "-o" || a == "--out") out = next();
        else if (a == "--manifest") manifest = next();
        else if (a == "-h" || a == "--help")
        {
            printf("Usage: XexTool genstubs --xdk <lib dir> --names <a,b,..> -o <stubs.s> [--manifest <json>]\n");
            return 1;
        }
    }
    if (xdk.empty() || names.empty())
    {
        fprintf(stderr, "genstubs: --xdk and --names are required\n");
        return 1;
    }
    if (manifest.empty())
    {
        size_t dot = out.find_last_of('.');
        manifest = (dot == std::string::npos ? out : out.substr(0, dot)) + ".imports.json";
    }

    auto index = buildOrdinalIndex(xdk);

    // split --names on ','
    std::vector<std::string> undefined;
    for (size_t i = 0, j; i <= names.size(); i = j + 1)
    {
        j = names.find(',', i);
        if (j == std::string::npos) j = names.size();
        std::string n = names.substr(i, j - i);
        if (!n.empty()) undefined.push_back(n);
    }

    // resolve, preserving module insertion order and name order within a module
    std::vector<std::string> moduleOrder;
    std::unordered_map<std::string, std::vector<Entry>> resolved;
    std::vector<std::string> unresolved;
    for (auto& n : undefined)
    {
        auto it = index.find(n);
        if (it == index.end()) { unresolved.push_back(n); continue; }
        if (resolved.find(it->second.module) == resolved.end())
            moduleOrder.push_back(it->second.module);
        resolved[it->second.module].push_back(Entry{ n, it->second.ordinal, it->second.isVar });
    }

    // emit the stub assembly (built as a line list, joined with '\n', to match
    // the reference emitter's exact byte layout - one trailing newline, no final
    // blank line).
    {
        std::vector<std::string> lines = {
            "# Generated import thunks -- do not edit.", "    .section .kthunks,\"ax\"", "" };
        for (auto& m : moduleOrder)
            for (auto& e : resolved[m])
            {
                if (e.isVar) continue;
                char buf[64];
                snprintf(buf, sizeof buf, "    .long 0x%08X, 0, 0, 0", 0x01000000u | e.ordinal);
                lines.push_back("    .globl " + e.name);
                lines.push_back(e.name + ":");
                lines.push_back(buf);
                lines.push_back("");
            }
        lines.push_back("    .section .kvars,\"a\"");
        lines.push_back("");
        for (auto& m : moduleOrder)
            for (auto& e : resolved[m])
            {
                lines.push_back("    .globl __imp_" + e.name);
                if (e.isVar)
                {
                    lines.push_back("    .globl " + e.name);
                    lines.push_back("    .p2align 2");
                    lines.push_back(e.name + ":");
                }
                char buf[32]; snprintf(buf, sizeof buf, "    .long 0x%08X", e.ordinal);
                lines.push_back("__imp_" + e.name + ":");
                lines.push_back(buf);
                lines.push_back("");
            }
        std::ofstream f(out, std::ios::binary);
        for (size_t i = 0; i < lines.size(); i++)
            f << lines[i] << (i + 1 < lines.size() ? "\n" : "");
    }

    // emit the JSON manifest (one record per import-record address, once each)
    {
        std::ofstream f(manifest, std::ios::binary);
        f << "{\n  \"libraries\": [\n";
        for (size_t mi = 0; mi < moduleOrder.size(); mi++)
        {
            auto& m = moduleOrder[mi];
            f << "    {\n      \"module\": \"" << m << "\",\n      \"records\": [\n";
            std::vector<std::string> recs;
            for (auto& e : resolved[m])
            {
                recs.push_back("__imp_" + e.name);
                if (!e.isVar) recs.push_back(e.name);
            }
            for (size_t ri = 0; ri < recs.size(); ri++)
                f << "        \"" << recs[ri] << "\"" << (ri + 1 < recs.size() ? "," : "") << "\n";
            f << "      ]\n    }" << (mi + 1 < moduleOrder.size() ? "," : "") << "\n";
        }
        f << "  ]\n}";   // json.dump writes no trailing newline
    }

    // summary
    size_t total = 0, nvar = 0;
    for (auto& m : moduleOrder) { total += resolved[m].size(); for (auto& e : resolved[m]) nvar += e.isVar ? 1 : 0; }
    printf("%s: %zu imports across %zu module(s)%s\n", out.c_str(), total, moduleOrder.size(),
           nvar ? (" (" + std::to_string(nvar) + " variable)").c_str() : "");
    for (auto& m : moduleOrder)
    {
        printf("  %s: ", m.c_str());
        bool first = true;
        for (auto& e : resolved[m])
        {
            printf("%s%s(0x%X%s)", first ? "" : ", ", e.name.c_str(), e.ordinal, e.isVar ? "/var" : "");
            first = false;
        }
        printf("\n");
    }
    if (!unresolved.empty())
    {
        printf("  unresolved (not kernel imports): ");
        for (size_t i = 0; i < unresolved.size(); i++)
            printf("%s%s", i ? ", " : "", unresolved[i].c_str());
        printf("\n");
    }
    return 0;
}
