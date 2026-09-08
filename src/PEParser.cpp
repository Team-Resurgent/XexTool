// 
// parses the PE file format
// 

#include "PEParser.h"
#include <stddef.h>
#include <string.h>

// do byte packing
#pragma pack(push, BytePack, 1)

typedef struct _IMAGE_DOS_HEADER {   // DOS .EXE header
	u16	e_magic;                     // Magic number
	u16	e_cblp;                      // Bytes on last page of file
	u16	e_cp;                        // Pages in file
	u16	e_crlc;                      // Relocations
	u16	e_cparhdr;                   // Size of header in paragraphs
	u16	e_minalloc;                  // Minimum extra paragraphs needed
	u16	e_maxalloc;                  // Maximum extra paragraphs needed
	u16	e_ss;                        // Initial (relative) SS value
	u16	e_sp;                        // Initial SP value
	u16	e_csum;                      // Checksum
	u16	e_ip;                        // Initial IP value
	u16	e_cs;                        // Initial (relative) CS value
	u16	e_lfarlc;                    // File address of relocation table
	u16	e_ovno;                      // Overlay number
	u16	e_res[4];                    // Reserved words
	u16	e_oemid;                     // OEM identifier (for e_oeminfo)
	u16	e_oeminfo;                   // OEM information; e_oemid specific
	u16	e_res2[10];                  // Reserved words
	u32	e_lfanew;                    // File address of new exe header
} IMAGE_DOS_HEADER, *PIMAGE_DOS_HEADER;

typedef struct _IMAGE_FILE_HEADER {
	u16 Machine;
	u16 NumberOfSections;
	u32 TimeDateStamp;
	u32 PointerToSymbolTable;
	u32 NumberOfSymbols;
	u16 SizeOfOptionalHeader;
	u16 Characteristics;
} IMAGE_FILE_HEADER, *PIMAGE_FILE_HEADER;

typedef struct _IMAGE_DATA_DIRECTORY {
	u32	VirtualAddress;
	u32	Size;
} IMAGE_DATA_DIRECTORY, *PIMAGE_DATA_DIRECTORY;

#define IMAGE_NUMBEROF_DIRECTORY_ENTRIES    16

typedef struct _IMAGE_OPTIONAL_HEADER {
    //
    // Standard fields.
    //

	u16	 Magic;
	u8	 MajorLinkerVersion;
	u8	 MinorLinkerVersion;
	u32	SizeOfCode;
	u32	SizeOfInitializedData;
	u32	SizeOfUninitializedData;
	u32	AddressOfEntryPoint;
	u32	BaseOfCode;
	u32	BaseOfData;

    //
    // NT additional fields.
    //

	u32	ImageBase;
	u32	SectionAlignment;
	u32	FileAlignment;
	u16	 MajorOperatingSystemVersion;
	u16	 MinorOperatingSystemVersion;
	u16	 MajorImageVersion;
	u16	 MinorImageVersion;
	u16	 MajorSubsystemVersion;
	u16	 MinorSubsystemVersion;
	u32	Win32VersionValue;
	u32	SizeOfImage;
	u32	SizeOfHeaders;
	u32	CheckSum;
	u16	 Subsystem;
	u16	 DllCharacteristics;
	u32	SizeOfStackReserve;
	u32	SizeOfStackCommit;
	u32	SizeOfHeapReserve;
	u32	SizeOfHeapCommit;
	u32	LoaderFlags;
	u32	NumberOfRvaAndSizes;
    IMAGE_DATA_DIRECTORY DataDirectory[IMAGE_NUMBEROF_DIRECTORY_ENTRIES];
} IMAGE_OPTIONAL_HEADER32, *PIMAGE_OPTIONAL_HEADER32;

typedef struct _IMAGE_SECTION_HEADER {
    char Name[8];
    union {
        	u32	PhysicalAddress;
        	u32	VirtualSize;
    } Misc;
	u32 VirtualAddress;
	u32 SizeOfRawData;
	u32 PointerToRawData;
	u32 PointerToRelocations;
	u32 PointerToLinenumbers;
	u16 NumberOfRelocations;
	u16 NumberOfLinenumbers;
	u32 Characteristics;
} IMAGE_SECTION_HEADER, *PIMAGE_SECTION_HEADER;

