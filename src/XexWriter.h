// 
// writes out xexs
// 

#ifndef _XEX_WRITER_H_
#define _XEX_WRITER_H_

#include <stdio.h>
#include "XexDefines.h"
#include "DataBlock.h"

class Xex;

class XexWriter
{
public:
	XexWriter();
	~XexWriter();
	
	// write out xex
	bool write(const Xex& xex, const char* filename);
	bool write(const Xex& xex, FILE* fd);
	
	// if successful, 'data' needs to be freed
	bool write(const Xex& xex, void*& data, int& size);
	
private:
/*
	void convertXexHeaderToBE(XexImageHeader& xexHeader);
	void getXexHeader(XexImageHeader& xexHeader, s32 basefileOffset, s32 securityInfoOffset, s32 numOptionalInfo);
	
	void convertSecurityInfoToBE(XexSecurityInfo& secInfo);
	void getSecurityInfo(XexSecurityInfo& secInfo);
	
	void convertSectionToBE(XexHvSectionInfo& section);
	bool getSection(s32 index, XexHvSectionInfo& section);
	
	s32  getNumImageEntries();
	s32  getImageEntriesDataSize();
	void getImageEntry(XexImageEntry* headers, u8* data, s32 dataSize, s32 dataOffset);
	void convertImageEntryHeaderToBE(XexImageEntry& header);
	void convertImageEntryDataToBE(const XexImageEntry& header, DataBlock& data);
	
	// functions to encrypt/decrypt xex crypt key
	bool encKey(XexKey& dataKey, const XexKey& cryptKey);
	bool encRetailKey(XexKey& dataKey);
	bool encDebugKey(XexKey& dataKey);
	bool encMfgRetailKey(XexKey& dataKey);
	bool encMfgDebugKey(XexKey& dataKey);
//	bool decKey(XexKey& dataKey, const XexKey& cryptKey);
//	bool decRetailKey(XexKey& dataKey);
//	bool decDebugKey(XexKey& dataKey);
//	bool decMfgRetailKey(XexKey& dataKey);
//	bool decMfgDebugKey(XexKey& dataKey);
	
	bool updateSign(XexSecurityInfo& secInfo, const u8* publicKey, const u8* privateKey);
	bool updateDebugSign(XexSecurityInfo& secInfo);
	bool updateRetailSign(XexSecurityInfo& secInfo);
	
	bool updateImportHash(u8* headersData);
	bool updateSectionHashes(XexHvSectionInfo* sections, XexSecurityInfo& secInfo);
	bool updateHeaderHash(u8* headersData, s32 headersSize, s32 securityInfoOffset);
	
	const Xex* m_pXex;
	// we store the basefile format in here for packing purposes
	DataBlock m_basefileFormat;
	*/

//	const Xex* m_pXex;
};

#endif // _XEX_WRITER_H_

