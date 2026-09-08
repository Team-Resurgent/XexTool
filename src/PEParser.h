// 
// parses the PE file format
// 

#ifndef _PE_PARSER_H_
#define _PE_PARSER_H_

#include "DataBlock.h"
#include "types.h"

class PEParser
{
public:
	PEParser(const DataBlock& peData);
	~PEParser();
	
	bool isPeFile() const;
	
	u32 getEntryPoint() const;
	u32 getLoadAddress() const;
	u32 getChecksum() const;
	u32 getFiletime() const;
	u16 getCharacteristics() const;

	s32 getNumSections() const;
	bool getSectionName(s32 index, char* name) const;
	bool getSectionAddr(s32 index, u32& addr) const;
	bool getSectionSize(s32 index, u32& size) const;
	bool getSectionFlags(s32 index, u32& flags) const; // section characteristics

private:
	const DataBlock* m_pPEData;
	
	s32 getPEOffset() const;
	s32 getOptionalHeaderOffset() const;
	s32 getSectionsOffset() const;
	bool getSection(s32 index, DataBlock& sectionData) const;
};


#endif // _PE_PARSER_H_


