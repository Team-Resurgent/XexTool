// 
// reads in xexs
// 

#include "XexReader.h"
#include "Xex.h"
#include "XexEndian.h"
#include "XexPacker.h"
#include "XexData.h"
#include "XexHeader.h"

XexReader::XexReader()
{
}

XexReader::~XexReader()
{
}


// read in xex
bool XexReader::read(Xex& xex, const char* filename)
{
	if(filename == NULL) return false;
	
	FILE* fd = fopen(filename, "rb");
	if(fd == NULL)
		return false;
	bool result = read(xex, fd);
	fclose(fd);
	return result;
}

bool XexReader::read(Xex& xex, FILE* fd)
{
	if(fd == NULL) return false;

	fseek(fd, 0, SEEK_END);
	int size = ftell(fd);
	fseek(fd, 0, SEEK_SET);
	u8* data = new u8[size];
	if( fread(data, 1, size, fd) != size )
	{
		delete[] data;
		return false;
	}
	bool result = read(xex, data, size);
	delete[] data;
	return result;
}

bool XexReader::read(Xex& xex, const void* data, int size)
{
	if(data == NULL || size < 0) return false;
	int offset = 0;
	u8* p_data = (u8*)data;
	
	xex.clear();
	
	// read in xex headers
	// first work out the size of all the headers
	// then read in that much data and pass it to the XexHeader object to parse
	XexImageHeader xex_image_header;
	memcpy(&xex_image_header, p_data+offset, sizeof(xex_image_header));
//	offset += sizeof(xex_image_header); 
	if( memcmp(xex_image_header.magic, XexData::XEX_MAGIC, 4) ) return false;
	
	s32 headers_size = GET32BE(&xex_image_header.sizeOfHeaders);
	DataBlock headers_block;
	headers_block.set(p_data+offset, 0, headers_size);
	offset += headers_size;
	
	XexHeader xex_headers;
	DataBlock basefile_format;
	s32 image_size;
	xex_headers.readHeaders(xex, headers_block, image_size, basefile_format);
	
	// read in basefile data
	s32 filesize = size;
	s32 basefile_size = filesize - headers_size;
	if(basefile_size < 0)
		return false;
	DataBlock basefile_block;
	basefile_block.set(p_data+offset, 0, basefile_size);
	
	// decrypt/unpack basefile
	DataBlock xex_basefile_block;
	XexKey xex_image_key;
	xex.getImageKey(xex_image_key);
	XexPacker packer;
	bool unpack_result = false;
	if( xex.isBinary() )
		unpack_result = packer.unpackBinary(xex_basefile_block, basefile_block,
						xex.isEncrypted(), xex_image_key, basefile_format, image_size);
	else if( xex.isRaw() )
		unpack_result = packer.unpackRaw(xex_basefile_block, basefile_block,
						xex.isEncrypted(), xex_image_key, basefile_format, image_size);
	else if( xex.isCompressed() )
		unpack_result = packer.unpackCompressed(xex_basefile_block, basefile_block,
						xex.isEncrypted(), xex_image_key, basefile_format, image_size);
	else if( xex.isDeltaCompressed() )
	{
//		unpack_result = packer.unpackDeltaCompressed(xex_basefile_block, basefile_block,
//						xex.isEncrypted(), xex_image_key, basefile_format, image_size);
		
		// store unaltered basefile data, as we can't decrypt this
		// unless we have data from the original xex file
		xex.setPatchData(basefile_block);
		xex.setPatchInfo(basefile_format);
		unpack_result = true;
	}
	
	return unpack_result &&
		xex.setBasefile(xex_basefile_block);
}

