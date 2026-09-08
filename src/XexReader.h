// 
// reads in xexs
// 

#ifndef _XEX_READER_H_
#define _XEX_READER_H_

#include <stdio.h>
#include "XexDefines.h"
#include "Xex.h"
#include "DataBlock.h"

//class Xex;
//class XexHeader;

class XexReader
{
public:
	XexReader();
	~XexReader();
	
	// read in xex
	bool read(Xex& xex, const char* filename);
	bool read(Xex& xex, FILE* fd);
	bool read(Xex& xex, const void* data, int size);
	
private:
/*
	void convertXexHeaderFromBE(XexImageHeader& xexHeader);
	void setXexHeader(const XexImageHeader& xexHeader);
	
	void convertSecurityInfoFromBE(XexSecurityInfo& secInfo);
	void setSecurityInfo(const XexSecurityInfo& secInfo);
	
	void convertSectionFromBE(XexHvSectionInfo& section);
	void addSection(const XexHvSectionInfo& section);
	
	void convertImageEntryHeaderFromBE(XexImageEntry& header);
	void convertImageEntryDataFromBE(const XexImageEntry& header, DataBlock& data);
	bool addImageEntry(const XexImageEntry& header, const DataBlock& data);
	
	// functions to encrypt/decrypt xex crypt key
	bool encKey(XexKey& dataKey, const XexKey& cryptKey);
	bool encRetailKey(XexKey& dataKey);
	bool encDebugKey(XexKey& dataKey);
	bool encMfgRetailKey(XexKey& dataKey);
	bool encMfgDebugKey(XexKey& dataKey);
	bool decKey(XexKey& dataKey, const XexKey& cryptKey);
	bool decRetailKey(XexKey& dataKey);
	bool decDebugKey(XexKey& dataKey);
	bool decMfgRetailKey(XexKey& dataKey);
	bool decMfgDebugKey(XexKey& dataKey);
	
	// need to be able to verify signature to determine if retail or debug xex
	bool verifySign(const u8* publicKey, const XexSecurityInfo& secInfo);
	
	// we store the basefile format in here for unpacking purposes
//	DataBlock m_basefileFormat;
*/	
//	Xex* m_pXex;
};

#endif // _XEX_READER_H_