// resume normal packing
#pragma pack(pop, BytePack)


PEParser::PEParser(const DataBlock& peData)
	: m_pPEData(NULL)
{
	m_pPEData = &peData;
}

PEParser::~PEParser()
{
}


s32 PEParser::getPEOffset() const
{
	return m_pPEData->get32le( offsetof(IMAGE_DOS_HEADER, e_lfanew) ) + 4;
}

s32 PEParser::getOptionalHeaderOffset() const
{
	return getPEOffset() + sizeof(IMAGE_FILE_HEADER);
}

s32 PEParser::getSectionsOffset() const
{
	s32 opt_header_size = m_pPEData->get16le( getPEOffset() + offsetof(IMAGE_FILE_HEADER, SizeOfOptionalHeader) );
	return getOptionalHeaderOffset() + opt_header_size;
}


s32  PEParser::getNumSections() const
{
	return m_pPEData->get16le( getPEOffset() + offsetof(IMAGE_FILE_HEADER, NumberOfSections) );
}

bool PEParser::getSection(s32 index, DataBlock& sectionData) const
{
	if(index < 0 || index >= getNumSections())
		return false;
	
	s32 section_offset = getSectionsOffset() + index * sizeof(IMAGE_SECTION_HEADER);
	m_pPEData->get(sectionData, 0, section_offset, sizeof(IMAGE_SECTION_HEADER));
	return true;
}

bool PEParser::getSectionName(s32 index, char* name) const
{
	DataBlock section_data;
	if( !getSection(index, section_data) )
		return false;
	section_data.get(name, offsetof(IMAGE_SECTION_HEADER, Name), 8);
	name[8] = 0;
	return true;
}

bool PEParser::getSectionAddr(s32 index, u32& addr) const
{
	DataBlock section_data;
	if( !getSection(index, section_data) )
		return false;
	addr = section_data.get32le(offsetof(IMAGE_SECTION_HEADER, VirtualAddress));
	return true;
}

bool PEParser::getSectionSize(s32 index, u32& size) const
{
	DataBlock section_data;
	if( !getSection(index, section_data) )
		return false;
	size = section_data.get32le(offsetof(IMAGE_SECTION_HEADER, Misc.VirtualSize));
	return true;
}

bool PEParser::getSectionFlags(s32 index, u32& flags) const // section characteristics
{
	DataBlock section_data;
	if( !getSection(index, section_data) )
		return false;
	flags = section_data.get32le(offsetof(IMAGE_SECTION_HEADER, Characteristics));
	return true;
}


bool PEParser::isPeFile() const
{
	u8 mz_magic[2];
	u8 pe_magic[2];
	m_pPEData->get(mz_magic, 0, 2);
	if( memcmp(mz_magic, "MZ", 2) ) return false;
	m_pPEData->get(pe_magic, getPEOffset()-4, 2);
	return memcmp(pe_magic, "PE", 2) == 0;
}


u32 PEParser::getEntryPoint() const
{
	return m_pPEData->get32le( getOptionalHeaderOffset() + offsetof(IMAGE_OPTIONAL_HEADER32, AddressOfEntryPoint) );
}
u32 PEParser::getLoadAddress() const
{
	return m_pPEData->get32le( getOptionalHeaderOffset() + offsetof(IMAGE_OPTIONAL_HEADER32, ImageBase) );
}
u32 PEParser::getChecksum() const
{
	return m_pPEData->get32le( getOptionalHeaderOffset() + offsetof(IMAGE_OPTIONAL_HEADER32, CheckSum) );
}
u32 PEParser::getFiletime() const
{
	return m_pPEData->get32le( getPEOffset() + offsetof(IMAGE_FILE_HEADER, TimeDateStamp) );
}
u16 PEParser::getCharacteristics() const
{
	return m_pPEData->get32le( getPEOffset() + offsetof(IMAGE_FILE_HEADER, Characteristics) );
}
