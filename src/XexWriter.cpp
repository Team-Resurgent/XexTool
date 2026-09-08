// 
// writes out xexs
// 

#include "XexWriter.h"
#include "Xex.h"
#include "XexEndian.h"
#include "XexPacker.h"
#include "XexData.h"
#include "XexHeader.h"

//#define SET_FLAG(toggle, flags, flag)	(toggle) ? ((flags) |= (flag)) : ((flags) &= (~(flag)))
//#define IS_FLAG_SET(flags, flag)		(((flags) & (flag)) != 0)


XexWriter::XexWriter()
{
}

XexWriter::~XexWriter()
{
}


// write out xex
bool XexWriter::write(const Xex& xex, const char* filename)
{
	if(filename == NULL) return false;
	
	FILE* fd = fopen(filename, "w+b");
	if(fd == NULL)
		return false;
	bool result = write(xex, fd);
	fclose(fd);
	return result;
}

bool XexWriter::write(const Xex& xex, FILE* fd)
{
	if(fd == NULL) return false;
	
	void* data = NULL;
	int size = 0;
	if( fseek(fd, 0, SEEK_SET) != 0 ) { return false; }
	// the cast here produced an rvalue, which cannot bind to the void*&
	// parameter; pass the variable itself
	if( !write(xex, data, size) )
		return false;
	bool ok = ((int)fwrite(data, 1, size, fd) == size);
	// the buffer is allocated as bytes, so delete it as bytes rather than
	// through a void*, which is undefined
	delete[] (u8*)data;
	return ok;
}

// if successful, 'data' needs to be freed
bool XexWriter::write(const Xex& xex, void*& data, int& size)
{
	// encrypt/pack basefile
	DataBlock basefile_format;
	DataBlock xex_basefile_block, basefile_block;
	xex.getBasefile(xex_basefile_block);
	XexKey xex_image_key;
	xex.getImageKey(xex_image_key);
	XexPacker packer;
	bool pack_result = false;
	if( xex.isDeltaCompressed() )
	{
//		pack_result = packer.packDeltaCompressed(basefile_block, xex_basefile_block,
//						xex.isEncrypted(), xex_image_key, basefile_format, xex.getImageSize());
		xex.getPatchData(basefile_block);
		u16 enc_type = (xex.isEncrypted()) ? 1 : 0;
		DataBlock patch_data;
		xex.getPatchData(patch_data);
		s32 data_size = patch_data.size();
// what is this bit for again?! :P
//		if(data_size > 0x10000)
//			data_size = 0x10000;
		basefile_format.clear();
		basefile_format.set32(sizeof(CompBaseFileInfo), offsetof(CompBaseFileInfo, infoSize));
		basefile_format.set16(enc_type, offsetof(CompBaseFileInfo, encType));
		basefile_format.set16(3, offsetof(CompBaseFileInfo, compType));
		basefile_format.set32(0x8000, offsetof(CompBaseFileInfo, compressionWindow));
		basefile_format.set32(data_size, offsetof(CompBaseFileInfo, block.dataSize));
		basefile_format.fill(0, offsetof(CompBaseFileInfo, block.hash), sizeof(XexHash));
		pack_result = true;
	}
	else if(xex_basefile_block.size() == 0)
	{
// what is this bit for again?! :P
		basefile_block = xex_basefile_block;
		u16 enc_type = (xex.isEncrypted()) ? 1 : 0;
		u16 comp_type;
		if( xex.isBinary() ||
			xex.isRaw() )
			comp_type = 1;
		else if( xex.isCompressed() )
			comp_type = 2;
		else if( xex.isDeltaCompressed() )
			comp_type = 3;
		basefile_format.clear();
		basefile_format.set32(sizeof(RawBaseFileInfo), offsetof(RawBaseFileInfo, infoSize));
		basefile_format.set16(enc_type, offsetof(RawBaseFileInfo, encType));
		basefile_format.set16(comp_type, offsetof(RawBaseFileInfo, compType));
		basefile_format.set32(0, offsetof(RawBaseFileInfo, block) + 0);
		basefile_format.set32(0, offsetof(RawBaseFileInfo, block) + 4);
		pack_result = true;
	}
	else if( xex.isBinary() )
		pack_result = packer.packBinary(basefile_block, xex_basefile_block,
						xex.isEncrypted(), xex_image_key, basefile_format, xex.getImageSize());
	else if( xex.isRaw() )
		pack_result = packer.packRaw(basefile_block, xex_basefile_block,
						xex.isEncrypted(), xex_image_key, basefile_format, xex.getImageSize());
	else if( xex.isCompressed() )
		pack_result = packer.packCompressed(basefile_block, xex_basefile_block,
						xex.isEncrypted(), xex_image_key, basefile_format, xex.getImageSize());
	if( !pack_result )
		return false;
	
	// set xex headers from an xex
	XexHeader xex_headers;
	DataBlock xex_header_data;
	xex_headers.writeHeaders(xex, xex_header_data, xex.getImageSize(), basefile_format);
	
/*	
	// prepare image entries
	s32 num_image_entries = getNumOptionalInfos();
	s32 optional_info_size = getImageEntryDataSize();
	s32 sec_info_offset = sizeof(XexImageHeader) + num_optional_info * sizeof(XexImageEntry);
	if( !xex.isPatchModule() )
		sec_info_offset += 0x80;
	s32 sections_offset = sec_info_offset + sizeof(XexSecurityInfo);
	s32 optional_info_offset = sections_offset + xex.numSections() * sizeof(XexHvSectionInfo);
	s32 basefile_offset = optional_info_offset + optional_info_size;
	if( xex.isPatchModule() )
		basefile_offset = (basefile_offset + 0x800-1) & (-0x800);
	else
		basefile_offset = (basefile_offset + 0x1000-1) & (-0x1000);
	s32 headers_size = basefile_offset;
	u8* headers_data = new u8[headers_size];
	memset(headers_data, 0, headers_size);
	
	// prepare xex header
	XexImageHeader* xex_header = (XexImageHeader*)headers_data;
	getXexHeader(*xex_header, basefile_offset, sec_info_offset, num_optional_info);
	convertXexHeaderToBE(*xex_header);
	
	// prepare security info
	XexSecurityInfo* sec_info = (XexSecurityInfo*)(headers_data + sec_info_offset);
	getSecurityInfo(*sec_info);
	convertSecurityInfoToBE(*sec_info);
	
	// prepare sections
	XexHvSectionInfo* sections = (XexHvSectionInfo*)(headers_data + sections_offset);
	for(s32 sec_num=0; sec_num<xex.numSections(); sec_num++)
	{
		getSection(sec_num, sections[sec_num]);
		convertSectionToBE(sections[sec_num]);
	}
	
	// prepare optional info
	XexImageEntry* optional_header_ptr = (XexImageEntry*)(headers_data + sizeof(XexImageHeader));
	getImageEntry(optional_header_ptr, headers_data + optional_info_offset, headers_size - optional_info_offset, optional_info_offset);
	
	// make sure hashes and signatures are valid
	if( !updateImportHash(headers_data) ||
		!updateSectionHashes(sections, *sec_info) ||
		!updateHeaderHash(headers_data, headers_size, sec_info_offset) )
	{
		delete[] headers_data;
		return false;
	}
	if( xex.isDebug() )
		updateDebugSign(*sec_info);
	else
		updateRetailSign(*sec_info);
*/	
	// allocate buffers
	s32 header_size = xex_header_data.size();
	s32 basefile_size = basefile_block.size();
	s32 basefile_pad_size = 0;
	if( xex.isRaw() )
		basefile_pad_size = ((basefile_size + 0x7FFF) & (-0x8000)) - basefile_size;
	size = header_size + basefile_size + basefile_pad_size;
	data = (void*)new u8[size];
	u8* p_data = (u8*)data;
	s32 offset = 0;
	
	// write out all headers
	xex_header_data.get(p_data+offset, 0, header_size);
	offset += header_size;
	
	// write out basefile data
	if(basefile_size == 0)
		return true;
	basefile_block.fill(0, basefile_size, basefile_pad_size);
	basefile_block.get(p_data+offset, 0, basefile_size+basefile_pad_size);
	return true;
}

