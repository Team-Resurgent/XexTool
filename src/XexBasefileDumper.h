// 
// dumps basefile from xex
// 

#ifndef _XEX_BASEFILE_DUMPER_H_
#define _XEX_BASEFILE_DUMPER_H_

#include <stdio.h>

class Xex;

class XexBasefileDumper
{
public:
	XexBasefileDumper();
	~XexBasefileDumper();
	
	// write out basefile
	bool dump(const Xex& xex, const char* filename);
	bool dump(const Xex& xex, FILE* fd);
	
private:
	const Xex* m_pXex;
};

#endif // _XEX_BASEFILE_DUMPER_H_

