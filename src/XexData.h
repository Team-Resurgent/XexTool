// 
// static xex data for things such as keys
// this allows all the data to be kept in one
// place and only updated once if they are discovered or changed
// 

#ifndef _XEX_DATA_H_
#define _XEX_DATA_H_

#include "types.h"

// uncomment this to include the real retail key
#define ALLOW_ALL_KEYS

class XexData
{
public:
	XexData();
	~XexData(){}
	
	// magic value at start of xex file
	static u8	XEX_MAGIC[4];
	
	// keys used to encrypt/decrypt xex files
	static u8	XEX_RETAIL_KEY[16];
	static u8	XEX_DEBUG_KEY[16];
	static u8	XEX_MFG_RETAIL_KEY[16];
	static u8	XEX_MFG_DEBUG_KEY[16];
	
	// salt used for signing/verifying xex files
	static u8	XEX_SALT_XEX[10];
	static u8	XEX_SALT_REV[10];
	
	// public keys used to verify the signature on xex files
	static u8	XEX_RETAIL_PUBLIC_KEY[272];
	static u8	XEX_DEBUG_PUBLIC_KEY[272];
	
	// private keys used to update the signature on xex files
	static u8	XEX_RETAIL_PRIVATE_KEY[1232];
	static u8	XEX_DEBUG_PRIVATE_KEY[1232];
private:
	void prepareKey(void* data, s32 size);
	void prepareAllKeys();
	static bool m_inited;
	// have an instance of itself to force constructor to be called
//	XexData xex_data;
};

#endif // _XEX_DATA_H_

