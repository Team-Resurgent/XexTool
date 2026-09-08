// 
// dumps basefile from xex
// 

#include "XexBasefileDumper.h"
#include "Xex.h"
#include "DataBlock.h"

XexBasefileDumper::XexBasefileDumper()
{
}
XexBasefileDumper::~XexBasefileDumper()
{
}


// write out basefile
bool XexBasefileDumper::dump(const Xex& xex, const char* filename)
{
	if(filename == NULL) return false;
//	if( !m_pXex->isBasefilePE() ) return false;
	
	FILE* fd = fopen(filename, "w+b");
	if(fd == NULL)
		return false;
	bool result = dump(xex, fd);
	fclose(fd);
	return result;
}

bool XexBasefileDumper::dump(const Xex& xex, FILE* fd)
{
	if(fd == NULL) return false;
//	if( !m_pXex->isBasefilePE() ) return false;
	
	DataBlock basefile;
	xex.getBasefile(basefile);
	u8* basefile_data = new u8[basefile.size()];
	basefile.get(basefile_data, 0, basefile.size());
	if( fseek(fd, 0, SEEK_SET) != 0 ) { delete[] basefile_data; return false; };
	if( fwrite(basefile_data, basefile.size(), 1, fd) != 1) { delete[] basefile_data; return false; };
	delete[] basefile_data;
	return true;
}

