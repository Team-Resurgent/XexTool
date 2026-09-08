// 
// packs exe-data in xex files
// 
 
#ifndef _XEX_PACKER_H_
#define _XEX_PACKER_H_

#include "XexDefines.h"
#include "DataBlock.h"

class XexPacker
{
public:
	XexPacker()		{ m_callbackData = NULL; }
	~XexPacker()	{}
	
	bool packBinary(DataBlock& basefileOut, const DataBlock& basefileIn,
					bool isEncrypted, const XexKey& encKey,
					DataBlock& unpackInfo, s32 imageSize);
	bool packRaw(DataBlock& basefileOut, const DataBlock& basefileIn,
					bool isEncrypted, const XexKey& encKey,
					DataBlock& unpackInfo, s32 imageSize);
	bool packCompressed(DataBlock& basefileOut, const DataBlock& basefileIn,
					bool isEncrypted, const XexKey& encKey,
					DataBlock& unpackInfo, s32 imageSize);
//	bool packDeltaCompressed(DataBlock& basefileOut, const DataBlock& basefileIn,
//					bool isEncrypted, const XexKey& encKey,
//					DataBlock& unpackInfo, s32 imageSize);
	
	bool unpackBinary(DataBlock& basefileOut, const DataBlock& basefileIn,
					bool isEncrypted, const XexKey& encKey,
					const DataBlock& unpackInfo, s32 imageSize);
	bool unpackRaw(DataBlock& basefileOut, const DataBlock& basefileIn,
					bool isEncrypted, const XexKey& encKey,
					const DataBlock& unpackInfo, s32 imageSize);
	bool unpackCompressed(DataBlock& basefileOut, const DataBlock& basefileIn,
					bool isEncrypted, const XexKey& encKey,
					const DataBlock& unpackInfo, s32 imageSize);
//	bool unpackDeltaCompressed(DataBlock& basefileOut, const DataBlock& basefileIn,
//					bool isEncrypted, 
//					const XexKey& xex_key, const XexKey& patch_key, const XexKey& patch_hdr_key,
//					const DataBlock& unpackInfo, s32 imageSize);
	
private:
	// decrypt/encrypt data in place
	bool decrypt(DataBlock& basefile, const XexKey& decKey);
	bool encrypt(DataBlock& basefile, const XexKey& encKey);
	
	static int s_compressionCallback(void* param, unsigned char* compData, long compDataSize, long uncompDataSize);
	int compressionCallback(void* param, unsigned char* compData, long compDataSize, long uncompDataSize);
	DataBlock* m_callbackData;
};

#endif // _XEX_PACKER_H_