/*
void XexWriter::convertXexHeaderToBE(XexImageHeader& xexHeader)
{
	SET32BE(&xexHeader.moduleFlags,				xexHeader.moduleFlags);
	SET32BE(&xexHeader.sizeOfHeaders,			xexHeader.sizeOfHeaders);
	SET32BE(&xexHeader.sizeOfDiscardableHeaders,xexHeader.sizeOfDiscardableHeaders);
	SET32BE(&xexHeader.securityInfoOffset,		xexHeader.securityInfoOffset);
	SET32BE(&xexHeader.imageEntryCount,	xexHeader.imageEntryCount);
}
void XexWriter::getXexHeader(XexImageHeader& xexHeader, s32 basefileOffset, s32 securityInfoOffset, s32 numOptionalInfo)
{
	memcpy(xexHeader.magic, XexData::XEX_MAGIC, 4);
	u32 mflags = 0;
	SET_FLAG(xex.isTitleModule(),	mflags, MODULEFLAG_TITLE_MODULE);
	SET_FLAG(xex.isTitleExports(),	mflags, MODULEFLAG_EXPORTS_TO_TITLE);
	SET_FLAG(xex.isSystemDebugger(),mflags, MODULEFLAG_SYSTEM_DEBUGGER);
	SET_FLAG(xex.isDllModule(),		mflags, MODULEFLAG_DLL_MODULE);
	SET_FLAG(xex.isPatchModule(),	mflags, MODULEFLAG_PATCH_MODULE);
	SET_FLAG(xex.isPatchFull(),		mflags, MODULEFLAG_PATCH_FULL);
	SET_FLAG(xex.isPatchDelta(),	mflags, MODULEFLAG_PATCH_DELTA);
	SET_FLAG(xex.isUserMode(),		mflags, MODULEFLAG_USER_MODE);
	mflags |= xex.getUnknownModuleFlags();
	xexHeader.moduleFlags = mflags;
	xexHeader.sizeOfHeaders = basefileOffset;
	xexHeader.sizeOfDiscardableHeaders = xex.getDiscardableHeaderSize();
	xexHeader.securityInfoOffset = securityInfoOffset;
	xexHeader.imageEntryCount = numOptionalInfo;
}


// convert security info in system format to BE format
void XexWriter::convertSecurityInfoToBE(XexSecurityInfo& secInfo)
{
	SET32BE(&secInfo.size,						secInfo.size);
	SET32BE(&secInfo.imageSize,					secInfo.imageSize);
	SET32BE(&secInfo.imageInfo.infoSize,		secInfo.imageInfo.infoSize);
	SET32BE(&secInfo.imageInfo.imageFlags,		secInfo.imageInfo.imageFlags);
	SET32BE(&secInfo.imageInfo.loadAddress,		secInfo.imageInfo.loadAddress);
	SET32BE(&secInfo.imageInfo.importTableCount,secInfo.imageInfo.importTableCount);
	SET32BE(&secInfo.imageInfo.exportTableAddress,secInfo.imageInfo.exportTableAddress);
	SET32BE(&secInfo.imageInfo.gameRegion,		secInfo.imageInfo.gameRegion);
	SET32BE(&secInfo.allowedMediaTypes,			secInfo.allowedMediaTypes);
	SET32BE(&secInfo.sectionCount,				secInfo.sectionCount);
}
void XexWriter::getSecurityInfo(XexSecurityInfo& secInfo)
{
	// size of security data
	secInfo.size = sizeof(XexSecurityInfo) + xex.numSections() * sizeof(XexHvSectionInfo);
	secInfo.imageSize = xex.getImageSize();
	memset(secInfo.imageInfo.signature, 0, sizeof(secInfo.imageInfo.signature));
	secInfo.imageInfo.infoSize = offsetof(XexSecurityInfo, allowedMediaTypes) - (offsetof(XexSecurityInfo, imageInfo)+offsetof(XexHvImageInfo, signature));
	u32 iflags = 0;
	SET_FLAG(xex.isManufacturingUtility(),		iflags, IMAGEFLAG_MANUFACTURING_UTILITY);
	SET_FLAG(xex.isManufacturingSupportTool(),	iflags, IMAGEFLAG_MANUFACTURING_TOOL);
	SET_FLAG(xex.isXGD2Only(),			iflags, IMAGEFLAG_XGD2);
	SET_FLAG(xex.isCardeaKey(),			iflags, IMAGEFLAG_CARDEA_KEY);
	SET_FLAG(xex.isXeikaKey(),			iflags, IMAGEFLAG_XEIKA_KEY);
	SET_FLAG(xex.isTitleUsermode(),		iflags, IMAGEFLAG_TITLE_USERMODE);
	SET_FLAG(xex.isSystemUsermode(),	iflags, IMAGEFLAG_SYSTEM_USERMODE);
	SET_FLAG(xex.isOrange0(),			iflags, IMAGEFLAG_ORANGE0);
	SET_FLAG(xex.isOrange1(),			iflags, IMAGEFLAG_ORANGE1);
	SET_FLAG(xex.isOrange2(),			iflags, IMAGEFLAG_ORANGE2);
	SET_FLAG(xex.isTestkitRestricted(),	iflags, IMAGEFLAG_TESTKIT_RESTRICTED);
	SET_FLAG(xex.isIptvSignupApp(),		iflags, IMAGEFLAG_IPTV_SIGNUP_APP);
	SET_FLAG(xex.isIptvTitleApp(),		iflags, IMAGEFLAG_IPTV_TITLE_APP);
	SET_FLAG(xex.isPageSize4KB(),		iflags, IMAGEFLAG_4K_PAGES);
	SET_FLAG(xex.isNoGameRegion(),		iflags, IMAGEFLAG_NO_GAME_REGION);
	SET_FLAG(xex.hasOptionalRevocationCheck(),	iflags, IMAGEFLAG_REVOCATION_CHECK_OPT);
	SET_FLAG(xex.hasRequiredRevocationCheck(),	iflags, IMAGEFLAG_REVOCATION_CHECK_REQ);
	iflags |= xex.getUnknownImageFlags();
	secInfo.imageInfo.imageFlags = iflags;
	secInfo.imageInfo.loadAddress = xex.getLoadAddress();
	secInfo.imageInfo.importTableCount = xex.getImportTableCount();
	xex.getMediaId(secInfo.imageInfo.mediaId);
	xex.getImageKey(secInfo.imageInfo.imageKey);
	if( xex.isManufacturingSupportTool() || xex.isManufacturingUtility() )
	{
		if( xex.isDebug() )
			encMfgDebugKey(secInfo.imageInfo.imageKey);
		else
			encMfgRetailKey(secInfo.imageInfo.imageKey);
	}
	else
	{
		if( xex.isDebug() )
			encDebugKey(secInfo.imageInfo.imageKey);
		else
			encRetailKey(secInfo.imageInfo.imageKey);
	}
	secInfo.imageInfo.exportTableAddress = xex.getExportTableAddress();
	u32 region_flags = 0;
	SET_FLAG(xex.isRegionNorthAmerica(),region_flags, REGION_NORTH_AMERICA);
	SET_FLAG(xex.isRegionJapan(),		region_flags, REGION_JAPAN);
	SET_FLAG(xex.isRegionChina(),		region_flags, REGION_CHINA);
	SET_FLAG(xex.isRegionRestOfAsia(),	region_flags, REGION_REST_OF_ASIA);
	SET_FLAG(xex.isRegionAustNZ(),		region_flags, REGION_AUST_NZ);
	SET_FLAG(xex.isRegionRestOfEurope(),region_flags, REGION_REST_OF_EUROPE);
	SET_FLAG(xex.isRegionRestOfWorld(),	region_flags, REGION_REST_OF_WORLD);
	secInfo.imageInfo.gameRegion = region_flags;
	u32 media_flags = 0;
	SET_FLAG(xex.isMediaHardDisk(),		media_flags, MEDIATYPE_HARD_DISK);
	SET_FLAG(xex.isMediaDvdX2(),		media_flags, MEDIATYPE_DVDX2);
	SET_FLAG(xex.isMediaDvdCd(),		media_flags, MEDIATYPE_DVD_CD);
	SET_FLAG(xex.isMediaDvd5(),			media_flags, MEDIATYPE_DVD5);
	SET_FLAG(xex.isMediaDvd9(),			media_flags, MEDIATYPE_DVD9);
	SET_FLAG(xex.isMediaSystemFlash(),	media_flags, MEDIATYPE_SYS_FLASH);
	SET_FLAG(xex.isMediaMemoryUnit(),	media_flags, MEDIATYPE_MEM_UNIT);
	SET_FLAG(xex.isMediaMassStorage(),	media_flags, MEDIATYPE_MASS_STORAGE);
	SET_FLAG(xex.isMediaSMB(),			media_flags, MEDIATYPE_SMB);
	SET_FLAG(xex.isMediaRam(),			media_flags, MEDIATYPE_RAM);
	SET_FLAG(xex.isMediaRamDrive(),		media_flags, MEDIATYPE_RAM_DRIVE);
	SET_FLAG(xex.isMediaInsecurePackage(),media_flags, MEDIATYPE_INSECURE_PKG);
	SET_FLAG(xex.isMediaSavegamePackage(),media_flags, MEDIATYPE_SAVEGAME_PKG);
	SET_FLAG(xex.isMediaLocallySignedPackage(),media_flags, MEDIATYPE_LOCALSIGN_PKG);
	SET_FLAG(xex.isMediaLiveSignedPackage(),media_flags, MEDIATYPE_LIVESIGN_PKG);
	SET_FLAG(xex.isMediaXboxPackage(),	media_flags, MEDIATYPE_XBOX_PKG);
	media_flags |= xex.getUnknownMediaTypes();
	secInfo.allowedMediaTypes = media_flags;
	secInfo.sectionCount = xex.numSections();
}


void XexWriter::convertSectionToBE(XexHvSectionInfo& section)
{
//	SET32BE(&section.flags,	section.flags);
	SET32BE(&section,	*(u32*)&section);
}
bool XexWriter::getSection(s32 index, XexHvSectionInfo& section)
{
	if(index < 0 || index >= xex.numSections())
		return false;
	s32 size;
	u8 info;
	xex.getSection(index, size, info);
	section.info = info;
	section.size = size / xex.getPageSize();
	return true;
}


s32 XexWriter::getNumOptionalInfos()
{
	s32 num_opt_info = 0;
	// IMAGEKEY_RESOURCE_SECTION			0x000002FF
	if(xex.numResources()) num_opt_info++;
	// IMAGEKEY_BASEFILE_FORMAT			0x000003FF
	num_opt_info++;
	// IMAGEKEY_BASE_REFERENCE				0x00000405
	if(xex.hasBaseReference()) num_opt_info++;
	// IMAGEKEY_DELTA_PATCH_DESCRIPTOR		0x000005FF
	if(xex.hasDeltaPatchDescriptor()) num_opt_info++;
	// IMAGEKEY_BOUND_PATHNAME				0x000080FF
	if(xex.hasBoundingPath()) num_opt_info++;
	// IMAGEKEY_ORIGINAL_BASE_ADDRESS		0x00010001
	if(xex.hasOrignalLoadAddress()) num_opt_info++;
	// IMAGEKEY_ENTRY_POINT				0x00010100
	if(xex.hasEntryPoint()) num_opt_info++;
	// IMAGEKEY_IMAGE_BASE_ADDRESS			0x00010201
	if(xex.isBasefilePE()) num_opt_info++;
	// IMAGEKEY_IMPORT_LIBRARIES			0x000103FF
	if(xex.numImportLibraries()) num_opt_info++;
	// IMAGEKEY_IMAGE_CHECKSUM				0x00018002
	if(xex.hasChecksum()) num_opt_info++;
	// IMAGEKEY_IMAGE_CALLCAP				0x00018102
	if(xex.hasCallCap()) num_opt_info++;
	// IMAGEKEY_IMAGE_FASTCAP				0x00018200
	if(xex.hasFastCap()) num_opt_info++;
	// IMAGEKEY_ORIGINAL_PE_NAME			0x000183FF
	if(xex.hasOriginalPEName()) num_opt_info++;
	// IMAGEKEY_LIBRARY_VERSIONS			0x000200FF
	if(xex.numLibraryVersions()) num_opt_info++;
	// IMAGEKEY_TLS_VALUES					0x00020104
	if(xex.hasTLSInfo()) num_opt_info++;
	// IMAGEKEY_STACK_SIZE					0x00020200
	if(xex.hasStackSize()) num_opt_info++;
	// IMAGEKEY_FILESYSTEM_CACHE_SIZE		0x00020301
	if(xex.hasFilesystemCacheSize()) num_opt_info++;
	// IMAGEKEY_HEAP_SIZE					0x00020401
	if(xex.hasHeapSize()) num_opt_info++;
	// IMAGEKEY_SYSTEM_FLAGS				0x00030000
	if(xex.hasSystemFlags()) num_opt_info++;
	// IMAGEKEY_EXECUTION_ID				0x00040006
	if(xex.hasExecutionId()) num_opt_info++;
	// IMAGEKEY_TITLE_WORKSPACE_SIZE		0x00040201
	if(xex.hasWorkspaceSize()) num_opt_info++;
	// IMAGEKEY_GAME_RATINGS				0x00040310
	if(xex.hasGameRatings()) num_opt_info++;
	// IMAGEKEY_LAN_KEY					0x00040404
	if(xex.hasLANKey()) num_opt_info++;
	// IMAGEKEY_LOGO_DATA					0x000405FF
	if(xex.hasLogoData()) num_opt_info++;
	// IMAGEKEY_MULTIDISC_MEDIA_IDS		0x000406FF
	if(xex.numMultidiscMediaIds()) num_opt_info++;
	// IMAGEKEY_EXPORTS_BY_NAME			0x00E10402
	if(xex.hasExportsByName()) num_opt_info++;
	
	num_opt_info += xex.numUnknownImageEntries();
	return num_opt_info;
}
s32 XexWriter::getImageEntryDataSize()
{
	const s32 buffer_size = 256;
	u8 buffer[buffer_size];
	s32 opt_info_size = 0;
	
	// IMAGEKEY_RESOURCE_SECTION			0x000002FF
	if(xex.numResources()) opt_info_size += 4 + (xex.numResources() * sizeof(ResourceEntry));
	// IMAGEKEY_BASEFILE_FORMAT			0x000003FF
	opt_info_size += basefile_format.size();
	// IMAGEKEY_BASE_REFERENCE				0x00000405
	if(xex.hasBaseReference()) opt_info_size += sizeof(BaseReference);
	// IMAGEKEY_DELTA_PATCH_DESCRIPTOR		0x000005FF
	if(xex.hasDeltaPatchDescriptor()) { DataBlock patch; xex.getDeltaPatchDescriptor(patch); opt_info_size += patch.size(); }
	// IMAGEKEY_BOUND_PATHNAME				0x000080FF
	if(xex.hasBoundingPath()) { xex.getBoundingPath((char*)buffer, buffer_size); opt_info_size += 4 + (((s32)strlen((char*)buffer) + 4) & (-4)); }
	// IMAGEKEY_ORIGINAL_BASE_ADDRESS		0x00010001
		// data is stored in header
	// IMAGEKEY_ENTRY_POINT				0x00010100
		// data is stored in header
	// IMAGEKEY_IMAGE_BASE_ADDRESS			0x00010201
		// data is stored in header
	// IMAGEKEY_IMPORT_LIBRARIES			0x000103FF
	if(xex.numImportLibraries())
	{
		// add import libraries header
		opt_info_size += 12;
		for(s32 i=0; i<xex.numImportLibraries(); i++)
		{
			DataBlock addresses;
			u32 module_number;
			u8  module_index;
			Version32 version;
			Version32 min_version;
			if( xex.getImportLibrary(i, (char*)buffer, version, min_version, addresses, module_number, module_index) )
			{
				// add import libraries name sizes
				opt_info_size += ((s32)strlen((char*)buffer) + 4) & (-4);
				opt_info_size += sizeof(ImportLibraryEntry) - 4 + addresses.size();
			}
		}
	}
	// IMAGEKEY_IMAGE_CHECKSUM				0x00018002
	if(xex.hasChecksum()) opt_info_size += sizeof(CheckSumTime);
	// IMAGEKEY_IMAGE_CALLCAP				0x00018102
	if(xex.hasCallCap()) opt_info_size += sizeof(CallCap);
	// IMAGEKEY_IMAGE_FASTCAP				0x00018200
		// data is stored in header
	// IMAGEKEY_ORIGINAL_PE_NAME			0x000183FF
	if(xex.hasOriginalPEName()) { xex.getOriginalPEName((char*)buffer, buffer_size); opt_info_size += 4 + (((s32)strlen((char*)buffer) + 4) & (-4)); }
	// IMAGEKEY_LIBRARY_VERSIONS			0x000200FF
	if(xex.numLibraryVersions()) opt_info_size += sizeof(StaticLibraryDirectory) + (xex.numLibraryVersions()-1) * sizeof(StaticLibraryEntry);
	// IMAGEKEY_TLS_VALUES					0x00020104
	if(xex.hasTLSInfo()) opt_info_size += sizeof(TLSInfo);
	// IMAGEKEY_STACK_SIZE					0x00020200
		// data is stored in header
	// IMAGEKEY_FILESYSTEM_CACHE_SIZE		0x00020301
		// data is stored in header
	// IMAGEKEY_HEAP_SIZE					0x00020401
		// data is stored in header
	// IMAGEKEY_SYSTEM_FLAGS				0x00030000
		// data is stored in header
	// IMAGEKEY_EXECUTION_ID				0x00040006
	if(xex.hasExecutionId()) opt_info_size += sizeof(ExecutionId);
	// IMAGEKEY_TITLE_WORKSPACE_SIZE		0x00040201
		// data is stored in header
	// IMAGEKEY_GAME_RATINGS				0x00040310
	if(xex.hasGameRatings()) opt_info_size += sizeof(GameRatings);
	// IMAGEKEY_LAN_KEY					0x00040404
	if(xex.hasLANKey()) opt_info_size += sizeof(LANKey);
	// IMAGEKEY_LOGO_DATA					0x000405FF
	if(xex.hasLogoData()) { DataBlock logo_data; xex.getLogoData(logo_data); opt_info_size += logo_data.size() + 8; }
	// IMAGEKEY_MULTIDISC_MEDIA_IDS		0x000406FF
	if(xex.numMultidiscMediaIds()) opt_info_size += 4 + xex.numMultidiscMediaIds() * sizeof(MediaId);
	// IMAGEKEY_EXPORTS_BY_NAME			0x00E10402
	if(xex.hasExportsByName()) opt_info_size += sizeof(ExportsByName);
	
	for(s32 i=0; i<xex.numUnknownImageEntries(); i++)
	{
		XexImageEntry header;
		DataBlock data;
		if( xex.getUnknownImageEntry(i, header, data) )
		{
			opt_info_size += data.size();
		}
	}
	return opt_info_size;
}
void XexWriter::getImageEntry(XexImageEntry* headers, u8* data, s32 dataSize, s32 dataOffset)
{
	// insert all optional info in order of id
	XexImageEntry info_header;
	XexImageEntry* headers_ptr = headers;
	u32 info_data_offset = 0;
	
	// IMAGEKEY_RESOURCE_SECTION			0x000002FF
	if(xex.numResources())
	{
		DataBlock resources;
		for(s32 res_num=0; res_num<xex.numResources(); res_num++)
		{
			u32 addr;
			s32 size;
			char name[12];
			memset(name, 0, 12);
			if( xex.getResource(res_num, addr, size, name) )
			{
				s32 res_offset = 4 + res_num * sizeof(ResourceEntry);
				resources.set(name,   res_offset + offsetof(ResourceEntry, name), 8);
				resources.set32(addr, res_offset + offsetof(ResourceEntry, addr));
				resources.set32(size, res_offset + offsetof(ResourceEntry, size));
			}
		}
		resources.set32(resources.size(), 0);
		// fix and set values
		info_header.key = IMAGEKEY_RESOURCE_SECTION;
		info_header.value = dataOffset + info_data_offset;
		convertImageEntryDataToBE(info_header, resources);
		convertImageEntryHeaderToBE(info_header);
		*headers_ptr++ = info_header;
		resources.get(data + info_data_offset, 0, resources.size());
		info_data_offset += resources.size();
	}
	// IMAGEKEY_BASEFILE_FORMAT			0x000003FF
	{
		DataBlock format = basefile_format;
		format.set32(format.size(), offsetof(BaseFileInfoHeader, infoSize));
		u16 type;
		(xex.isEncrypted()) ? type = 1 : type = 0;
		format.set16(type, offsetof(BaseFileInfoHeader, encType));
		type = 0;
		if(		xex.isBinary())				type = 1;
		else if(xex.isRaw())				type = 1;
		else if(xex.isCompressed())			type = 2;
		else if(xex.isDeltaCompressed())	type = 3;
		format.set16(type, offsetof(BaseFileInfoHeader, compType));
		// fix and set values
		info_header.key = IMAGEKEY_BASEFILE_FORMAT;
		info_header.value = dataOffset + info_data_offset;
		convertImageEntryDataToBE(info_header, format);
		convertImageEntryHeaderToBE(info_header);
		*headers_ptr++ = info_header;
		format.get(data + info_data_offset, 0, format.size());
		info_data_offset += format.size();
	}
	// IMAGEKEY_BASE_REFERENCE				0x00000405
	if(xex.hasBaseReference())
	{
		DataBlock base_ref;
		u8 ref[20];
		xex.getBaseReference(ref);
		base_ref.set(ref, 0, 20);
		// fix and set values
		info_header.key = IMAGEKEY_BASE_REFERENCE;
		info_header.value = dataOffset + info_data_offset;
		convertImageEntryDataToBE(info_header, base_ref);
		convertImageEntryHeaderToBE(info_header);
		*headers_ptr++ = info_header;
		base_ref.get(data + info_data_offset, 0, base_ref.size());
		info_data_offset += base_ref.size();
	}
	// IMAGEKEY_DELTA_PATCH_DESCRIPTOR		0x000005FF
	if(xex.hasDeltaPatchDescriptor())
	{
		DataBlock patch;
		xex.getDeltaPatchDescriptor(patch);
		// fix and set values
		info_header.key = IMAGEKEY_DELTA_PATCH_DESCRIPTOR;
		info_header.value = dataOffset + info_data_offset;
		convertImageEntryDataToBE(info_header, patch);
		convertImageEntryHeaderToBE(info_header);
		*headers_ptr++ = info_header;
		patch.get(data + info_data_offset, 0, patch.size());
		info_data_offset += patch.size();
	}
	// IMAGEKEY_BOUND_PATHNAME				0x000080FF
	if(xex.hasBoundingPath())
	{
		DataBlock bound_path;
		const s32 bound_path_str_size = 1024;
		char* bound_path_str = new char[bound_path_str_size];
		memset(bound_path_str, 0, bound_path_str_size);
		xex.getBoundingPath(bound_path_str, bound_path_str_size);
		bound_path.set(bound_path_str, 4, ((s32)strlen(bound_path_str) + 4) & (-4));
		bound_path.set32(bound_path.size(), 0);
		// fix and set values
		info_header.key = IMAGEKEY_BOUND_PATHNAME;
		info_header.value = dataOffset + info_data_offset;
		convertImageEntryDataToBE(info_header, bound_path);
		convertImageEntryHeaderToBE(info_header);
		*headers_ptr++ = info_header;
		bound_path.get(data + info_data_offset, 0, bound_path.size());
		info_data_offset += bound_path.size();
	}
	// IMAGEKEY_ORIGINAL_BASE_ADDRESS		0x00010001
	if(xex.hasOrignalLoadAddress())
	{
		info_header.key = IMAGEKEY_ORIGINAL_BASE_ADDRESS;
		info_header.value = xex.getOrignalLoadAddress();
		convertImageEntryHeaderToBE(info_header);
		*headers_ptr++ = info_header;
	}
	// IMAGEKEY_ENTRY_POINT				0x00010100
	if(xex.hasEntryPoint())
	{
		info_header.key = IMAGEKEY_ENTRY_POINT;
		info_header.value = xex.getEntryPoint();
		convertImageEntryHeaderToBE(info_header);
		*headers_ptr++ = info_header;
	}
	// IMAGEKEY_IMAGE_BASE_ADDRESS			0x00010201
	if(xex.isBasefilePE())
	{
		info_header.key = IMAGEKEY_IMAGE_BASE_ADDRESS;
		info_header.value = xex.getLoadAddress();
		convertImageEntryHeaderToBE(info_header);
		*headers_ptr++ = info_header;
	}
	// IMAGEKEY_IMPORT_LIBRARIES			0x000103FF
	if(xex.numImportLibraries())
	{
		DataBlock import_libs;
		const s32 names_size = 1024;
		char* names = new char[names_size];
		char* name_ptr = names;
		memset(names, 0, names_size);
		Version32 version;
		Version32 min_version;
		DataBlock addresses;
		u32 module_number;
		u8  module_index;
		
		// we need to work out the name size first
		// we'll also get all the names here
		s32 name_size = 0;
		for(s32 lib_num=0; lib_num<xex.numImportLibraries(); lib_num++)
		{
			if( xex.getImportLibrary(lib_num, name_ptr, version, min_version, addresses, module_number, module_index) )
			{
				// add import libraries name sizes
				name_size += ((s32)strlen(name_ptr) + 4) & (-4);
				name_ptr = names + name_size;
			}
		}
		
		// do the import libraries header
		import_libs.set32(0, offsetof(ImportLibraryDirectory, infoSize));	// fill in the actual size last
		import_libs.set32(name_size, offsetof(ImportLibraryDirectory, nameTableSize));
		import_libs.set32(xex.numImportLibraries(), offsetof(ImportLibraryDirectory, numLibraries));
		import_libs.set(names, offsetof(ImportLibraryDirectory, names), name_size);
		
		// now do each import library
		s32 lib_offset = 12 + name_size;
		for(s32 lib_num=0; lib_num<xex.numImportLibraries(); lib_num++)
		{
			if( !xex.getImportLibrary(lib_num, names, version, min_version, addresses, module_number, module_index) )
				break;
			s32 num_addresses = addresses.size() / 4;
			s32 lib_size = sizeof(ImportLibraryEntry) - 4 + addresses.size();
			import_libs.set32(lib_size,				lib_offset + offsetof(ImportLibraryEntry, infoSize));
			import_libs.set32(module_number,		lib_offset + offsetof(ImportLibraryEntry, moduleNumber));
			import_libs.set32(*(u32*)&version,		lib_offset + offsetof(ImportLibraryEntry, version));
			import_libs.set32(*(u32*)&min_version,	lib_offset + offsetof(ImportLibraryEntry, minVersion));
			import_libs.set8( module_index,			lib_offset + offsetof(ImportLibraryEntry, moduleIndex));
			import_libs.set16(num_addresses,		lib_offset + offsetof(ImportLibraryEntry, numAddresses));
			
			for(s32 addr_num=0; addr_num<num_addresses; addr_num++)
				import_libs.set32(addresses.get32(addr_num * 4), lib_offset + offsetof(ImportLibraryEntry, addresses) + addr_num * 4);
			
			lib_offset += lib_size;
		}
		import_libs.set32(import_libs.size(), offsetof(ImportLibraryDirectory, infoSize));	// fill in the actual size last
		delete[] names;
		
		// fix endian and output data
		s32 import_lib_offset = dataSize - import_libs.size();
		info_header.key = IMAGEKEY_IMPORT_LIBRARIES;
		info_header.value = dataOffset + import_lib_offset;
		convertImageEntryDataToBE(info_header, import_libs);
		convertImageEntryHeaderToBE(info_header);
		*headers_ptr++ = info_header;
		import_libs.get(data + import_lib_offset, 0, import_libs.size());
	}

	// IMAGEKEY_IMAGE_CHECKSUM				0x00018002
	if(xex.hasChecksum())
	{
		DataBlock checksum;
		checksum.set32(xex.getChecksum(), 0);
		checksum.set32(xex.getFiletime(), 4);
		// fix and set values
		info_header.key = IMAGEKEY_IMAGE_CHECKSUM;
		info_header.value = dataOffset + info_data_offset;
		convertImageEntryDataToBE(info_header, checksum);
		convertImageEntryHeaderToBE(info_header);
		*headers_ptr++ = info_header;
		checksum.get(data + info_data_offset, 0, checksum.size());
		info_data_offset += checksum.size();
	}
	// IMAGEKEY_IMAGE_CALLCAP				0x00018102
	if(xex.hasCallCap())
	{
		u32 addr1, addr2;
		DataBlock callcap;
		xex.getCallCap(addr1, addr2);
		callcap.set32(addr1, 0);
		callcap.set32(addr2, 4);
		// fix and set values
		info_header.key = IMAGEKEY_IMAGE_CALLCAP;
		info_header.value = dataOffset + info_data_offset;
		convertImageEntryDataToBE(info_header, callcap);
		convertImageEntryHeaderToBE(info_header);
		*headers_ptr++ = info_header;
		callcap.get(data + info_data_offset, 0, callcap.size());
		info_data_offset += callcap.size();
	}
	// IMAGEKEY_IMAGE_FASTCAP				0x00018200
	if(xex.hasFastCap())
	{
		u32 fastcap;
		xex.getFastCap(fastcap);
		info_header.key = IMAGEKEY_IMAGE_FASTCAP;
		info_header.value = fastcap;
		convertImageEntryHeaderToBE(info_header);
		*headers_ptr++ = info_header;
	}
	// IMAGEKEY_ORIGINAL_PE_NAME			0x000183FF
	if(xex.hasOriginalPEName())
	{
		DataBlock name;
		const s32 name_str_size = 1024;
		char* name_str = new char[name_str_size];
		memset(name_str, 0, name_str_size);
		xex.getOriginalPEName(name_str, name_str_size);
		name.set(name_str, 4, ((s32)strlen(name_str) + 4) & (-4));
		name.set32(name.size(), 0);
		delete[] name_str;
		// fix and set values
		info_header.key = IMAGEKEY_ORIGINAL_PE_NAME;
		info_header.value = dataOffset + info_data_offset;
		convertImageEntryDataToBE(info_header, name);
		convertImageEntryHeaderToBE(info_header);
		*headers_ptr++ = info_header;
		name.get(data + info_data_offset, 0, name.size());
		info_data_offset += name.size();
	}
	// IMAGEKEY_LIBRARY_VERSIONS			0x000200FF
	if(xex.numLibraryVersions())
	{
		DataBlock lib_ver;
		for(s32 lib_num=0; lib_num<xex.numLibraryVersions(); lib_num++)
		{
			char name[12];
			memset(name, 0, 12);
			VersionInfo version;
			if( xex.getLibraryVersion(lib_num, name, version) )
			{
				s32 offset = 4 + lib_num * sizeof(StaticLibraryEntry);
				lib_ver.set(name, offset + offsetof(StaticLibraryEntry, name), 8);
				lib_ver.set64(*(u64*)&version, offset + offsetof(StaticLibraryEntry, version));
			}
		}
		lib_ver.set32(lib_ver.size(), 0);
		// fix and set values
		info_header.key = IMAGEKEY_LIBRARY_VERSIONS;
		info_header.value = dataOffset + info_data_offset;
		convertImageEntryDataToBE(info_header, lib_ver);
		convertImageEntryHeaderToBE(info_header);
		*headers_ptr++ = info_header;
		lib_ver.get(data + info_data_offset, 0, lib_ver.size());
		info_data_offset += lib_ver.size();
	}
	// IMAGEKEY_TLS_VALUES					0x00020104
	if(xex.hasTLSInfo())
	{
		TLSInfo tls_info;
		DataBlock tls;
		xex.getTLSInfo(tls_info);
		tls.set(&tls_info, 0, sizeof(tls_info));
		// fix and set values
		info_header.key = IMAGEKEY_TLS_VALUES;
		info_header.value = dataOffset + info_data_offset;
		convertImageEntryDataToBE(info_header, tls);
		convertImageEntryHeaderToBE(info_header);
		*headers_ptr++ = info_header;
		tls.get(data + info_data_offset, 0, tls.size());
		info_data_offset += tls.size();
	}
	// IMAGEKEY_STACK_SIZE					0x00020200
	if(xex.hasStackSize())
	{
		s32 stacksize;
		xex.getStackSize(stacksize);
		info_header.key = IMAGEKEY_STACK_SIZE;
		info_header.value = stacksize;
		convertImageEntryHeaderToBE(info_header);
		*headers_ptr++ = info_header;
	}
	// IMAGEKEY_FILESYSTEM_CACHE_SIZE		0x00020301
	if(xex.hasFilesystemCacheSize())
	{
		s32 size;
		xex.getFilesystemCacheSize(size);
		info_header.key = IMAGEKEY_FILESYSTEM_CACHE_SIZE;
		info_header.value = size;
		convertImageEntryHeaderToBE(info_header);
		*headers_ptr++ = info_header;
	}
	// IMAGEKEY_HEAP_SIZE					0x00020401
	if(xex.hasHeapSize())
	{
		s32 size;
		xex.getHeapSize(size);
		info_header.key = IMAGEKEY_HEAP_SIZE;
		info_header.value = size;
		convertImageEntryHeaderToBE(info_header);
		*headers_ptr++ = info_header;
	}
	// IMAGEKEY_SYSTEM_FLAGS				0x00030000
	if(xex.hasSystemFlags())
	{
		u32 sys_flags = 0;
		SET_FLAG(xex.isNoForcedReboot(),			sys_flags, SYSFLAG_NO_FORCE_REBOOT);
		SET_FLAG(xex.isForegroundTasks(),			sys_flags, SYSFLAG_FOREGROUND_TASKS);
		SET_FLAG(xex.isNoODDMapping(),				sys_flags, SYSFLAG_NO_ODD_MAPPING);
		SET_FLAG(xex.isMceInputHandler(),			sys_flags, SYSFLAG_HANDLE_MCE_INPUT);
		SET_FLAG(xex.isRestrictedHudFeatures(),	sys_flags, SYSFLAG_RESTRICT_HUD_FEATURES);
		SET_FLAG(xex.isGamepadDisconnectHandler(),	sys_flags, SYSFLAG_HANDLE_GAMEPAD_DISCONNECT);
		SET_FLAG(xex.isSecureSockets(),			sys_flags, SYSFLAG_INSECURE_SOCKETS);
		SET_FLAG(xex.isXbox1Interoperability(),	sys_flags, SYSFLAG_XBOX_1_XSP_INTEROP);
		SET_FLAG(xex.isDashContext(),				sys_flags, SYSFLAG_SET_DASH_CONTEXT);
		SET_FLAG(xex.isGameVoiceChannelUser(),		sys_flags, SYSFLAG_USES_GAME_VOICE_CHANNEL);
		SET_FLAG(xex.isPal50Incompatible(),			sys_flags, SYSFLAG_PAL50_INCOMPATIBLE);
		SET_FLAG(xex.isInsecureUtilDriveUser(),		sys_flags, SYSFLAG_INSECURE_UTILITYDRIVE);
		SET_FLAG(xex.isXamHooks(),					sys_flags, SYSFLAG_HAS_XAM_HOOKS);
		SET_FLAG(xex.isPII(),						sys_flags, SYSFLAG_PII);
		SET_FLAG(xex.isCrossPlatformSyslinkUser(),	sys_flags, SYSFLAG_CROSSPLATFORM_SYSLINK);
		SET_FLAG(xex.isMultidiscSwap(),			sys_flags, SYSFLAG_MULTIDISC_SWAP);
		SET_FLAG(xex.isMultidiscInsecureMedia(),	sys_flags, SYSFLAG_MULTIDISC_INSECURE_MEDIA);
		SET_FLAG(xex.isAP25Media(),					sys_flags, SYSFLAG_AP25_MEDIA);
		SET_FLAG(xex.isNoCofirmExit(),				sys_flags, SYSFLAG_NO_CONFIRM_EXIT);
		SET_FLAG(xex.isAllowBackgroundDownload(),	sys_flags, SYSFLAG_ALLOW_BKGRND_DOWNLOAD);
		SET_FLAG(xex.isCreatePersistRamdrive(),		sys_flags, SYSFLAG_CREATE_PERSIST_RAMDRIVE);
		SET_FLAG(xex.isInheritPersistRamdrive(),	sys_flags, SYSFLAG_INHERIT_PERSIST_RAMDRIVE);
		SET_FLAG(xex.isAllowHudVibration(),			sys_flags, SYSFLAG_ALLOW_HUD_VIBRATION);
		SET_FLAG(xex.isBothUtilityPartitions(),		sys_flags, SYSFLAG_BOTH_UTILITY_PARTITIONS);
		sys_flags |= xex.getUnknownSystemFlags();
		// fix and set values
		info_header.key = IMAGEKEY_SYSTEM_FLAGS;
		info_header.value = sys_flags;
		convertImageEntryHeaderToBE(info_header);
		*headers_ptr++ = info_header;
	}
	// IMAGEKEY_EXECUTION_ID				0x00040006
	if(xex.hasExecutionId())
	{
		ExecutionId exec_id;
		DataBlock exec;
		xex.getExecutionId(exec_id);
		exec.set(&exec_id, 0, sizeof(exec_id));
		// fix and set values
		info_header.key = IMAGEKEY_EXECUTION_ID;
		info_header.value = dataOffset + info_data_offset;
		convertImageEntryDataToBE(info_header, exec);
		convertImageEntryHeaderToBE(info_header);
		*headers_ptr++ = info_header;
		exec.get(data + info_data_offset, 0, exec.size());
		info_data_offset += exec.size();
	}
	// IMAGEKEY_TITLE_WORKSPACE_SIZE		0x00040201
	if(xex.hasWorkspaceSize())
	{
		s32 size;
		xex.getWorkspaceSize(size);
		info_header.key = IMAGEKEY_TITLE_WORKSPACE_SIZE;
		info_header.value = size;
		convertImageEntryHeaderToBE(info_header);
		*headers_ptr++ = info_header;
	}
	// IMAGEKEY_GAME_RATINGS				0x00040310
	if(xex.hasGameRatings())
	{
		GameRatings game_ratings;
		DataBlock ratings;
		xex.getGameRatings(game_ratings);
		ratings.set(&game_ratings, 0, sizeof(game_ratings));
		// fix and set values
		info_header.key = IMAGEKEY_GAME_RATINGS;
		info_header.value = dataOffset + info_data_offset;
		convertImageEntryDataToBE(info_header, ratings);
		convertImageEntryHeaderToBE(info_header);
		*headers_ptr++ = info_header;
		ratings.get(data + info_data_offset, 0, ratings.size());
		info_data_offset += ratings.size();
	}
	// IMAGEKEY_LAN_KEY					0x00040404
	if(xex.hasLANKey())
	{
		LANKey lan_key;
		DataBlock lan;
		xex.getLANKey(lan_key);
		lan.set(&lan_key, 0, sizeof(lan_key));
		// fix and set values
		info_header.key = IMAGEKEY_LAN_KEY;
		info_header.value = dataOffset + info_data_offset;
		convertImageEntryDataToBE(info_header, lan);
		convertImageEntryHeaderToBE(info_header);
		*headers_ptr++ = info_header;
		lan.get(data + info_data_offset, 0, lan.size());
		info_data_offset += lan.size();
	}
	// IMAGEKEY_LOGO_DATA					0x000405FF
	if(xex.hasLogoData())
	{
		// get logo pixel data
		DataBlock pixel_data;
		xex.getLogoData(pixel_data);
		// setup logo info
		DataBlock logo_info;
		logo_info.set32(pixel_data.size()+8, 0);
		logo_info.set32(pixel_data.size(), 4);
		logo_info.set(pixel_data, 0, 8, pixel_data.size());
		// fix and set values
		info_header.key = IMAGEKEY_LOGO_DATA;
		info_header.value = dataOffset + info_data_offset;
		convertImageEntryDataToBE(info_header, logo_info);
		convertImageEntryHeaderToBE(info_header);
		*headers_ptr++ = info_header;
		logo_info.get(data + info_data_offset, 0, logo_info.size());
		info_data_offset += logo_info.size();
	}
	// IMAGEKEY_MULTIDISC_MEDIA_IDS		0x000406FF
	if(xex.numMultidiscMediaIds())
	{
		DataBlock ids;
		for(s32 id_num=0; id_num<xex.numMultidiscMediaIds(); id_num++)
		{
			s32 offset = 4 + id_num * sizeof(MediaId);
			MediaId media_id;
			xex.getMultidiscMediaId(id_num, media_id);
			ids.set(&media_id, offset, sizeof(media_id));
		}
		ids.set32(ids.size(), 0);
		// fix and set values
		info_header.key = IMAGEKEY_MULTIDISC_MEDIA_IDS;
		info_header.value = dataOffset + info_data_offset;
		convertImageEntryDataToBE(info_header, ids);
		convertImageEntryHeaderToBE(info_header);
		*headers_ptr++ = info_header;
		ids.get(data + info_data_offset, 0, ids.size());
		info_data_offset += ids.size();
	}
	// IMAGEKEY_EXPORTS_BY_NAME			0x00E10402
	if(xex.hasExportsByName())
	{
		ExportsByName exports_by_name;
		DataBlock exports;
		xex.getExportsByName(exports_by_name);
		exports.set(&exports_by_name, 0, sizeof(exports_by_name));
		// fix and set values
		info_header.key = IMAGEKEY_EXPORTS_BY_NAME;
		info_header.value = dataOffset + info_data_offset;
		convertImageEntryDataToBE(info_header, exports);
		convertImageEntryHeaderToBE(info_header);
		*headers_ptr++ = info_header;
		exports.get(data + info_data_offset, 0, exports.size());
		info_data_offset += exports.size();
	}
	
	// insert unknown optional info ids
	for(s32 i=0; i<xex.numUnknownImageEntries(); i++)
	{
		XexImageEntry info_header;
		DataBlock info_data;
		if( xex.getUnknownImageEntry(i, info_header, info_data) )
		{
			// check if has any extra data
			// if so then the offset needs to be updated
			if( IS_IMAGEENTRY_DATA_OFFSET(info_header) )
			{
				info_header.value = dataOffset + info_data_offset;
			}
			convertImageEntryDataToBE(info_header, info_data);
			convertImageEntryHeaderToBE(info_header);
			*headers_ptr++ = info_header;
			info_data.get(data + info_data_offset, 0, info_data.size());
			info_data_offset += info_data.size();
		}
	}
}
void XexWriter::convertImageEntryHeaderToBE(XexImageEntry& header)
{
	SET32BE(&header.key, header.key);
	SET32BE(&header.value, header.value);
}
#define DATABLOCK_TO_BE_16(data_block, offset) (data_block).set16be((data_block).get16(offset), (offset))
#define DATABLOCK_TO_BE_32(data_block, offset) (data_block).set32be((data_block).get32(offset), (offset))
#define DATABLOCK_TO_BE_64(data_block, offset) (data_block).set64be((data_block).get64(offset), (offset))
void XexWriter::convertImageEntryDataToBE(const XexImageEntry& header, DataBlock& data)
{
	s32 info_size = data.get32(0);
	if((header.key & 0xFF) == 0xFF)
		DATABLOCK_TO_BE_32(data, 0);
	s32 num, type, offset;
	
	switch(header.key)
	{
	case 0x000002FF:		// IMAGEKEY_RESOURCE_SECTION
		num = (info_size - offsetof(ResourceDirectory, entry)) / sizeof(ResourceEntry);
		for(s32 i=0; i<num; i++)
		{
			s32 section_offset = offsetof(ResourceDirectory, entry) + i * sizeof(ResourceEntry);
			DATABLOCK_TO_BE_32(data, section_offset + offsetof(ResourceEntry, addr));
			DATABLOCK_TO_BE_32(data, section_offset + offsetof(ResourceEntry, size));
		}
		break;
	
	case 0x000003FF:		// IMAGEKEY_BASEFILE_INFO
		type = data.get16( offsetof(BaseFileInfoHeader, compType) );
		DATABLOCK_TO_BE_16(data, offsetof(BaseFileInfoHeader, encType));
		DATABLOCK_TO_BE_16(data, offsetof(BaseFileInfoHeader, compType));
		if(type == 1)
		{
			// raw
			s32 num_blocks = (info_size - offsetof(RawBaseFileInfo, block)) / sizeof(RawBaseFileBlock);
			for(s32 i=0; i<num_blocks; i++)
			{
				s32 block_offset = offsetof(RawBaseFileInfo, block) + i * sizeof(RawBaseFileBlock);
				DATABLOCK_TO_BE_32(data, block_offset + offsetof(RawBaseFileBlock, dataSize));
				DATABLOCK_TO_BE_32(data, block_offset + offsetof(RawBaseFileBlock, zeroSize));
			}
		}
		else if(type == 2 ||
				type == 3 )
		{
			// compressed and delta-compressed
			DATABLOCK_TO_BE_32(data, offsetof(CompBaseFileInfo, compressionWindow));
			DATABLOCK_TO_BE_32(data, offsetof(CompBaseFileInfo, block.dataSize));
		}
		break;
	
	case 0x00000405:		// IMAGEKEY_BASE_REFERENCE
		// no conversion required
		break;
	
	case 0x000005FF:		// IMAGEKEY_DELTA_PATCH_DESCRIPTOR
		DATABLOCK_TO_BE_32(data, offsetof(DeltaPatchDescriptor, targetVersion));
		DATABLOCK_TO_BE_32(data, offsetof(DeltaPatchDescriptor, sourceVersion));
		DATABLOCK_TO_BE_32(data, offsetof(DeltaPatchDescriptor, targetHeaderSize));
		DATABLOCK_TO_BE_32(data, offsetof(DeltaPatchDescriptor, deltaHeaderSourceOffset));
		DATABLOCK_TO_BE_32(data, offsetof(DeltaPatchDescriptor, deltaHeaderSourceSize));
		DATABLOCK_TO_BE_32(data, offsetof(DeltaPatchDescriptor, deltaHeaderTargetOffset));
		DATABLOCK_TO_BE_32(data, offsetof(DeltaPatchDescriptor, deltaImageSourceOffset));
		DATABLOCK_TO_BE_32(data, offsetof(DeltaPatchDescriptor, deltaImageSourceSize));
		DATABLOCK_TO_BE_32(data, offsetof(DeltaPatchDescriptor, deltaImageTargetOffset));
		// after this is the patch data, this is left as is for the packer to use
		break;
	
	case 0x000080FF:		// IMAGEKEY_BOUND_PATHNAME
		// no conversion required
		break;
	
	case 0x000103FF:		// IMAGEKEY_IMPORT_LIBRARIES
		num = data.get32(offsetof(ImportLibraryDirectory, numLibraries));
		offset = offsetof(ImportLibraryDirectory, names) + data.get32(offsetof(ImportLibraryDirectory, nameTableSize));
		offset = (offset + 3) & (-4);
		DATABLOCK_TO_BE_32(data, offsetof(ImportLibraryDirectory, nameTableSize));
		DATABLOCK_TO_BE_32(data, offsetof(ImportLibraryDirectory, numLibraries));
		for(s32 i=0; i<num; i++)
		{
			s32 num_addresses = data.get16(offset + offsetof(ImportLibraryEntry, numAddresses));
			s32 lib_size = data.get32(offset + offsetof(ImportLibraryEntry, infoSize));
			DATABLOCK_TO_BE_32(data, offset + offsetof(ImportLibraryEntry, infoSize));
			DATABLOCK_TO_BE_32(data, offset + offsetof(ImportLibraryEntry, moduleNumber));
			DATABLOCK_TO_BE_32(data, offset + offsetof(ImportLibraryEntry, version));
			DATABLOCK_TO_BE_32(data, offset + offsetof(ImportLibraryEntry, minVersion));
			DATABLOCK_TO_BE_16(data, offset + offsetof(ImportLibraryEntry, numAddresses));
			for(s32 addr_num=0; addr_num<num_addresses; addr_num++)
			{
				DATABLOCK_TO_BE_32(data, offset + offsetof(ImportLibraryEntry, addresses) + addr_num * 4);
			}
			offset += lib_size;
		}
		break;
	
	case 0x00018002:		// IMAGEKEY_IMAGE_CHECKSUM
		DATABLOCK_TO_BE_32(data, offsetof(CheckSumTime, checksum));
		DATABLOCK_TO_BE_32(data, offsetof(CheckSumTime, filetime));
		break;
	
	case 0x000183FF:		// IMAGEKEY_ORIGINAL_PE_NAME
		// no conversion required
		break;
	
	case 0x000200FF:		// IMAGEKEY_LIBRARY_VERSIONS
		num = (info_size - offsetof(StaticLibraryDirectory, entry)) / sizeof(StaticLibraryEntry);
		for(s32 i=0; i<num; i++)
		{
			offset = offsetof(StaticLibraryDirectory, entry) + i * sizeof(StaticLibraryEntry);
			DATABLOCK_TO_BE_16(data, offset + offsetof(StaticLibraryEntry, version.major));
			DATABLOCK_TO_BE_16(data, offset + offsetof(StaticLibraryEntry, version.minor));
			DATABLOCK_TO_BE_16(data, offset + offsetof(StaticLibraryEntry, version.build));
		}
		break;
	
	case 0x00020104:		// IMAGEKEY_TLS_VALUES
		DATABLOCK_TO_BE_32(data, offsetof(TLSInfo, numSlots));
		DATABLOCK_TO_BE_32(data, offsetof(TLSInfo, dataSize));
		DATABLOCK_TO_BE_32(data, offsetof(TLSInfo, rawAddress));
		DATABLOCK_TO_BE_32(data, offsetof(TLSInfo, rawSize));
		break;
	
	case 0x00040006:		// IMAGEKEY_EXECUTION_ID
		DATABLOCK_TO_BE_32(data, offsetof(ExecutionId, mediaId));
		DATABLOCK_TO_BE_32(data, offsetof(ExecutionId, version));
		DATABLOCK_TO_BE_32(data, offsetof(ExecutionId, baseVersion));
		DATABLOCK_TO_BE_32(data, offsetof(ExecutionId, titleId));
		DATABLOCK_TO_BE_32(data, offsetof(ExecutionId, saveGameId));
		break;
	
	case 0x00040310:		// IMAGEKEY_GAME_RATINGS
		// no conversion required
		break;
	
	case 0x00040404:		// IMAGEKEY_LAN_KEY
		// no conversion required
		break;
	
	case 0x000405FF:		// IMAGEKEY_LOGO_DATA
		DATABLOCK_TO_BE_32(data, offsetof(LogoData, logoSize));
		break;
	
	case 0x000406FF:		// IMAGEKEY_MULTIDISC_MEDIA_IDS
		// no conversion required
		break;
	
	case 0x00E10402:		// IMAGEKEY_EXPORTS_BY_NAME
		DATABLOCK_TO_BE_32(data, offsetof(ExportsByName, exportTableOffset));
		DATABLOCK_TO_BE_32(data, offsetof(ExportsByName, exportTableSize));
		break;
	}
}


bool XexWriter::encKey(XexKey& dataKey, const XexKey& cryptKey)
{
	XECRYPT_AES_STATE aes_ctx;
	XeCryptAesKey(&aes_ctx, cryptKey.data);
	XeCryptAesEcb(&aes_ctx, dataKey.data, dataKey.data, TRUE);
	return true;
}
bool XexWriter::encRetailKey(XexKey& dataKey)
{
	XexKey retail_key;
	memcpy(retail_key.data, XexData::XEX_RETAIL_KEY, sizeof(XexKey));
	return encKey(dataKey, retail_key);
}
bool XexWriter::encDebugKey(XexKey& dataKey)
{
	XexKey debug_key;
	memcpy(debug_key.data, XexData::XEX_DEBUG_KEY, sizeof(XexKey));
	return encKey(dataKey, debug_key);
}
bool XexWriter::encMfgRetailKey(XexKey& dataKey)
{
	XexKey mfg_retail_key;
	memcpy(mfg_retail_key.data, XexData::XEX_MFG_RETAIL_KEY, sizeof(XexKey));
	return encKey(dataKey, mfg_retail_key);
}
bool XexWriter::encMfgDebugKey(XexKey& dataKey)
{
	XexKey mfg_debug_key;
	memcpy(mfg_debug_key.data, XexData::XEX_MFG_DEBUG_KEY, sizeof(XexKey));
	return encKey(dataKey, mfg_debug_key);
}


// signs xex file with debug key
bool XexWriter::updateSign(XexSecurityInfo& secInfo, const u8* publicKey, const u8* privateKey)
{
	u8* sig_ptr = secInfo.imageInfo.signature;
	s32 sig_size = 0x100;
	s32 hash_size = GET32BE(&secInfo.imageInfo.infoSize) - sig_size;
	XexHash hash;
	XeCryptRotSumSha(sig_ptr+sig_size, hash_size,
		0, 0,
		hash.data, sizeof(XexHash));
	
	const u8* salt;
	if( xex.hasRequiredRevocationCheck() )
		salt = XexData::XEX_SALT_REV;
	else
		salt = XexData::XEX_SALT_XEX;
	XeCryptBnQwBeSigCreate((u64*)sig_ptr, hash.data, salt, (XECRYPT_RSA*)publicKey);
	if( !XeCryptBnQwNeModExp((u64*)sig_ptr, (u64*)sig_ptr, (u64*)(privateKey+0x390), (u64*)(privateKey+0x10), 0x20) )
		return false;
	memcpy(secInfo.imageInfo.signature, sig_ptr, sig_size);
	return true;
}
bool XexWriter::updateRetailSign(XexSecurityInfo& secInfo)
{
	// dont have retail private key, so can't sign it
	//return updateSign(secInfo, XexData::XEX_RETAIL_PUBLIC_KEY, XexData::XEX_RETAIL_PRIVATE_KEY);
	
	memset(secInfo.imageInfo.signature, 0, sizeof(secInfo.imageInfo.signature));
	return false;
}
bool XexWriter::updateDebugSign(XexSecurityInfo& secInfo)
{
	return updateSign(secInfo, XexData::XEX_DEBUG_PUBLIC_KEY, XexData::XEX_DEBUG_PRIVATE_KEY);
}


// fix all section hashes
bool XexWriter::updateSectionHashes(XexHvSectionInfo* sections, XexSecurityInfo& secInfo)
{
	XexHash hash;
	memset(hash.data, 0, sizeof(XexHash));
	s32 image_size_left = xex.getImageSize();
	if(image_size_left <= 0)
		return true;
	
	DataBlock basefile;
	if(xex.isPatchModule())
		xex.getPatchData(basefile);
	else
		xex.getBasefile(basefile);
	
	for(s32 hash_flag_index = xex.numSections()-1;
		hash_flag_index >= 0;
		hash_flag_index--)
	{
		// get next section and fill its hash with
		// the result of the last hash calculated
		memcpy(sections[hash_flag_index].hash.data, hash.data, sizeof(XexHash));
		
		// calculate bounds for calculating hash over
		s32 hash_size;
		u8  type;
		xex.getSection(hash_flag_index, hash_size, type);
		image_size_left -= hash_size;
		s32 hash_offset = image_size_left;
		
		// get data to calculate hash over, then calculate hash
		u8* block_data = new u8[hash_size];
		basefile.get(block_data, hash_offset, hash_size);
		XECRYPT_SHA_STATE sha_ctx;
		XeCryptShaInit(&sha_ctx);
		XeCryptShaUpdate(&sha_ctx, block_data, hash_size);
		delete[] block_data;
		
		// update hash with section-info contents
		XeCryptShaUpdate(&sha_ctx, (u8*)&sections[hash_flag_index], sizeof(XexHvSectionInfo));
		XeCryptShaFinal(&sha_ctx, hash.data, sizeof(XexHash));
	}
	
	// the first section hash goes in the header
	memcpy(secInfo.imageInfo.imageHash.data, hash.data, sizeof(XexHash));
	return true;
}

// updates hash over library import data
bool XexWriter::updateImportHash(u8* headersData)
{
	// find import data
	XexImageHeader* xex_header_ptr = (XexImageHeader*)(headersData);
	XexSecurityInfo* sec_info = (XexSecurityInfo*)(headersData + GET32BE(&xex_header_ptr->securityInfoOffset));
	XexImageEntry* opt_info = (XexImageEntry*)(headersData + sizeof(XexImageHeader));
	s32 num_opt_info = GET32BE(&xex_header_ptr->imageEntryCount);
	u8* import_data = NULL;
	for(s32 i=0; i<num_opt_info; i++)
	{
		if(GET32BE(&opt_info[i].key) == 0x103FF)
		{
			import_data = headersData + GET32BE(&opt_info[i].value);
			break;
		}
	}
	if(import_data == NULL)
		return true;
	
	XexHash hash;
	memset(hash.data, 0, sizeof(XexHash));
	for(s32 lib_num=GET32BE(import_data+8)-1; lib_num>=0; lib_num--)
	{
		// get pointer to current lib import
		u8* import_ptr = import_data + 12 + GET32BE(import_data + 4);
		for(s32 i=0; i<lib_num; i++)
			import_ptr += GET32BE(import_ptr);
		s32 import_size = 0x24 + GET16BE(import_ptr + 0x26) * 4;
		memcpy(import_ptr + 4, hash.data, sizeof(XexHash));
		
		XeCryptSha(import_ptr + 4, import_size,
			0,0,
			0,0,
			hash.data, sizeof(XexHash));
	}
	
	memcpy(sec_info->imageInfo.importHash.data, hash.data, sizeof(XexHash));
	return true;
}

// updates hash over various parts of the xex headers
bool XexWriter::updateHeaderHash(u8* headersData, s32 headersSize, s32 securityInfoOffset)
{
	XexImageHeader* xex_header_ptr = (XexImageHeader*)(headersData);
	XexSecurityInfo* security_ptr = (XexSecurityInfo*)(headersData + securityInfoOffset);
	s32 data_offset = headersSize;
	
	// get hash over: security info from allowedMediaTypes onwards, section info and optional header info
	// then over: xex header, optional header entries and first 8 bytes of security info
	XeCryptSha((u8*)&security_ptr->allowedMediaTypes, data_offset - securityInfoOffset - offsetof(XexSecurityInfo, allowedMediaTypes),
		(u8*)xex_header_ptr, securityInfoOffset+8,
		0,0,
		security_ptr->imageInfo.headerHash.data, sizeof(XexHash));
	
	return true;
}
*/
