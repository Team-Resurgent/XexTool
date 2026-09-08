// 
// used to dump resources from an xex file
// 

#include "XexResourceDumper.h"
#include "Xex.h"
#include "DataBlock.h"

XexResourceDumper::XexResourceDumper(const Xex& xex)
{
	m_pXex = &xex;
}
XexResourceDumper::~XexResourceDumper()
{
}


// resource name funcs
const char* XexResourceDumper::nameFromIndex(s32 resourceIndex)
{
	if( resourceIndex < 0 || resourceIndex >= m_pXex->numResources() )
		return NULL;
	
	// get resource details
	u32 res_addr;
	s32 res_size;
	static char res_name[260];
	res_name[0] = 0;
	if( !m_pXex->getResource(resourceIndex, res_addr, res_size, res_name) )
		return NULL;
	return res_name;
}

s32 XexResourceDumper::indexFromName(const char* resourceName)
{
	if( resourceName == NULL )
		return -1;
	for(int res_idx=0; res_idx<m_pXex->numResources(); res_idx++)
	{
		// get resource details
		u32 res_addr;
		s32 res_size;
		char res_name[260];
		res_name[0] = 0;
		if( !m_pXex->getResource(res_idx, res_addr, res_size, res_name) )
			return false;
		if( strcmp(res_name, resourceName) == 0 )
			return res_idx;
	}
	return -1;
}


// dump resources
bool XexResourceDumper::dump(const char* dirname)
{
	if(dirname == NULL) return false;
	
	for(int res_idx=0; res_idx<m_pXex->numResources(); res_idx++)
	{
		// get resource details
		u32 res_addr;
		s32 res_size;
		char res_name[260];
		res_name[0] = 0;
		if( !m_pXex->getResource(res_idx, res_addr, res_size, res_name) )
			return false;
		if( strlen(res_name) == 0 )
			sprintf(res_name, "resource_%d", res_idx);

		// extract resource data
		char res_filename[260];
		sprintf(res_filename, "%s\\%s", dirname, res_name);
		if( !dump(res_idx, res_filename) )
			return false;
	}
	
	return true;
}


bool XexResourceDumper::dump(s32 resourceIndex, const char* filename)
{
	if(filename == NULL ||
		resourceIndex < 0 ||
		resourceIndex >= m_pXex->numResources() )
		return false;
	
	FILE* fd = fopen(filename, "w+b");
	if(fd == NULL)
		return false;
	bool result = dump(resourceIndex, fd);
	fclose(fd);
	return result;
}

bool XexResourceDumper::dump(s32 resourceIndex, FILE* fd)
{
	if(	fd == NULL ||
		resourceIndex < 0 ||
		resourceIndex >= m_pXex->numResources() )
		return false;
	
	u8* res_data;
	s32 res_size;
	if( !dump(resourceIndex, res_data, res_size) )
		return false;
	
	fwrite(res_data, 1, res_size, fd);
	delete[] res_data;
	return true;
}

bool XexResourceDumper::dump(s32 resourceIndex, u8*& resData, s32& resSize)
{
	if(	resourceIndex < 0 ||
		resourceIndex >= m_pXex->numResources() )
		return false;
	
	resData = NULL;
	resSize = 0;
	
	// get resource details
	u32 res_addr;
	char res_name[260];
	if( !m_pXex->getResource(resourceIndex, res_addr, resSize, res_name) )
		return false;
	
	u32 base_addr = m_pXex->getLoadAddress();
	s32 res_offset = res_addr - base_addr;
	resData = new u8[resSize];
	
	// extract resource data
	DataBlock basefile;
	m_pXex->getBasefile(basefile);
	basefile.get(resData, res_offset, resSize);
	
	return true;
}