/*
void XexReader::convertXexHeaderFromBE(XexImageHeader& xex_header)
{
	xex_header.moduleFlags			= GET32BE(&xex_header.moduleFlags);
	xex_header.sizeOfHeaders		= GET32BE(&xex_header.sizeOfHeaders);
	xex_header.sizeOfDiscardableHeaders	= GET32BE(&xex_header.sizeOfDiscardableHeaders);
	xex_header.securityInfoOffset	= GET32BE(&xex_header.securityInfoOffset);
	xex_header.imageEntryCount= GET32BE(&xex_header.imageEntryCount);
}

void XexReader::setXexHeader(const XexImageHeader& xex_header)
{
	u32 mflags = xex_header.moduleFlags;
	xex.setTitleModule(		IS_FLAG_SET(mflags, MODULEFLAG_TITLE_MODULE) );
	xex.setTitleExports(	IS_FLAG_SET(mflags, MODULEFLAG_EXPORTS_TO_TITLE) );
	xex.setSystemDebugger(	IS_FLAG_SET(mflags, MODULEFLAG_SYSTEM_DEBUGGER) );
	xex.setDllModule(		IS_FLAG_SET(mflags, MODULEFLAG_DLL_MODULE) );
	xex.setPatchModule(		IS_FLAG_SET(mflags, MODULEFLAG_PATCH_MODULE) );
	xex.setPatchFull(		IS_FLAG_SET(mflags, MODULEFLAG_PATCH_FULL) );
	xex.setPatchDelta(		IS_FLAG_SET(mflags, MODULEFLAG_PATCH_DELTA) );
	xex.setUserMode(		IS_FLAG_SET(mflags, MODULEFLAG_USER_MODE) );
	xex.setUnknownModuleFlags(mflags);
	
	xex.setDiscardableHeaderSize( xex_header.sizeOfDiscardableHeaders );
}


// convert security info to system format from BE format
void XexReader::convertSecurityInfoFromBE(XexSecurityInfo& sec_info)
{
	sec_info.size					= GET32BE(&sec_info.size);
	sec_info.imageSize				= GET32BE(&sec_info.imageSize);
	sec_info.imageInfo.infoSize		= GET32BE(&sec_info.imageInfo.infoSize);
	sec_info.imageInfo.imageFlags	= GET32BE(&sec_info.imageInfo.imageFlags);
	sec_info.imageInfo.loadAddress	= GET32BE(&sec_info.imageInfo.loadAddress);
	sec_info.imageInfo.importTableCount	= GET32BE(&sec_info.imageInfo.importTableCount);
	sec_info.imageInfo.exportTableAddress= GET32BE(&sec_info.imageInfo.exportTableAddress);
	sec_info.imageInfo.gameRegion	= GET32BE(&sec_info.imageInfo.gameRegion);
	sec_info.allowedMediaTypes		= GET32BE(&sec_info.allowedMediaTypes);
	sec_info.sectionCount			= GET32BE(&sec_info.sectionCount);
}

void XexReader::setSecurityInfo(const XexSecurityInfo& sec_info)
{
	// check and assign data from security info
	u32 iflags = sec_info.imageInfo.imageFlags;
	xex.setManufacturingUtility(	IS_FLAG_SET(iflags, IMAGEFLAG_MANUFACTURING_UTILITY) );
	xex.setManufacturingSupportTool(IS_FLAG_SET(iflags, IMAGEFLAG_MANUFACTURING_TOOL) );
	xex.setXGD2Only(		IS_FLAG_SET(iflags, IMAGEFLAG_XGD2) );
	xex.setCardeaKey(		IS_FLAG_SET(iflags, IMAGEFLAG_CARDEA_KEY) );
	xex.setXeikaKey(		IS_FLAG_SET(iflags, IMAGEFLAG_XEIKA_KEY) );
	xex.setTitleUsermode(	IS_FLAG_SET(iflags, IMAGEFLAG_TITLE_USERMODE) );
	xex.setSystemUsermode(	IS_FLAG_SET(iflags, IMAGEFLAG_SYSTEM_USERMODE) );
	xex.setOrange0(			IS_FLAG_SET(iflags, IMAGEFLAG_ORANGE0) );
	xex.setOrange1(			IS_FLAG_SET(iflags, IMAGEFLAG_ORANGE1) );
	xex.setOrange2(			IS_FLAG_SET(iflags, IMAGEFLAG_ORANGE2) );
	xex.setTestkitRestricted(IS_FLAG_SET(iflags,IMAGEFLAG_TESTKIT_RESTRICTED) );
	xex.setIptvSignupApp(	IS_FLAG_SET(iflags, IMAGEFLAG_IPTV_SIGNUP_APP) );
	xex.setIptvTitleApp(	IS_FLAG_SET(iflags, IMAGEFLAG_IPTV_TITLE_APP) );
	if(IS_FLAG_SET(iflags, IMAGEFLAG_4K_PAGES)) xex.setPageSize4KB();
	else										xex.setPageSize64KB();
	xex.setNoGameRegion(	IS_FLAG_SET(iflags, IMAGEFLAG_NO_GAME_REGION) );
	xex.setOptionalRevocationCheck(IS_FLAG_SET(iflags, IMAGEFLAG_REVOCATION_CHECK_OPT) );
	xex.setRequiredRevocationCheck(IS_FLAG_SET(iflags, IMAGEFLAG_REVOCATION_CHECK_REQ) );
	xex.setUnknownImageFlags(iflags);
	xex.setImportTableCount(sec_info.imageInfo.importTableCount);
	xex.setMediaId(sec_info.imageInfo.mediaId);
	xex.setImageKey(sec_info.imageInfo.imageKey);
	xex.setExportTableAddress(sec_info.imageInfo.exportTableAddress);
	u32 rflags = sec_info.imageInfo.gameRegion;
	xex.setRegionNorthAmerica(	IS_FLAG_SET(rflags, REGION_NORTH_AMERICA) );
	xex.setRegionJapan(			IS_FLAG_SET(rflags, REGION_JAPAN) );
	xex.setRegionChina(			IS_FLAG_SET(rflags, REGION_CHINA) );
	xex.setRegionRestOfAsia(	IS_FLAG_SET(rflags, REGION_REST_OF_ASIA) );
	xex.setRegionAustNZ(		IS_FLAG_SET(rflags, REGION_AUST_NZ) );
	xex.setRegionRestOfEurope(	IS_FLAG_SET(rflags, REGION_REST_OF_EUROPE) );
	xex.setRegionRestOfWorld(	IS_FLAG_SET(rflags, REGION_REST_OF_WORLD) );
	u32 medflags = sec_info.allowedMediaTypes;
	xex.setMediaHardDisk(		IS_FLAG_SET(medflags, MEDIATYPE_HARD_DISK) );
	xex.setMediaDvdX2(			IS_FLAG_SET(medflags, MEDIATYPE_DVDX2) );
	xex.setMediaDvdCd(			IS_FLAG_SET(medflags, MEDIATYPE_DVD_CD) );
	xex.setMediaDvd5(			IS_FLAG_SET(medflags, MEDIATYPE_DVD5) );
	xex.setMediaDvd9(			IS_FLAG_SET(medflags, MEDIATYPE_DVD9) );
	xex.setMediaSystemFlash(	IS_FLAG_SET(medflags, MEDIATYPE_SYS_FLASH) );
	xex.setMediaMemoryUnit(		IS_FLAG_SET(medflags, MEDIATYPE_MEM_UNIT) );
	xex.setMediaMassStorage(	IS_FLAG_SET(medflags, MEDIATYPE_MASS_STORAGE) );
	xex.setMediaSMB(			IS_FLAG_SET(medflags, MEDIATYPE_SMB) );
	xex.setMediaRam(			IS_FLAG_SET(medflags, MEDIATYPE_RAM) );
	xex.setMediaRamDrive(		IS_FLAG_SET(medflags, MEDIATYPE_RAM_DRIVE) );
	xex.setMediaInsecurePackage(IS_FLAG_SET(medflags, MEDIATYPE_INSECURE_PKG) );
	xex.setMediaSavegamePackage(IS_FLAG_SET(medflags, MEDIATYPE_SAVEGAME_PKG) );
	xex.setMediaLocallySignedPackage(IS_FLAG_SET(medflags, MEDIATYPE_LOCALSIGN_PKG) );
	xex.setMediaLiveSignedPackage(IS_FLAG_SET(medflags, MEDIATYPE_LIVESIGN_PKG) );
	xex.setMediaXboxPackage(	IS_FLAG_SET(medflags, MEDIATYPE_XBOX_PKG) );
	xex.setUnknownMediaTypes(medflags);
	
	// non flag values
	xex.setLoadAddress(sec_info.imageInfo.loadAddress);
	xex.setImportTableCount(sec_info.imageInfo.importTableCount);
	xex.setMediaId(sec_info.imageInfo.mediaId);
}


void XexReader::convertSectionFromBE(XexHvSectionInfo& section)
{
	*(u32*)&section	= GET32BE(&section);
}

void XexReader::addSection(const XexHvSectionInfo& section)
{
	xex.addSection(section.size * xex.getPageSize(), section.info);
}


void XexReader::convertImageEntryHeaderFromBE(XexImageEntry& header)
{
	header.key	= GET32BE(&header.key);
	header.value= GET32BE(&header.value);
}

#define DATABLOCK_FROM_BE_16(data_block, offset) (data_block).set16((data_block).get16be(offset), (offset))
#define DATABLOCK_FROM_BE_32(data_block, offset) (data_block).set32((data_block).get32be(offset), (offset))
#define DATABLOCK_FROM_BE_64(data_block, offset) (data_block).set64((data_block).get64be(offset), (offset))

void XexReader::convertImageEntryDataFromBE(const XexImageEntry& header, DataBlock& data)
{
	if((header.key & 0xFF) == 0xFF)
		DATABLOCK_FROM_BE_32(data, 0);
	s32 info_size = data.get32(0);
	s32 num, type, offset;
	
	switch(header.key)
	{
	case 0x000002FF:		// IMAGEKEY_RESOURCE_SECTION
		num = (info_size - offsetof(ResourceDirectory, entry)) / sizeof(ResourceEntry);
		for(s32 i=0; i<num; i++)
		{
			s32 section_offset = offsetof(ResourceDirectory, entry) + i * sizeof(ResourceEntry);
			DATABLOCK_FROM_BE_32(data, section_offset + offsetof(ResourceEntry, addr));
			DATABLOCK_FROM_BE_32(data, section_offset + offsetof(ResourceEntry, size));
		}
		break;
	
	case 0x000003FF:		// IMAGEKEY_BASEFILE_FORMAT
		DATABLOCK_FROM_BE_16(data, offsetof(BaseFileInfoHeader, encType));
		DATABLOCK_FROM_BE_16(data, offsetof(BaseFileInfoHeader, compType));
		type = data.get16( offsetof(BaseFileInfoHeader, compType) );
		if(type == 1)
		{
			// raw
			s32 num_blocks = (info_size - offsetof(RawBaseFileInfo, block)) / sizeof(RawBaseFileBlock);
			for(s32 i=0; i<num_blocks; i++)
			{
				s32 block_offset = offsetof(RawBaseFileInfo, block) + i * sizeof(RawBaseFileBlock);
				DATABLOCK_FROM_BE_32(data, block_offset + offsetof(RawBaseFileBlock, dataSize));
				DATABLOCK_FROM_BE_32(data, block_offset + offsetof(RawBaseFileBlock, zeroSize));
			}
		}
		else if(type == 2 ||
				type == 3 )
		{
			// compressed and delta-compressed
			DATABLOCK_FROM_BE_32(data, offsetof(CompBaseFileInfo, compressionWindow));
			DATABLOCK_FROM_BE_32(data, offsetof(CompBaseFileInfo, block.dataSize));
		}
		break;
	
	case 0x00000405:		// IMAGEKEY_BASE_REFERENCE
		// no conversion required
		break;
	
	case 0x000005FF:		// IMAGEKEY_DELTA_PATCH_DESCRIPTOR
		DATABLOCK_FROM_BE_32(data, offsetof(DeltaPatchDescriptor, targetVersion));
		DATABLOCK_FROM_BE_32(data, offsetof(DeltaPatchDescriptor, sourceVersion));
		DATABLOCK_FROM_BE_32(data, offsetof(DeltaPatchDescriptor, targetHeaderSize));
		DATABLOCK_FROM_BE_32(data, offsetof(DeltaPatchDescriptor, deltaHeaderSourceOffset));
		DATABLOCK_FROM_BE_32(data, offsetof(DeltaPatchDescriptor, deltaHeaderSourceSize));
		DATABLOCK_FROM_BE_32(data, offsetof(DeltaPatchDescriptor, deltaHeaderTargetOffset));
		DATABLOCK_FROM_BE_32(data, offsetof(DeltaPatchDescriptor, deltaImageSourceOffset));
		DATABLOCK_FROM_BE_32(data, offsetof(DeltaPatchDescriptor, deltaImageSourceSize));
		DATABLOCK_FROM_BE_32(data, offsetof(DeltaPatchDescriptor, deltaImageTargetOffset));
		// after this is the patch data, this is left as is for the packer to use
		break;
	
	case 0x000080FF:		// IMAGEKEY_BOUND_PATHNAME
		// no conversion required
		break;
	
	case 0x000103FF:		// IMAGEKEY_IMPORT_LIBRARIES
		DATABLOCK_FROM_BE_32(data, offsetof(ImportLibraryDirectory, nameTableSize));
		DATABLOCK_FROM_BE_32(data, offsetof(ImportLibraryDirectory, numLibraries));
		num = data.get32(offsetof(ImportLibraryDirectory, numLibraries));
		offset = offsetof(ImportLibraryDirectory, names) + data.get32(offsetof(ImportLibraryDirectory, nameTableSize));
		offset = (offset + 3) & (-4);
		for(s32 i=0; i<num; i++)
		{
			DATABLOCK_FROM_BE_32(data, offset + offsetof(ImportLibraryEntry, infoSize));
			DATABLOCK_FROM_BE_32(data, offset + offsetof(ImportLibraryEntry, moduleNumber));
			DATABLOCK_FROM_BE_32(data, offset + offsetof(ImportLibraryEntry, version));
			DATABLOCK_FROM_BE_32(data, offset + offsetof(ImportLibraryEntry, minVersion));
			DATABLOCK_FROM_BE_16(data, offset + offsetof(ImportLibraryEntry, numAddresses));
			s32 num_addresses = data.get16(offset + offsetof(ImportLibraryEntry, numAddresses));
			for(s32 addr_num=0; addr_num<num_addresses; addr_num++)
			{
				DATABLOCK_FROM_BE_32(data, offset + offsetof(ImportLibraryEntry, addresses) + addr_num * 4);
			}
			
			offset += data.get32(offset + offsetof(ImportLibraryEntry, infoSize));
		}
		break;
	
	case 0x00018002:		// IMAGEKEY_IMAGE_CHECKSUM
		DATABLOCK_FROM_BE_32(data, offsetof(CheckSumTime, checksum));
		DATABLOCK_FROM_BE_32(data, offsetof(CheckSumTime, filetime));
		break;
	
	case 0x000183FF:		// IMAGEKEY_ORIGINAL_PE_NAME
		// no conversion required
		break;
	
	case 0x000200FF:		// IMAGEKEY_LIBRARY_VERSIONS
		num = (info_size - offsetof(StaticLibraryDirectory, entry)) / sizeof(StaticLibraryEntry);
		for(s32 i=0; i<num; i++)
		{
			offset = offsetof(StaticLibraryDirectory, entry) + i * sizeof(StaticLibraryEntry);
			DATABLOCK_FROM_BE_16(data, offset + offsetof(StaticLibraryEntry, version.major));
			DATABLOCK_FROM_BE_16(data, offset + offsetof(StaticLibraryEntry, version.minor));
			DATABLOCK_FROM_BE_16(data, offset + offsetof(StaticLibraryEntry, version.build));
		}
		break;
	
	case 0x00020104:		// IMAGEKEY_TLS_VALUES
		DATABLOCK_FROM_BE_32(data, offsetof(TLSInfo, numSlots));
		DATABLOCK_FROM_BE_32(data, offsetof(TLSInfo, dataSize));
		DATABLOCK_FROM_BE_32(data, offsetof(TLSInfo, rawAddress));
		DATABLOCK_FROM_BE_32(data, offsetof(TLSInfo, rawSize));
		break;
	
	case 0x00040006:		// IMAGEKEY_EXECUTION_ID
		DATABLOCK_FROM_BE_32(data, offsetof(ExecutionId, mediaId));
		DATABLOCK_FROM_BE_32(data, offsetof(ExecutionId, version));
		DATABLOCK_FROM_BE_32(data, offsetof(ExecutionId, baseVersion));
		DATABLOCK_FROM_BE_32(data, offsetof(ExecutionId, titleId));
		DATABLOCK_FROM_BE_32(data, offsetof(ExecutionId, saveGameId));
		break;
	
	case 0x00040310:		// IMAGEKEY_GAME_RATINGS
		// no conversion required
		break;
	
	case 0x00040404:		// IMAGEKEY_LAN_KEY
		// no conversion required
		break;
	
	case 0x000405FF:		// IMAGEKEY_LOGO_DATA
		DATABLOCK_FROM_BE_32(data, offsetof(LogoData, logoSize));
		break;
	
	case 0x000406FF:		// IMAGEKEY_MULTIDISC_MEDIA_IDS
		// no conversion required
		break;
	
	case 0x00E10402:		// IMAGEKEY_EXPORTS_BY_NAME
		DATABLOCK_FROM_BE_32(data, offsetof(ExportsByName, exportTableOffset));
		DATABLOCK_FROM_BE_32(data, offsetof(ExportsByName, exportTableSize));
		break;
	}
}

bool XexReader::addImageEntry(const XexImageEntry& header, const DataBlock& data)
{
	s32 info_size = data.get32(0);
	s32 num, offset, type;
	
	switch(header.key)
	{
	case 0x000002FF:		// IMAGEKEY_RESOURCE_SECTION
		num = (info_size - offsetof(ResourceDirectory, entry)) / sizeof(ResourceEntry);
		for(s32 i=0; i<num; i++)
		{
			offset = offsetof(ResourceDirectory, entry) + i * sizeof(ResourceEntry);
			ResourceEntry resource;
			data.get(&resource, offset, sizeof(resource));
			char name[9];
			strncpy(name, resource.name, 8);
			name[8] = 0;
			xex.addResource(resource.addr, resource.size, name);
		}
		break;
	
	case 0x000003FF:		// IMAGEKEY_BASEFILE_FORMAT
		type = data.get16(offsetof(BaseFileInfoHeader, encType));
		xex.setEncrypted( type == 1 );
		type = data.get16(offsetof(BaseFileInfoHeader, compType));
		if(		type == 1)	xex.setRaw();
		else if(type == 2)	xex.setCompressed();
		else if(type == 3)	xex.setDeltaCompressed();
		else				xex.setBinary();
		// store basefile info required to unpack file
		m_basefileFormat = data;
		break;
	
	case 0x00000405:		// IMAGEKEY_BASE_REFERENCE
	{
		u8 ref[20];
		data.get(ref, 0, 20);
		xex.setBaseReference(ref);
		break;
	}
	
	case 0x000005FF:		// IMAGEKEY_DELTA_PATCH_DESCRIPTOR
	{
		DataBlock patch_headers;
		data.get(patch_headers, 0, 0, data.size());
		xex.setDeltaPatchDescriptor(patch_headers);
		break;
	}
	
	case 0x000080FF:		// IMAGEKEY_BOUND_PATHNAME
	{
		s32 str_len = data.size() - offsetof(BoundPathname, pathname);
		char* bound_path = new char[str_len];
		data.get(bound_path, offsetof(BoundPathname, pathname), str_len);
		xex.setBoundingPath(bound_path);
		delete[] bound_path;
		break;
	}
	
	case 0x00010001:		// IMAGEKEY_ORIGINAL_BASE_ADDRESS
		// ignore this as it can be calculated from the basefile
		break;
	
	case 0x00010100:		// IMAGEKEY_ENTRY_POINT
		// ignore this as it can be calculated from the basefile
		break;
	
	case 0x00010201:		// IMAGEKEY_IMAGE_BASE_ADDRESS
		// ignore this as its a field in XexSecurityInfo
		break;
	
	case 0x000103FF:		// IMAGEKEY_IMPORT_LIBRARIES
	{
		s32 num_libs = data.get32(offsetof(ImportLibraryDirectory, numLibraries));
		offset = offsetof(ImportLibraryDirectory, names) + data.get32(offsetof(ImportLibraryDirectory, nameTableSize));
		char* names = new char[data.get32(offsetof(ImportLibraryDirectory, nameTableSize))];
		data.get(names, offsetof(ImportLibraryDirectory, names), data.get32(offsetof(ImportLibraryDirectory, nameTableSize)));
		for(s32 lib_num=0; lib_num<num_libs; lib_num++)
		{
			char* name = names;
			for(s32 i=0; i<lib_num; i++)
			{
				s32 len = ((s32)strlen(name) + 1 + 3) & (-4);
				name += len;
			}
			u32 module_number = data.get32(offset + offsetof(ImportLibraryEntry, moduleNumber));
			u8  module_index = data.get8(offset + offsetof(ImportLibraryEntry, moduleIndex));
			u32 ver = data.get32(offset + offsetof(ImportLibraryEntry, version));
			Version32 version = *(Version32*)&ver;
			ver = data.get32(offset + offsetof(ImportLibraryEntry, minVersion));
			Version32 min_version = *(Version32*)&ver;
			DataBlock addresses;
			s32 num_addresses = data.get16(offset + offsetof(ImportLibraryEntry, numAddresses));
			data.get(addresses, 0, offset + offsetof(ImportLibraryEntry, addresses), num_addresses * 4);
			xex.addImportLibrary(name, version, min_version, addresses, module_number, module_index);
			
			offset += data.get32(offset);
		}
		delete[] names;
		break;
	}
	
	case 0x00018002:		// IMAGEKEY_IMAGE_CHECKSUM
		// ignore this as it can be calculated from the basefile
		break;
	
	case 0x00018102:		// IMAGEKEY_IMAGE_CALLCAP
	{
		u32 addr1;	data.get(&addr1, 0, 4);
		u32 addr2;	data.get(&addr2, 4, 4);
		xex.setCallCap(addr1, addr2);
		break;
	}
	
	case 0x00018200:		// IMAGEKEY_IMAGE_FASTCAP
		xex.setFastCap(header.value);
		break;
	
	case 0x000183FF:		// IMAGEKEY_ORIGINAL_PE_NAME
	{
		s32 str_len = data.size() - offsetof(OriginalPEName, name);
		char* name = new char[str_len];
		data.get(name, offsetof(OriginalPEName, name), str_len);
		xex.setOriginalPEName(name);
		delete[] name;
		break;
	}
	
	case 0x000200FF:		// IMAGEKEY_LIBRARY_VERSIONS
	{
		s32 num_libs = (info_size - offsetof(StaticLibraryDirectory, entry)) / sizeof(StaticLibraryEntry);
		s32 offset = offsetof(StaticLibraryDirectory, entry);
		for(s32 i=0; i<num_libs; i++)
		{
			StaticLibraryEntry block;
			data.get(&block, offset + i * sizeof(block), sizeof(block));
			char name[9];
			strncpy(name, block.name, 9);
			name[8] = 0;
			xex.addLibraryVersion(name, block.version);
		}
		break;
	}
	
	case 0x00020104:		// IMAGEKEY_TLS_VALUES
	{
		TLSInfo tls;
		data.get(&tls, 0, sizeof(TLSInfo));
		xex.setTLSInfo(tls);
		break;
	}
	
	case 0x00020200:		// IMAGEKEY_STACK_SIZE
		xex.setStackSize(header.value);
		break;
	
	case 0x00020301:		// IMAGEKEY_FILESYSTEM_CACHE_SIZE
		xex.setFilesystemCacheSize(header.value);
		break;
	
	case 0x00020401:		// IMAGEKEY_HEAP_SIZE
		xex.setHeapSize(header.value);
		break;
	
	case 0x00030000:		// IMAGEKEY_SYSTEM_FLAGS
	{
		u32 sflags = header.value;
		xex.setNoForcedReboot(			IS_FLAG_SET(sflags, SYSFLAG_NO_FORCE_REBOOT) );
		xex.setForegroundTasks(			IS_FLAG_SET(sflags, SYSFLAG_FOREGROUND_TASKS) );
		xex.setNoODDMapping(			IS_FLAG_SET(sflags, SYSFLAG_NO_ODD_MAPPING) );
		xex.setMceInputHandler(			IS_FLAG_SET(sflags, SYSFLAG_HANDLE_MCE_INPUT) );
		xex.setRestrictedHudFeatures(	IS_FLAG_SET(sflags, SYSFLAG_RESTRICT_HUD_FEATURES) );
		xex.setGamepadDisconnectHandler(IS_FLAG_SET(sflags, SYSFLAG_HANDLE_GAMEPAD_DISCONNECT) );
		xex.setSecureSockets(			IS_FLAG_SET(sflags, SYSFLAG_INSECURE_SOCKETS) );
		xex.setXbox1Interoperability(	IS_FLAG_SET(sflags, SYSFLAG_XBOX_1_XSP_INTEROP) );
		xex.setDashContext(				IS_FLAG_SET(sflags, SYSFLAG_SET_DASH_CONTEXT) );
		xex.setGameVoiceChannelUser(	IS_FLAG_SET(sflags, SYSFLAG_USES_GAME_VOICE_CHANNEL) );
		xex.setPal50Incompatible(		IS_FLAG_SET(sflags, SYSFLAG_PAL50_INCOMPATIBLE) );
		xex.setInsecureUtilDriveUser(	IS_FLAG_SET(sflags, SYSFLAG_INSECURE_UTILITYDRIVE) );
		xex.setXamHooks(				IS_FLAG_SET(sflags, SYSFLAG_HAS_XAM_HOOKS) );
		xex.setPII(						IS_FLAG_SET(sflags, SYSFLAG_PII) );
		xex.setCrossPlatformSyslinkUser(IS_FLAG_SET(sflags, SYSFLAG_CROSSPLATFORM_SYSLINK) );
		xex.setMultidiscSwap(			IS_FLAG_SET(sflags, SYSFLAG_MULTIDISC_SWAP) );
		xex.setMultidiscInsecureMedia(	IS_FLAG_SET(sflags, SYSFLAG_MULTIDISC_INSECURE_MEDIA) );
		xex.setAP25Media(				IS_FLAG_SET(sflags, SYSFLAG_AP25_MEDIA) );
		xex.setNoCofirmExit(			IS_FLAG_SET(sflags, SYSFLAG_NO_CONFIRM_EXIT) );
		xex.setAllowBackgroundDownload(	IS_FLAG_SET(sflags, SYSFLAG_ALLOW_BKGRND_DOWNLOAD) );
		xex.setCreatePersistRamdrive(	IS_FLAG_SET(sflags, SYSFLAG_CREATE_PERSIST_RAMDRIVE) );
		xex.setInheritPersistRamdrive(	IS_FLAG_SET(sflags, SYSFLAG_INHERIT_PERSIST_RAMDRIVE) );
		xex.setAllowHudVibration(		IS_FLAG_SET(sflags, SYSFLAG_ALLOW_HUD_VIBRATION) );
		xex.setBothUtilityPartitions(	IS_FLAG_SET(sflags, SYSFLAG_BOTH_UTILITY_PARTITIONS) );
		xex.setUnknownSystemFlags(sflags);
		break;
	}
	
	case 0x00040006:		// IMAGEKEY_EXECUTION_ID
	{
		ExecutionId exec_id;
		data.get(&exec_id, 0, sizeof(ExecutionId));
		xex.setExecutionId(exec_id);
		break;
	}
	
	case 0x00040201:		// IMAGEKEY_TITLE_WORKSPACE_SIZE
		xex.setWorkspaceSize(header.value);
		break;
	
	case 0x00040310:		// IMAGEKEY_GAME_RATINGS
	{
		GameRatings ratings;
		data.get(&ratings, 0, sizeof(GameRatings));
		xex.setGameRatings(ratings);
		break;
	}
	
	case 0x00040404:		// IMAGEKEY_LAN_KEY
	{
		LANKey lan_key;
		data.get(&lan_key, 0, sizeof(LANKey));
		xex.setLANKey(lan_key);
		break;
	}
	
	case 0x000405FF:		// IMAGEKEY_LOGO_DATA
	{
		DataBlock logo_data;
//		data.get(logo_data, 0, offsetof(LogoData, logoSize), info_size -  offsetof(LogoData, logoSize));
		data.get(logo_data, 0, offsetof(LogoData, logoData), info_size -  offsetof(LogoData, logoData));
		xex.setLogoData(logo_data);
		break;
	}
	
	case 0x000406FF:		// IMAGEKEY_MULTIDISC_MEDIA_IDS
	{
		s32 num_media_ids = (info_size - offsetof(MultidiscMediaIds, mediaId)) / sizeof(MediaId);
		for(s32 i=0; i<num_media_ids; i++)
		{
			MediaId media_id;
			data.get(&media_id, offsetof(MultidiscMediaIds, mediaId) + i * sizeof(MediaId), sizeof(MediaId));
			xex.addMultidiscMediaId(media_id);
		}
		break;
	}
	
	case 0x00E10402:		// IMAGEKEY_EXPORTS_BY_NAME
	{
		ExportsByName exports;
		data.get(&exports, 0, sizeof(ExportsByName));
		xex.setExportsByName(exports);
		break;
	}
	
	default:
		xex.addUnknownImageEntry(header, data);
		break;
	}
	
	return true;
}



bool XexReader::encKey(XexKey& dataKey, const XexKey& cryptKey)
{
	XECRYPT_AES_STATE aes_ctx;
	XeCryptAesKey(&aes_ctx, cryptKey.data);
	XeCryptAesEcb(&aes_ctx, dataKey.data, dataKey.data, TRUE);
	return true;
}
bool XexReader::encRetailKey(XexKey& dataKey)
{
	XexKey retail_key;
	memcpy(retail_key.data, XexData::XEX_RETAIL_KEY, sizeof(XexKey));
	return encKey(dataKey, retail_key);
}
bool XexReader::encDebugKey(XexKey& dataKey)
{
	XexKey debug_key;
	memcpy(debug_key.data, XexData::XEX_DEBUG_KEY, sizeof(XexKey));
	return encKey(dataKey, debug_key);
}
bool XexReader::encMfgRetailKey(XexKey& dataKey)
{
	XexKey mfg_retail_key;
	memcpy(mfg_retail_key.data, XexData::XEX_MFG_RETAIL_KEY, sizeof(XexKey));
	return encKey(dataKey, mfg_retail_key);
}
bool XexReader::encMfgDebugKey(XexKey& dataKey)
{
	XexKey mfg_debug_key;
	memcpy(mfg_debug_key.data, XexData::XEX_MFG_DEBUG_KEY, sizeof(XexKey));
	return encKey(dataKey, mfg_debug_key);
}

bool XexReader::decKey(XexKey& dataKey, const XexKey& cryptKey)
{
	XECRYPT_AES_STATE aes_ctx;
	XeCryptAesKey(&aes_ctx, cryptKey.data);
	XeCryptAesEcb(&aes_ctx, dataKey.data, dataKey.data, FALSE);
	return true;
}
bool XexReader::decRetailKey(XexKey& dataKey)
{
	XexKey retail_key;
	memcpy(retail_key.data, XexData::XEX_RETAIL_KEY, sizeof(XexKey));
	return decKey(dataKey, retail_key);
}
bool XexReader::decDebugKey(XexKey& dataKey)
{
	XexKey debug_key;
	memcpy(debug_key.data, XexData::XEX_DEBUG_KEY, sizeof(XexKey));
	return decKey(dataKey, debug_key);
}
bool XexReader::decMfgRetailKey(XexKey& dataKey)
{
	XexKey mfg_retail_key;
	memcpy(mfg_retail_key.data, XexData::XEX_MFG_RETAIL_KEY, sizeof(XexKey));
	return decKey(dataKey, mfg_retail_key);
}
bool XexReader::decMfgDebugKey(XexKey& dataKey)
{
	XexKey mfg_debug_key;
	memcpy(mfg_debug_key.data, XexData::XEX_MFG_DEBUG_KEY, sizeof(XexKey));
	return decKey(dataKey, mfg_debug_key);
}

// checks that signature is valid
bool XexReader::verifySign(const u8* publicKey, const XexSecurityInfo& secInfo)
{
	const u8* sig_ptr = secInfo.imageInfo.signature;
	s32 sig_size = 0x100;
	s32 hash_size = GET32BE(&secInfo.imageInfo.infoSize) - sig_size;
	u8 hash[20];
	XeCryptRotSumSha(sig_ptr+sig_size, hash_size,
		0, 0,
		hash, 20);
	
	// since verifySign is being called before the security options have
	// been set on the xex - we cant use these settings from the xex
	// object itself. we need to check them from the passed in security info.
	const u8* salt;
//	if( xex.hasRequiredRevocationCheck() )
	u32 image_flags = GET32BE(&secInfo.imageInfo.imageFlags);
	if( IS_FLAG_SET(image_flags, IMAGEFLAG_REVOCATION_CHECK_REQ) )
		salt = XexData::XEX_SALT_REV;
	else
		salt = XexData::XEX_SALT_XEX;
	
	if( !XeCryptBnQwBeSigVerify((u64*)sig_ptr, hash, salt, (XECRYPT_RSA*)publicKey) )
		return false;
	return true;
}
*/

