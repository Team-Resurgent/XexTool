// 
// used to dump resources from an xex file
// 

#ifndef _XEX_RESOURCE_DUMPER_H_
#define _XEX_RESOURCE_DUMPER_H_

#include <stdio.h>
#include "types.h"

class Xex;

class XexResourceDumper
{
public:
	XexResourceDumper(const Xex& xex);
	~XexResourceDumper();
	
	// resource name funcs
	const char* nameFromIndex(s32 resourceIndex);
	s32 indexFromName(const char* resourceName);
	
	// dump resources
	bool dump(const char* dirname);
	bool dump(s32 resourceIndex, const char* filename);
	bool dump(s32 resourceIndex, FILE* fd);
	bool dump(s32 resourceIndex, u8*& resData, s32& resSize);
	
private:
	const Xex* m_pXex;
};


#endif // _XEX_RESOURCE_DUMPER_H_

