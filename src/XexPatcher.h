// 
// patches xex files with xex patch files
// 

#ifndef _XEX_PATCHER_H_
#define _XEX_PATCHER_H_

#include "DataBlock.h"

class Xex;

class XexPatcher {
public:
	XexPatcher();
	~XexPatcher();
	
	// checks if the patch xex is the correct one for the source xex
	bool isCorrectPatch(const Xex& sourceXex, const Xex& patchXex) const;
	
	// create an xex from the source xex and patch xex
	// the output xex must not be the same as the input xex or patch
	bool patch(Xex& outputXex, const Xex& sourceXex, const Xex& patchXex);
	
private:
	Xex* m_pTargetXex;			// target xex

	bool XexpDeltaDecompress(s32 windowSize, DataBlock& output, const u8* inputBuff, s32 inputSize);
	bool unpackDeltaHeaders(DataBlock& headersOut, const DataBlock& headersIn, const DataBlock& patchInfo);
	bool unpackDeltaBasefile(DataBlock& basefileTarget, s32 targetImageSize, const DataBlock& basefileSource, const DataBlock& basefilePatch, const DataBlock& basefilePatchInfo, const u8* decKey);
};


#endif // _XEX_PATCHER_H_


