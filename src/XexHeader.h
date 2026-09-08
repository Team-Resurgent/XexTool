// 
// handles all data in an xex header
// 

#ifndef _XEX_HEADER_H_
#define _XEX_HEADER_H_

#include "types.h"
#include "XexDefines.h"
#include "DataBlock.h"
#include "XexImageEntryTypes.h"
#include <vector>

class Xex;

class XexHeader
{
public:
	XexHeader();
	XexHeader(XexHeader const& source);
	XexHeader& operator=(XexHeader const& source);
	~XexHeader();
	
	// sets up xex from xex headers
	bool readHeaders(Xex& xex, const DataBlock& headers, s32& imageSize, DataBlock& basefileFormat);
	
	// sets xex headers from an xex
	bool writeHeaders(const Xex& xex, DataBlock& headers, const s32& imageSize, const DataBlock& basefileFormat);
	
private:
	void convertXexVersionInfoFromBE(XexVersionInfo& ver);
	void convertXexVersionInfoToBE(XexVersionInfo& ver);
	void convertXexVersion32FromBE(XexVersion32& ver);
	void convertXexVersion32ToBE(XexVersion32& ver);
	
	void convertXexHeaderFromBE(XexImageHeader& xexHeader);
	void convertXexHeaderToBE(XexImageHeader& xexHeader);
	void getXexHeader(XexImageHeader& xexHeader, s32 basefileOffset, s32 securityInfoOffset, s32 numOptionalInfo);
	void setXexHeader(const XexImageHeader& xexHeader);
	
	void convertSecurityInfoFromBE(XexSecurityInfo& secInfo);
	void convertSecurityInfoToBE(XexSecurityInfo& secInfo);
	void getSecurityInfo(XexSecurityInfo& secInfo);
	void setSecurityInfo(const XexSecurityInfo& secInfo);
	
	void convertSectionFromBE(XexHvSectionInfo& section);
	void convertSectionToBE(XexHvSectionInfo& section);
	bool getSection(s32 index, XexHvSectionInfo& section);
	void addSection(const XexHvSectionInfo& section);
	
	void convertImageEntryHeaderFromBE(XexImageEntry& header);
	void convertImageEntryHeaderToBE(XexImageEntry& header);
	void convertImageEntryDataFromBE(const XexImageEntry& header, DataBlock& data);
	void convertImageEntryDataToBE(const XexImageEntry& header, DataBlock& data);
	s32  getNumImageEntries();
	s32  getImageEntryDataSize();
	void getImageEntry(XexImageEntry* headers, u8* data, s32 dataSize, s32 dataOffset);
	bool addImageEntry(const XexImageEntry& header, const DataBlock& data);
	
	// functions to encrypt the xex image key
	bool encKey(XexKey& dataKey, const XexKey& cryptKey);
	bool encRetailKey(XexKey& dataKey);
	bool encDebugKey(XexKey& dataKey);
	bool encMfgRetailKey(XexKey& dataKey);
	bool encMfgDebugKey(XexKey& dataKey);
	bool encKey(XexKey& dataKey, bool isRetail, bool isMfg);
	
	// functions to decrypt the xex image key
	bool decKey(XexKey& dataKey, const XexKey& cryptKey);
	bool decRetailKey(XexKey& dataKey);
	bool decDebugKey(XexKey& dataKey);
	bool decMfgRetailKey(XexKey& dataKey);
	bool decMfgDebugKey(XexKey& dataKey);
	bool decKey(XexKey& dataKey, bool isRetail, bool isMfg);
	
	bool verifySign(const u8* publicKey, const XexSecurityInfo& secInfo);
	bool updateSign(XexSecurityInfo& secInfo, const u8* publicKey, const u8* privateKey);
	bool updateDebugSign(XexSecurityInfo& secInfo);
	bool updateRetailSign(XexSecurityInfo& secInfo);
	
	bool updateImportHash(u8* headersData);
	bool updateSectionHashes(XexHvSectionInfo* sections, XexSecurityInfo& secInfo);
	bool updateHeaderHash(u8* headersData, s32 headersSize, s32 securityInfoOffset);
	
	DataBlock m_headers;
	DataBlock m_basefileFormat;
	Xex* m_pXex;
};


#endif // _XEX_HEADER_H_

