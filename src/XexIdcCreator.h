// 
// XexIdcCreator.h
// 
// dumps idc info for basefile to a file
// 


#ifndef _XEX_IDC_CREATOR_H_
#define _XEX_IDC_CREATOR_H_

#include <stdio.h>

class Xex;

class XexIdcCreator
{
public:
	XexIdcCreator();
	~XexIdcCreator();
	
	// dumps the basefile info to a file
	bool dump(const Xex& xex, const char* filename);
	bool dump(const Xex& xex, FILE* fd);
	
private:
	const Xex* m_pXex;
};

#endif // _XEX_IDC_CREATOR_H_

