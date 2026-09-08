// 
// handles all data in an xex header
// 

#include "XexHeader.h"
#include "XexDefines.h"
#include "XexImageEntryTypes.h"
#include "XexData.h"
#include "XeCrypt.h"
#include "Xex.h"
#include "Endian.h"

#define SET_FLAG(toggle, flags, flag)	(toggle) ? ((flags) |= (flag)) : ((flags) &= (~(flag)))
#define IS_FLAG_SET(flags, flag)		(((flags) & (flag)) != 0)

#define DATABLOCK_TO_BE_16(data_block, offset) (data_block).set16be((data_block).get16(offset), (offset))
#define DATABLOCK_TO_BE_32(data_block, offset) (data_block).set32be((data_block).get32(offset), (offset))
#define DATABLOCK_TO_BE_64(data_block, offset) (data_block).set64be((data_block).get64(offset), (offset))

#define DATABLOCK_FROM_BE_16(data_block, offset) (data_block).set16((data_block).get16be(offset), (offset))
#define DATABLOCK_FROM_BE_32(data_block, offset) (data_block).set32((data_block).get32be(offset), (offset))
#define DATABLOCK_FROM_BE_64(data_block, offset) (data_block).set64((data_block).get64be(offset), (offset))


XexHeader::XexHeader()
{
}
XexHeader::XexHeader(XexHeader const& source)
{
	m_headers = source.m_headers;
}

XexHeader& XexHeader::operator=(XexHeader const& source)
{
	// watch out for self assignment
	if(this != &source)
	{
		m_headers = source.m_headers;
	}
	return *this;
}
XexHeader::~XexHeader()
{
}


// sets up all headers
bool XexHeader::readHeaders(Xex& xex, const DataBlock& headers, s32& imageSize, DataBlock& basefileFormat)
{
	m_pXex = &xex;
	
	s32 headers_offset = 0;
	
	// get image headers
	XexImageHeader xex_header;
	headers.get(&xex_header, headers_offset, sizeof(xex_header));
	headers_offset += sizeof(xex_header);
	convertXexHeaderFromBE(xex_header);
	setXexHeader(xex_header);
	
	// read in image entries
	s32 entry_offset = sizeof(XexImageHeader);
	for(s32 i=0; i<xex_header.imageEntryCount; i++)
	{
		// read in image entry
		XexImageEntry image_entry;
		headers.get(&image_entry, entry_offset, sizeof(image_entry));
		convertImageEntryHeaderFromBE(image_entry);
		
		// get image entry data
		s32 data_offset;
		DataBlock entry_block;
		if( !IS_IMAGEENTRY_DATA_OFFSET(image_entry) )
		{
			data_offset = -1;
			entry_block.set32(image_entry.value, 0);
		}
		else
		{
			data_offset = image_entry.value;
			s32 data_size;
			if( IS_IMAGEENTRY_DATA_VARIABLE_SIZE(image_entry) )
			{
				data_size = headers.get32be(data_offset);
			}
			else
			{
				data_size = (image_entry.key & 0xFF) * 4;
			}
			headers.get(entry_block, 0, data_offset, data_size);
		}
		
		// extract data from image entry
		convertImageEntryDataFromBE(image_entry, entry_block);
		addImageEntry(image_entry, entry_block);
		entry_offset += sizeof(XexImageEntry);
	}
	
	// read in security info
	XexSecurityInfo sec_info;
	headers.get(&sec_info, xex_header.securityInfoOffset, sizeof(sec_info));
	
	// verify signature to determine if retail or debug xex
	// if verification of retail signature fails we assume it is debug.
	// UNLESS the signature is all zeros.
	// since we cannot resign for retail, so we just zero the signature when altering a retail xex
	bool is_debug = !verifySign(XexData::XEX_RETAIL_PUBLIC_KEY, sec_info);
	int num_zeros;
	for(num_zeros=0; num_zeros<sizeof(sec_info.imageInfo.signature); num_zeros++)
	{
		if(sec_info.imageInfo.signature[num_zeros] != 0)
			break;
	}
	if(num_zeros == sizeof(sec_info.imageInfo.signature))
		is_debug = false;
	
	convertSecurityInfoFromBE(sec_info);
	
	// decrypt image key (not if a patch module tho)
	if( sec_info.imageInfo.imageFlags & (IMAGEFLAG_MANUFACTURING_UTILITY|IMAGEFLAG_MANUFACTURING_TOOL) )
	{
		// manufacturing mode
		if(is_debug)
		{
			m_pXex->setDebug();
			if( !m_pXex->isPatchModule() ) decMfgDebugKey(sec_info.imageInfo.imageKey);
		}
		else
		{
			m_pXex->setRetail();
			if( !m_pXex->isPatchModule() ) decMfgRetailKey(sec_info.imageInfo.imageKey);
		}
	}
	else
	{
		// normal mode
		if(is_debug)
		{
			m_pXex->setDebug();
			if( !m_pXex->isPatchModule() ) decDebugKey(sec_info.imageInfo.imageKey);
		}
		else
		{
			m_pXex->setRetail();
			if( !m_pXex->isPatchModule() ) decRetailKey(sec_info.imageInfo.imageKey);
		}
	}
	setSecurityInfo(sec_info);
	
	// read in sections
	s32 section_offset = xex_header.securityInfoOffset + sizeof(XexSecurityInfo);
	for(s32 i=0; i<(s32)sec_info.sectionCount; i++)
	{
		XexHvSectionInfo section;
		headers.get(&section, section_offset, sizeof(section));
//		section.dword = GET32BE(&section.dword);
		convertSectionFromBE(section);
		addSection(section);
		section_offset += sizeof(section);
	}
	
	// store "original headers" for use with patching
	m_pXex->setOriginalHeaders(headers);
	
	basefileFormat = m_basefileFormat;
	imageSize = sec_info.imageSize;
	
	return true;
}

// gets all headers
bool XexHeader::writeHeaders(const Xex& xex, DataBlock& headers, const s32& imageSize, const DataBlock& basefileFormat)
{
	m_pXex = (Xex*)&xex;
	m_basefileFormat = basefileFormat;
	
	// prepare image entries
	s32 num_image_entries = getNumImageEntries();
	s32 entry_data_size = getImageEntryDataSize();
	s32 sec_info_offset = sizeof(XexImageHeader) + num_image_entries * sizeof(XexImageEntry);
	if( !m_pXex->isPatchModule() )
		sec_info_offset += 0x80;
	s32 sections_offset = sec_info_offset + sizeof(XexSecurityInfo);
	s32 entry_data_offset = sections_offset + m_pXex->numSections() * sizeof(XexHvSectionInfo);
	s32 basefile_offset = entry_data_offset + entry_data_size;
	if( m_pXex->isPatchModule() )
		basefile_offset = (basefile_offset + 0x800-1) & (-0x800);
	else
		basefile_offset = (basefile_offset + 0x1000-1) & (-0x1000);
	s32 headers_size = basefile_offset;
	u8* headers_data = new u8[headers_size];
	memset(headers_data, 0, headers_size);
	
	// prepare xex header
	XexImageHeader* xex_header = (XexImageHeader*)headers_data;
	getXexHeader(*xex_header, basefile_offset, sec_info_offset, num_image_entries);
	convertXexHeaderToBE(*xex_header);
	
	// prepare security info
	XexSecurityInfo* sec_info = (XexSecurityInfo*)(headers_data + sec_info_offset);
	getSecurityInfo(*sec_info);
	convertSecurityInfoToBE(*sec_info);
	
	// prepare sections
	XexHvSectionInfo* sections = (XexHvSectionInfo*)(headers_data + sections_offset);
	for(s32 sec_num=0; sec_num<m_pXex->numSections(); sec_num++)
	{
		getSection(sec_num, sections[sec_num]);
		sections[sec_num].dword = sections[sec_num].dword;
//		SET32BE(&sections[sec_num].dword, sections[sec_num].dword);
		convertSectionToBE(sections[sec_num]);
	}
	
	// prepare image entries
	XexImageEntry* image_entries_ptr = (XexImageEntry*)(headers_data + sizeof(XexImageHeader));
	getImageEntry(image_entries_ptr, headers_data + entry_data_offset, headers_size - entry_data_offset, entry_data_offset);
	
	// make sure hashes and signatures are valid
	if( !updateImportHash(headers_data) ||
		!updateSectionHashes(sections, *sec_info) ||
		!updateHeaderHash(headers_data, headers_size, sec_info_offset) )
	{
		delete[] headers_data;
		return false;
	}
	if( m_pXex->isDebug() )
		updateDebugSign(*sec_info);
	else
		updateRetailSign(*sec_info);
	
	headers.set(headers_data, 0, headers_size);
	delete[] headers_data;
	return true;
}



void XexHeader::convertXexVersionInfoFromBE(XexVersionInfo& ver)
{
	ver.major	= GET16BE(&ver.major);
	ver.minor	= GET16BE(&ver.minor);
	ver.build	= GET16BE(&ver.build);
	ver.word	= GET16BE(&ver.word);
#ifndef _XBOX
/*	u16 tmp				= ver.word;
	ver.qfe				= (tmp & 0x00FF) >> 0;
	ver.unused			= (tmp & 0x0F00) >> 8;
	ver.xexVersion		= (tmp & 0x1000) >>12;
	ver.approvedlibrary	= (tmp & 0x6000) >>13;
	ver.debugBuild		= (tmp & 0x8000) >>15;
*/
#endif
}
void XexHeader::convertXexVersionInfoToBE(XexVersionInfo& ver)
{
#ifndef _XBOX
/*	u16 tmp =	(ver.qfe			<< 0) |
				(ver.unused			<< 8) |
				(ver.xexVersion		<<12) |
				(ver.approvedlibrary<<13) |
				(ver.debugBuild		<<15);
	ver.word = tmp;
*/
#endif
	SET16BE(&ver.major, ver.major);
	SET16BE(&ver.minor, ver.minor);
	SET16BE(&ver.build, ver.build);
	SET16BE(&ver.word,	ver.word);
}

void XexHeader::convertXexVersion32FromBE(XexVersion32& ver)
{
	ver.dword = GET32BE(&ver.dword);
#ifndef _XBOX
//	u32 tmp		= ver.dword;
//	ver.qfe		= (tmp & 0x000000FF) >>  0;
//	ver.build	= (tmp & 0x00FFFF00) >>  8;
//	ver.minor	= (tmp & 0x0F000000) >> 24;
//	ver.major	= (tmp & 0xF0000000) >> 28;
#endif
}
void XexHeader::convertXexVersion32ToBE(XexVersion32& ver)
{
#ifndef _XBOX
//	u32 tmp		= (ver.qfe<<0) | (ver.build<<8) | (ver.minor<<24) | (ver.major<<28);
//	ver.dword	= tmp;
#endif
	SET32BE(&ver.dword, ver.dword);
}


void XexHeader::convertXexHeaderFromBE(XexImageHeader& xexHeader)
{
	xexHeader.moduleFlags				= GET32BE(&xexHeader.moduleFlags);
	xexHeader.sizeOfHeaders				= GET32BE(&xexHeader.sizeOfHeaders);
	xexHeader.sizeOfDiscardableHeaders	= GET32BE(&xexHeader.sizeOfDiscardableHeaders);
	xexHeader.securityInfoOffset		= GET32BE(&xexHeader.securityInfoOffset);
	xexHeader.imageEntryCount			= GET32BE(&xexHeader.imageEntryCount);
}
void XexHeader::convertXexHeaderToBE(XexImageHeader& xexHeader)
{
	SET32BE(&xexHeader.moduleFlags,				xexHeader.moduleFlags);
	SET32BE(&xexHeader.sizeOfHeaders,			xexHeader.sizeOfHeaders);
	SET32BE(&xexHeader.sizeOfDiscardableHeaders,xexHeader.sizeOfDiscardableHeaders);
	SET32BE(&xexHeader.securityInfoOffset,		xexHeader.securityInfoOffset);
	SET32BE(&xexHeader.imageEntryCount,			xexHeader.imageEntryCount);
}

void XexHeader::getXexHeader(XexImageHeader& xexHeader, s32 basefileOffset, s32 securityInfoOffset, s32 numOptionalInfo)
{
	memcpy(xexHeader.magic, XexData::XEX_MAGIC, 4);
	u32 mflags = 0;
	SET_FLAG(m_pXex->isTitleModule(),	mflags, MODULEFLAG_TITLE_MODULE);
	SET_FLAG(m_pXex->isTitleExports(),	mflags, MODULEFLAG_EXPORTS_TO_TITLE);
	SET_FLAG(m_pXex->isSystemDebugger(),mflags, MODULEFLAG_SYSTEM_DEBUGGER);
	SET_FLAG(m_pXex->isDllModule(),		mflags, MODULEFLAG_DLL_MODULE);
	SET_FLAG(m_pXex->isPatchModule(),	mflags, MODULEFLAG_PATCH_MODULE);
	SET_FLAG(m_pXex->isPatchFull(),		mflags, MODULEFLAG_PATCH_FULL);
	SET_FLAG(m_pXex->isPatchDelta(),	mflags, MODULEFLAG_PATCH_DELTA);
	SET_FLAG(m_pXex->isUserMode(),		mflags, MODULEFLAG_USER_MODE);
	mflags |= m_pXex->getUnknownModuleFlags();
	xexHeader.moduleFlags = mflags;
	xexHeader.sizeOfHeaders = basefileOffset;
	xexHeader.sizeOfDiscardableHeaders = m_pXex->getDiscardableHeaderSize();
	xexHeader.securityInfoOffset = securityInfoOffset;
	xexHeader.imageEntryCount = numOptionalInfo;
}
void XexHeader::setXexHeader(const XexImageHeader& xexHeader)
{
	u32 mflags = xexHeader.moduleFlags;
	m_pXex->setTitleModule(		IS_FLAG_SET(mflags, MODULEFLAG_TITLE_MODULE) );
	m_pXex->setTitleExports(	IS_FLAG_SET(mflags, MODULEFLAG_EXPORTS_TO_TITLE) );
	m_pXex->setSystemDebugger(	IS_FLAG_SET(mflags, MODULEFLAG_SYSTEM_DEBUGGER) );
	m_pXex->setDllModule(		IS_FLAG_SET(mflags, MODULEFLAG_DLL_MODULE) );
	m_pXex->setPatchModule(		IS_FLAG_SET(mflags, MODULEFLAG_PATCH_MODULE) );
	m_pXex->setPatchFull(		IS_FLAG_SET(mflags, MODULEFLAG_PATCH_FULL) );
	m_pXex->setPatchDelta(		IS_FLAG_SET(mflags, MODULEFLAG_PATCH_DELTA) );
	m_pXex->setUserMode(		IS_FLAG_SET(mflags, MODULEFLAG_USER_MODE) );
	m_pXex->setUnknownModuleFlags(mflags & MODULEFLAG_UNKNOWN);
	
	m_pXex->setDiscardableHeaderSize( xexHeader.sizeOfDiscardableHeaders );
}


// convert security info to system format from BE format
void XexHeader::convertSecurityInfoFromBE(XexSecurityInfo& sec_info)
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
// convert security info in system format to BE format
void XexHeader::convertSecurityInfoToBE(XexSecurityInfo& secInfo)
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

void XexHeader::getSecurityInfo(XexSecurityInfo& secInfo)
{
	// size of security data
	secInfo.size = sizeof(XexSecurityInfo) + m_pXex->numSections() * sizeof(XexHvSectionInfo);
	secInfo.imageSize = m_pXex->getImageSize();
	memset(secInfo.imageInfo.signature, 0, sizeof(secInfo.imageInfo.signature));
	secInfo.imageInfo.infoSize = offsetof(XexSecurityInfo, allowedMediaTypes) - (offsetof(XexSecurityInfo, imageInfo)+offsetof(XexHvImageInfo, signature));
	u32 iflags = 0;
	SET_FLAG(m_pXex->isManufacturingUtility(),		iflags, IMAGEFLAG_MANUFACTURING_UTILITY);
	SET_FLAG(m_pXex->isManufacturingSupportTool(),	iflags, IMAGEFLAG_MANUFACTURING_TOOL);
	SET_FLAG(m_pXex->isXGD2Only(),			iflags, IMAGEFLAG_XGD2);
	SET_FLAG(m_pXex->isCardeaKey(),			iflags, IMAGEFLAG_CARDEA_KEY);
	SET_FLAG(m_pXex->isXeikaKey(),			iflags, IMAGEFLAG_XEIKA_KEY);
	SET_FLAG(m_pXex->isTitleUsermode(),		iflags, IMAGEFLAG_TITLE_USERMODE);
	SET_FLAG(m_pXex->isSystemUsermode(),	iflags, IMAGEFLAG_SYSTEM_USERMODE);
	SET_FLAG(m_pXex->isOrange0(),			iflags, IMAGEFLAG_ORANGE0);
	SET_FLAG(m_pXex->isOrange1(),			iflags, IMAGEFLAG_ORANGE1);
	SET_FLAG(m_pXex->isOrange2(),			iflags, IMAGEFLAG_ORANGE2);
//	SET_FLAG(m_pXex->isTestkitRestricted(),	iflags, IMAGEFLAG_TESTKIT_RESTRICTED);
	SET_FLAG(m_pXex->isSignedKeyvaultRestricted(),	iflags, IMAGEFLAG_SIGNED_KV_RESTRICTED);
	SET_FLAG(m_pXex->isIptvSignupApp(),		iflags, IMAGEFLAG_IPTV_SIGNUP_APP);
	SET_FLAG(m_pXex->isIptvTitleApp(),		iflags, IMAGEFLAG_IPTV_TITLE_APP);
	SET_FLAG(m_pXex->isNccpKeys(),			iflags, IMAGEFLAG_NCCP_KEYS);
	SET_FLAG(m_pXex->isActivationReq(),		iflags, IMAGEFLAG_ACTIVATION_REQ);
	SET_FLAG(m_pXex->isPageSize4KB(),		iflags, IMAGEFLAG_4K_PAGES);
	SET_FLAG(m_pXex->isNoGameRegion(),		iflags, IMAGEFLAG_NO_GAME_REGION);
	SET_FLAG(m_pXex->hasOptionalRevocationCheck(),	iflags, IMAGEFLAG_REVOCATION_CHECK_OPT);
	SET_FLAG(m_pXex->hasRequiredRevocationCheck(),	iflags, IMAGEFLAG_REVOCATION_CHECK_REQ);
	iflags |= m_pXex->getUnknownImageFlags();
	secInfo.imageInfo.imageFlags = iflags;
	secInfo.imageInfo.loadAddress = m_pXex->getLoadAddress();
	secInfo.imageInfo.importTableCount = m_pXex->numImportLibraries();
	m_pXex->getMediaId(secInfo.imageInfo.mediaId);
	m_pXex->getImageKey(secInfo.imageInfo.imageKey);
	if( m_pXex->isManufacturingSupportTool() || m_pXex->isManufacturingUtility() )
	{
		if( m_pXex->isDebug() )
			encMfgDebugKey(secInfo.imageInfo.imageKey);
		else
			encMfgRetailKey(secInfo.imageInfo.imageKey);
	}
	else
	{
		if( m_pXex->isDebug() )
			encDebugKey(secInfo.imageInfo.imageKey);
		else
			encRetailKey(secInfo.imageInfo.imageKey);
	}
	secInfo.imageInfo.exportTableAddress = m_pXex->getExportTableAddress();
	u32 region_flags = 0;
	SET_FLAG(m_pXex->isRegionNorthAmerica(),region_flags, REGION_NORTH_AMERICA);
	SET_FLAG(m_pXex->isRegionJapan(),		region_flags, REGION_JAPAN);
	SET_FLAG(m_pXex->isRegionChina(),		region_flags, REGION_CHINA);
	SET_FLAG(m_pXex->isRegionRestOfAsia(),	region_flags, REGION_REST_OF_ASIA);
	SET_FLAG(m_pXex->isRegionAustNZ(),		region_flags, REGION_AUST_NZ);
	SET_FLAG(m_pXex->isRegionRestOfEurope(),region_flags, REGION_REST_OF_EUROPE);
	SET_FLAG(m_pXex->isRegionRestOfWorld(),	region_flags, REGION_REST_OF_WORLD);
	secInfo.imageInfo.gameRegion = region_flags;
	u32 media_flags = 0;
	SET_FLAG(m_pXex->isMediaHardDisk(),		media_flags, MEDIATYPE_HARD_DISK);
	SET_FLAG(m_pXex->isMediaDvdX2(),		media_flags, MEDIATYPE_DVDX2);
	SET_FLAG(m_pXex->isMediaDvdCd(),		media_flags, MEDIATYPE_DVD_CD);
	SET_FLAG(m_pXex->isMediaDvd5(),			media_flags, MEDIATYPE_DVD5);
	SET_FLAG(m_pXex->isMediaDvd9(),			media_flags, MEDIATYPE_DVD9);
	SET_FLAG(m_pXex->isMediaSystemFlash(),	media_flags, MEDIATYPE_SYS_FLASH);
	SET_FLAG(m_pXex->isMediaMemoryUnit(),	media_flags, MEDIATYPE_MEM_UNIT);
	SET_FLAG(m_pXex->isMediaMassStorage(),	media_flags, MEDIATYPE_MASS_STORAGE);
	SET_FLAG(m_pXex->isMediaSMB(),			media_flags, MEDIATYPE_SMB);
	SET_FLAG(m_pXex->isMediaRam(),			media_flags, MEDIATYPE_RAM);
	SET_FLAG(m_pXex->isMediaRamDrive(),		media_flags, MEDIATYPE_RAM_DRIVE);
	SET_FLAG(m_pXex->isMediaSecureVirtOD(),	media_flags, MEDIATYPE_SECURE_VIRT_OD);
	SET_FLAG(m_pXex->isMediaWirelessNStorage(),		media_flags, MEDIATYPE_WIRELESS_N_STORAGE);
	SET_FLAG(m_pXex->isMediaSystemExtPartition(),	media_flags, MEDIATYPE_SYS_EXT_PARTITION);
	SET_FLAG(m_pXex->isMediaSystemAuxPartition(),	media_flags, MEDIATYPE_SYS_AUX_PARTITION);
	SET_FLAG(m_pXex->isMediaInsecurePackage(),		media_flags, MEDIATYPE_INSECURE_PKG);
	SET_FLAG(m_pXex->isMediaSavegamePackage(),		media_flags, MEDIATYPE_SAVEGAME_PKG);
	SET_FLAG(m_pXex->isMediaLocallySignedPackage(),	media_flags, MEDIATYPE_LOCALSIGN_PKG);
	SET_FLAG(m_pXex->isMediaLiveSignedPackage(),	media_flags, MEDIATYPE_LIVESIGN_PKG);
	SET_FLAG(m_pXex->isMediaXboxPackage(),	media_flags, MEDIATYPE_XBOX_PKG);
	media_flags |= m_pXex->getUnknownMediaTypes();
	secInfo.allowedMediaTypes = media_flags;
	secInfo.sectionCount = m_pXex->numSections();
}
void XexHeader::setSecurityInfo(const XexSecurityInfo& sec_info)
{
	// check and assign data from security info
	u32 iflags = sec_info.imageInfo.imageFlags;
	m_pXex->setManufacturingUtility(	IS_FLAG_SET(iflags, IMAGEFLAG_MANUFACTURING_UTILITY) );
	m_pXex->setManufacturingSupportTool(IS_FLAG_SET(iflags, IMAGEFLAG_MANUFACTURING_TOOL) );
	m_pXex->setXGD2Only(		IS_FLAG_SET(iflags, IMAGEFLAG_XGD2) );
	m_pXex->setCardeaKey(		IS_FLAG_SET(iflags, IMAGEFLAG_CARDEA_KEY) );
	m_pXex->setXeikaKey(		IS_FLAG_SET(iflags, IMAGEFLAG_XEIKA_KEY) );
	m_pXex->setTitleUsermode(	IS_FLAG_SET(iflags, IMAGEFLAG_TITLE_USERMODE) );
	m_pXex->setSystemUsermode(	IS_FLAG_SET(iflags, IMAGEFLAG_SYSTEM_USERMODE) );
	m_pXex->setOrange0(			IS_FLAG_SET(iflags, IMAGEFLAG_ORANGE0) );
	m_pXex->setOrange1(			IS_FLAG_SET(iflags, IMAGEFLAG_ORANGE1) );
	m_pXex->setOrange2(			IS_FLAG_SET(iflags, IMAGEFLAG_ORANGE2) );
//	m_pXex->setTestkitRestricted(IS_FLAG_SET(iflags,IMAGEFLAG_TESTKIT_RESTRICTED) );
	m_pXex->setSignedKeyvaultRestricted(IS_FLAG_SET(iflags,IMAGEFLAG_SIGNED_KV_RESTRICTED) );
	m_pXex->setIptvSignupApp(	IS_FLAG_SET(iflags, IMAGEFLAG_IPTV_SIGNUP_APP) );
	m_pXex->setIptvTitleApp(	IS_FLAG_SET(iflags, IMAGEFLAG_IPTV_TITLE_APP) );
	m_pXex->setNccpKeys(		IS_FLAG_SET(iflags, IMAGEFLAG_NCCP_KEYS) );
	m_pXex->setActivationReq(	IS_FLAG_SET(iflags, IMAGEFLAG_ACTIVATION_REQ) );
	if(IS_FLAG_SET(iflags, IMAGEFLAG_4K_PAGES)) m_pXex->setPageSize4KB();
	else										m_pXex->setPageSize64KB();
	m_pXex->setNoGameRegion(	IS_FLAG_SET(iflags, IMAGEFLAG_NO_GAME_REGION) );
	m_pXex->setOptionalRevocationCheck(IS_FLAG_SET(iflags, IMAGEFLAG_REVOCATION_CHECK_OPT) );
	m_pXex->setRequiredRevocationCheck(IS_FLAG_SET(iflags, IMAGEFLAG_REVOCATION_CHECK_REQ) );
	m_pXex->setUnknownImageFlags(iflags & IMAGEFLAG_UNKNOWN);
//	m_pXex->setImportTableCount(sec_info.imageInfo.importTableCount);
	m_pXex->setMediaId(sec_info.imageInfo.mediaId);
	m_pXex->setImageKey(sec_info.imageInfo.imageKey);
	m_pXex->setExportTableAddress(sec_info.imageInfo.exportTableAddress);
	u32 rflags = sec_info.imageInfo.gameRegion;
	m_pXex->setRegionNorthAmerica(	IS_FLAG_SET(rflags, REGION_NORTH_AMERICA) );
	m_pXex->setRegionJapan(			IS_FLAG_SET(rflags, REGION_JAPAN) );
	m_pXex->setRegionChina(			IS_FLAG_SET(rflags, REGION_CHINA) );
	m_pXex->setRegionRestOfAsia(	IS_FLAG_SET(rflags, REGION_REST_OF_ASIA) );
	m_pXex->setRegionAustNZ(		IS_FLAG_SET(rflags, REGION_AUST_NZ) );
	m_pXex->setRegionRestOfEurope(	IS_FLAG_SET(rflags, REGION_REST_OF_EUROPE) );
	m_pXex->setRegionRestOfWorld(	IS_FLAG_SET(rflags, REGION_REST_OF_WORLD) );
	u32 medflags = sec_info.allowedMediaTypes;
	m_pXex->setMediaHardDisk(		IS_FLAG_SET(medflags, MEDIATYPE_HARD_DISK) );
	m_pXex->setMediaDvdX2(			IS_FLAG_SET(medflags, MEDIATYPE_DVDX2) );
	m_pXex->setMediaDvdCd(			IS_FLAG_SET(medflags, MEDIATYPE_DVD_CD) );
	m_pXex->setMediaDvd5(			IS_FLAG_SET(medflags, MEDIATYPE_DVD5) );
	m_pXex->setMediaDvd9(			IS_FLAG_SET(medflags, MEDIATYPE_DVD9) );
	m_pXex->setMediaSystemFlash(	IS_FLAG_SET(medflags, MEDIATYPE_SYS_FLASH) );
	m_pXex->setMediaMemoryUnit(		IS_FLAG_SET(medflags, MEDIATYPE_MEM_UNIT) );
	m_pXex->setMediaMassStorage(	IS_FLAG_SET(medflags, MEDIATYPE_MASS_STORAGE) );
	m_pXex->setMediaSMB(			IS_FLAG_SET(medflags, MEDIATYPE_SMB) );
	m_pXex->setMediaRam(			IS_FLAG_SET(medflags, MEDIATYPE_RAM) );
	m_pXex->setMediaRamDrive(		IS_FLAG_SET(medflags, MEDIATYPE_RAM_DRIVE) );
	m_pXex->setMediaSecureVirtOD(	IS_FLAG_SET(medflags, MEDIATYPE_SECURE_VIRT_OD) );
	m_pXex->setMediaWirelessNStorage(IS_FLAG_SET(medflags, MEDIATYPE_WIRELESS_N_STORAGE) );
	m_pXex->setMediaSystemExtPartition(IS_FLAG_SET(medflags, MEDIATYPE_SYS_EXT_PARTITION) );
	m_pXex->setMediaSystemAuxPartition(IS_FLAG_SET(medflags, MEDIATYPE_SYS_AUX_PARTITION) );
	m_pXex->setMediaInsecurePackage(IS_FLAG_SET(medflags, MEDIATYPE_INSECURE_PKG) );
	m_pXex->setMediaSavegamePackage(IS_FLAG_SET(medflags, MEDIATYPE_SAVEGAME_PKG) );
	m_pXex->setMediaLocallySignedPackage(IS_FLAG_SET(medflags, MEDIATYPE_LOCALSIGN_PKG) );
	m_pXex->setMediaLiveSignedPackage(IS_FLAG_SET(medflags, MEDIATYPE_LIVESIGN_PKG) );
	m_pXex->setMediaXboxPackage(	IS_FLAG_SET(medflags, MEDIATYPE_XBOX_PKG) );
	m_pXex->setUnknownMediaTypes(medflags & MEDIATYPE_UNKNOWN);
	
	// non flag values
	m_pXex->setLoadAddress(sec_info.imageInfo.loadAddress);
//	m_pXex->setImportTableCount(sec_info.imageInfo.importTableCount);
	m_pXex->setMediaId(sec_info.imageInfo.mediaId);
}



void XexHeader::convertSectionFromBE(XexHvSectionInfo& section)
{
	section.dword = GET32BE(&section.dword);
#ifndef _XBOX
//	u32 tmp = section.dword;
//	section.info = tmp & 0xF;
//	section.size = tmp >> 4;
#endif
}
void XexHeader::convertSectionToBE(XexHvSectionInfo& section)
{
#ifndef _XBOX
//	u32 tmp = (section.info & 0xF) | (section.size << 4);
//	section.dword = tmp;
#endif
	SET32BE(&section.dword, section.dword);
}


bool XexHeader::getSection(s32 index, XexHvSectionInfo& section)
{
	if(index < 0 || index >= m_pXex->numSections())
		return false;
	s32 size;
	u8 info;
	m_pXex->getSection(index, size, info);
	section.info = info;
	section.size = size / m_pXex->getPageSize();
	return true;
}
void XexHeader::addSection(const XexHvSectionInfo& section)
{
	m_pXex->addSection(section.size * m_pXex->getPageSize(), section.info);
}


void XexHeader::convertImageEntryHeaderFromBE(XexImageEntry& header)
{
	header.key	= GET32BE(&header.key);
	header.value= GET32BE(&header.value);
}
void XexHeader::convertImageEntryHeaderToBE(XexImageEntry& header)
{
	SET32BE(&header.key, header.key);
	SET32BE(&header.value, header.value);
}

void XexHeader::convertImageEntryDataFromBE(const XexImageEntry& header, DataBlock& data)
{
	if((header.key & 0xFF) == 0xFF)
		DATABLOCK_FROM_BE_32(data, 0);
	s32 info_size = data.get32(0);
	s32 num, type, offset;
	XexVersion32 ver32;
	XexVersionInfo ver_info;
	
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
		// target version
		data.get(&ver32, offsetof(DeltaPatchDescriptor, targetVersion), sizeof(ver32));
		convertXexVersion32FromBE(ver32);
		data.set(&ver32, offsetof(DeltaPatchDescriptor, targetVersion), sizeof(ver32));
		// source version
		data.get(&ver32, offsetof(DeltaPatchDescriptor, sourceVersion), sizeof(ver32));
		convertXexVersion32FromBE(ver32);
		data.set(&ver32, offsetof(DeltaPatchDescriptor, sourceVersion), sizeof(ver32));
		//DATABLOCK_FROM_BE_32(data, offsetof(DeltaPatchDescriptor, targetVersion));
		//DATABLOCK_FROM_BE_32(data, offsetof(DeltaPatchDescriptor, sourceVersion));
		DATABLOCK_FROM_BE_32(data, offsetof(DeltaPatchDescriptor, targetHeaderSize));
		DATABLOCK_FROM_BE_32(data, offsetof(DeltaPatchDescriptor, deltaHeaderSourceOffset));
		DATABLOCK_FROM_BE_32(data, offsetof(DeltaPatchDescriptor, deltaHeaderSourceSize));
		DATABLOCK_FROM_BE_32(data, offsetof(DeltaPatchDescriptor, deltaHeaderTargetOffset));
		DATABLOCK_FROM_BE_32(data, offsetof(DeltaPatchDescriptor, deltaImageSourceOffset));
		DATABLOCK_FROM_BE_32(data, offsetof(DeltaPatchDescriptor, deltaImageSourceSize));
		DATABLOCK_FROM_BE_32(data, offsetof(DeltaPatchDescriptor, deltaImageTargetOffset));
		// after this is the patch data, this is left as is for the packer to use
		break;
	
	case 0x00004004:		// IMAGEKEY_RESTRICT_KV_PRIVS
		DATABLOCK_FROM_BE_64(data, offsetof(RestrictKVPrivs, mask));
		DATABLOCK_FROM_BE_64(data, offsetof(RestrictKVPrivs, val));
		break;
	
	case 0x00004104:		// IMAGEKEY_RESTRICT_DATES
		DATABLOCK_FROM_BE_64(data, offsetof(RestrictDates, start));
		DATABLOCK_FROM_BE_64(data, offsetof(RestrictDates, end));
		break;
	
	case 0x000042FF:		// IMAGEKEY_RESTRICT_CONSOLE_IDS
		// no conversion required
		break;
	
	case 0x00004304:		// IMAGEKEY_DISC_PROFILE_ID
		// no conversion required
		break;
	
	case 0x000080FF:		// IMAGEKEY_BOUND_PATHNAME
		// no conversion required
		break;
	
	case 0x00008105:		// IMAGEKEY_BOUND_DEVICE_ID
		// no conversion required
		break;
	
	case 0x000103FF:		// IMAGEKEY_IMPORT_LIBRARIES
		DATABLOCK_FROM_BE_32(data, offsetof(ImportLibraryDirectory, nameTableSize));
		DATABLOCK_FROM_BE_32(data, offsetof(ImportLibraryDirectory, numNames));
		offset = offsetof(ImportLibraryDirectory, names) + data.get32(offsetof(ImportLibraryDirectory, nameTableSize));
		offset = (offset + 3) & (-4);
		for(s32 i=0; offset<info_size; i++)
		{
			DATABLOCK_FROM_BE_32(data, offset + offsetof(ImportLibraryEntry, infoSize));
			DATABLOCK_FROM_BE_32(data, offset + offsetof(ImportLibraryEntry, moduleNumber));
			// version
			data.get(&ver32, offset + offsetof(ImportLibraryEntry, version), sizeof(ver32));
			convertXexVersion32FromBE(ver32);
			data.set(&ver32, offset + offsetof(ImportLibraryEntry, version), sizeof(ver32));
			// min version
			data.get(&ver32, offset + offsetof(ImportLibraryEntry, minVersion), sizeof(ver32));
			convertXexVersion32FromBE(ver32);
			data.set(&ver32, offset + offsetof(ImportLibraryEntry, minVersion), sizeof(ver32));
			//DATABLOCK_FROM_BE_32(data, offset + offsetof(ImportLibraryEntry, version));
			//DATABLOCK_FROM_BE_32(data, offset + offsetof(ImportLibraryEntry, minVersion));
			// this is actually 2 8bit values, but they need to be swapped around!
//			DATABLOCK_FROM_BE_16(data, offset + offsetof(ImportLibraryEntry, reserved));
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
	
	case 0x00018102:		// IMAGEKEY_IMAGE_CALLCAP
		DATABLOCK_FROM_BE_32(data, offsetof(CallCap, addr1));
		DATABLOCK_FROM_BE_32(data, offsetof(CallCap, addr2));
		break;
	
	case 0x00018200:		// IMAGEKEY_IMAGE_FASTCAP
		// no conversion required
		break;
	
	case 0x000183FF:		// IMAGEKEY_ORIGINAL_PE_NAME
		// no conversion required
		break;
	
	case 0x000200FF:		// IMAGEKEY_LIBRARY_VERSIONS
		num = (info_size - offsetof(StaticLibraryDirectory, entry)) / sizeof(StaticLibraryEntry);
		for(s32 i=0; i<num; i++)
		{
			offset = offsetof(StaticLibraryDirectory, entry) + i * sizeof(StaticLibraryEntry) + offsetof(StaticLibraryEntry, version);
			data.get(&ver_info, offset, sizeof(ver_info));
//			ver_info.word = GET16BE(&ver_info.word);
			convertXexVersionInfoFromBE(ver_info);
			data.set(&ver_info, offset, sizeof(ver_info));
		}
		break;
	
	case 0x00020104:		// IMAGEKEY_TLS_VALUES
		DATABLOCK_FROM_BE_32(data, offsetof(TLSInfo, numSlots));
		DATABLOCK_FROM_BE_32(data, offsetof(TLSInfo, dataSize));
		DATABLOCK_FROM_BE_32(data, offsetof(TLSInfo, rawAddress));
		DATABLOCK_FROM_BE_32(data, offsetof(TLSInfo, rawSize));
		break;
	
	case 0x00028002:		// IMAGEKEY_PAGE_HEAP
		DATABLOCK_FROM_BE_32(data, offsetof(PageHeap, size));
		DATABLOCK_FROM_BE_32(data, offsetof(PageHeap, flags));
		break;
	
	case 0x00040006:		// IMAGEKEY_EXECUTION_ID
		DATABLOCK_FROM_BE_32(data, offsetof(ExecutionId, mediaId));
		// version
		data.get(&ver32, offsetof(ExecutionId, version), sizeof(ver32));
		convertXexVersion32FromBE(ver32);
		data.set(&ver32, offsetof(ExecutionId, version), sizeof(ver32));
		// base version
		data.get(&ver32, offsetof(ExecutionId, baseVersion), sizeof(ver32));
		convertXexVersion32FromBE(ver32);
		data.set(&ver32, offsetof(ExecutionId, baseVersion), sizeof(ver32));
		//DATABLOCK_FROM_BE_32(data, offsetof(ExecutionId, version));
		//DATABLOCK_FROM_BE_32(data, offsetof(ExecutionId, baseVersion));
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
	
	case 0x000407FF:		// IMAGEKEY_ALT_TITLE_IDS
		for(s32 offset=0; offset<(info_size-4); offset+=4)
		{
			DATABLOCK_FROM_BE_32(data, 4 + offset);
		}
		break;
	
	case 0x00E10402:		// IMAGEKEY_EXPORTS_BY_NAME
		DATABLOCK_FROM_BE_32(data, offsetof(ExportsByName, exportTableOffset));
		DATABLOCK_FROM_BE_32(data, offsetof(ExportsByName, exportTableSize));
		break;
	}
}
void XexHeader::convertImageEntryDataToBE(const XexImageEntry& header, DataBlock& data)
{
	s32 info_size = data.get32(0);
	if((header.key & 0xFF) == 0xFF)
		DATABLOCK_TO_BE_32(data, 0);
	s32 num, type, offset;
	XexVersion32 ver32;
	XexVersionInfo ver_info;
	
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
		// target version
		data.get(&ver32, offsetof(DeltaPatchDescriptor, targetVersion), sizeof(ver32));
		convertXexVersion32ToBE(ver32);
		data.set(&ver32, offsetof(DeltaPatchDescriptor, targetVersion), sizeof(ver32));
		// source version
		data.get(&ver32, offsetof(DeltaPatchDescriptor, sourceVersion), sizeof(ver32));
		convertXexVersion32ToBE(ver32);
		data.set(&ver32, offsetof(DeltaPatchDescriptor, sourceVersion), sizeof(ver32));
		//DATABLOCK_TO_BE_32(data, offsetof(DeltaPatchDescriptor, targetVersion));
		//DATABLOCK_TO_BE_32(data, offsetof(DeltaPatchDescriptor, sourceVersion));
		DATABLOCK_TO_BE_32(data, offsetof(DeltaPatchDescriptor, targetHeaderSize));
		DATABLOCK_TO_BE_32(data, offsetof(DeltaPatchDescriptor, deltaHeaderSourceOffset));
		DATABLOCK_TO_BE_32(data, offsetof(DeltaPatchDescriptor, deltaHeaderSourceSize));
		DATABLOCK_TO_BE_32(data, offsetof(DeltaPatchDescriptor, deltaHeaderTargetOffset));
		DATABLOCK_TO_BE_32(data, offsetof(DeltaPatchDescriptor, deltaImageSourceOffset));
		DATABLOCK_TO_BE_32(data, offsetof(DeltaPatchDescriptor, deltaImageSourceSize));
		DATABLOCK_TO_BE_32(data, offsetof(DeltaPatchDescriptor, deltaImageTargetOffset));
		// after this is the patch data, this is left as is for the packer to use
		break;
	
	case 0x00004004:		// IMAGEKEY_RESTRICT_KV_PRIVS
		DATABLOCK_TO_BE_32(data, offsetof(RestrictKVPrivs, mask));
		DATABLOCK_TO_BE_32(data, offsetof(RestrictKVPrivs, val));
		break;
	
	case 0x00004104:		// IMAGEKEY_RESTRICT_DATES
		DATABLOCK_TO_BE_32(data, offsetof(RestrictDates, start));
		DATABLOCK_TO_BE_32(data, offsetof(RestrictDates, end));
		break;
	
	case 0x000042FF:		// IMAGEKEY_RESTRICT_CONSOLE_IDS
		// no conversion required
		break;
	
	case 0x00004304:		// IMAGEKEY_DISC_PROFILE_ID
		// no conversion required
		break;
	
	case 0x000080FF:		// IMAGEKEY_BOUND_PATHNAME
		// no conversion required
		break;
	
	case 0x00008105:		// IMAGEKEY_BOUND_DEVICE_ID
		// no conversion required
		break;
	
	case 0x000103FF:		// IMAGEKEY_IMPORT_LIBRARIES
		offset = offsetof(ImportLibraryDirectory, names) + data.get32(offsetof(ImportLibraryDirectory, nameTableSize));
		offset = (offset + 3) & (-4);
		DATABLOCK_TO_BE_32(data, offsetof(ImportLibraryDirectory, nameTableSize));
		DATABLOCK_TO_BE_32(data, offsetof(ImportLibraryDirectory, numNames));
		for(s32 i=0; offset<info_size; i++)
		{
			s32 num_addresses = data.get16(offset + offsetof(ImportLibraryEntry, numAddresses));
			s32 lib_size = data.get32(offset + offsetof(ImportLibraryEntry, infoSize));
			DATABLOCK_TO_BE_32(data, offset + offsetof(ImportLibraryEntry, infoSize));
			DATABLOCK_TO_BE_32(data, offset + offsetof(ImportLibraryEntry, moduleNumber));
			// version
			data.get(&ver32, offset + offsetof(ImportLibraryEntry, version), sizeof(ver32));
			convertXexVersion32ToBE(ver32);
			data.set(&ver32, offset + offsetof(ImportLibraryEntry, version), sizeof(ver32));
			// min version
			data.get(&ver32, offset + offsetof(ImportLibraryEntry, minVersion), sizeof(ver32));
			convertXexVersion32ToBE(ver32);
			data.set(&ver32, offset + offsetof(ImportLibraryEntry, minVersion), sizeof(ver32));
			//DATABLOCK_TO_BE_32(data, offset + offsetof(ImportLibraryEntry, version));
			//DATABLOCK_TO_BE_32(data, offset + offsetof(ImportLibraryEntry, minVersion));
			// this is actually 2 8bit values, but they need to be swapped around!
//			DATABLOCK_TO_BE_16(data, offset + offsetof(ImportLibraryEntry, reserved));
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
	
	case 0x00018102:		// IMAGEKEY_IMAGE_CALLCAP
		DATABLOCK_TO_BE_32(data, offsetof(CallCap, addr1));
		DATABLOCK_TO_BE_32(data, offsetof(CallCap, addr2));
		break;
	
	case 0x00018200:		// IMAGEKEY_IMAGE_FASTCAP
		// no conversion required
		break;
	
	case 0x000183FF:		// IMAGEKEY_ORIGINAL_PE_NAME
		// no conversion required
		break;
	
	case 0x000200FF:		// IMAGEKEY_LIBRARY_VERSIONS
		num = (info_size - offsetof(StaticLibraryDirectory, entry)) / sizeof(StaticLibraryEntry);
		for(s32 i=0; i<num; i++)
		{
			offset = offsetof(StaticLibraryDirectory, entry) + i * sizeof(StaticLibraryEntry) + offsetof(StaticLibraryEntry, version);
			data.get(&ver_info, offset, sizeof(ver_info));
			convertXexVersionInfoToBE(ver_info);
			data.set(&ver_info, offset, sizeof(ver_info));
		}
		break;
	
	case 0x00020104:		// IMAGEKEY_TLS_VALUES
		DATABLOCK_TO_BE_32(data, offsetof(TLSInfo, numSlots));
		DATABLOCK_TO_BE_32(data, offsetof(TLSInfo, dataSize));
		DATABLOCK_TO_BE_32(data, offsetof(TLSInfo, rawAddress));
		DATABLOCK_TO_BE_32(data, offsetof(TLSInfo, rawSize));
		break;
	
	case 0x00028002:		// IMAGEKEY_PAGE_HEAP
		DATABLOCK_TO_BE_32(data, offsetof(PageHeap, size));
		DATABLOCK_TO_BE_32(data, offsetof(PageHeap, flags));
		break;
	
	case 0x00040006:		// IMAGEKEY_EXECUTION_ID
		DATABLOCK_TO_BE_32(data, offsetof(ExecutionId, mediaId));
		// version
		data.get(&ver32, offsetof(ExecutionId, version), sizeof(ver32));
		convertXexVersion32ToBE(ver32);
		data.set(&ver32, offsetof(ExecutionId, version), sizeof(ver32));
		// base version
		data.get(&ver32, offsetof(ExecutionId, baseVersion), sizeof(ver32));
		convertXexVersion32ToBE(ver32);
		data.set(&ver32, offsetof(ExecutionId, baseVersion), sizeof(ver32));
		//DATABLOCK_TO_BE_32(data, offsetof(ExecutionId, version));
		//DATABLOCK_TO_BE_32(data, offsetof(ExecutionId, baseVersion));
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
	
	case 0x000407FF:		// IMAGEKEY_ALT_TITLE_IDS
		for(s32 offset=0; offset<(info_size-4); offset+=4)
		{
			DATABLOCK_TO_BE_32(data, 4 + offset);
		}
		break;
	
	case 0x00E10402:		// IMAGEKEY_EXPORTS_BY_NAME
		DATABLOCK_TO_BE_32(data, offsetof(ExportsByName, exportTableOffset));
		DATABLOCK_TO_BE_32(data, offsetof(ExportsByName, exportTableSize));
		break;
	}
}

s32 XexHeader::getNumImageEntries()
{
	s32 num_opt_info = 0;
	// IMAGEKEY_RESOURCE_SECTION			0x000002FF
	if(m_pXex->numResources()) num_opt_info++;
	// IMAGEKEY_BASEFILE_FORMAT				0x000003FF
	num_opt_info++;
	// IMAGEKEY_BASE_REFERENCE				0x00000405
	if(m_pXex->hasBaseReference()) num_opt_info++;
	// IMAGEKEY_DELTA_PATCH_DESCRIPTOR		0x000005FF
	if(m_pXex->hasDeltaPatchDescriptor()) num_opt_info++;
	// IMAGEKEY_RESTRICT_KV_PRIVS			0x00004004
	if(m_pXex->hasRestrictKVPrivs()) num_opt_info++;
	// IMAGEKEY_RESTRICT_DATES				0x00004104
	if(m_pXex->hasRestrictDates()) num_opt_info++;
	// IMAGEKEY_RESTRICT_CONSOLE_IDS		0x000042FF
	if(m_pXex->numRestrictConsoleIds()) num_opt_info++;
	// IMAGEKEY_DISC_PROFILE_ID				0x00004304
	if(m_pXex->hasDiscProfileId()) num_opt_info++;
	// IMAGEKEY_BOUND_PATHNAME				0x000080FF
	if(m_pXex->hasBoundingPath()) num_opt_info++;
	// IMAGEKEY_BOUND_DEVICE_ID				0x00008105
	if(m_pXex->hasBoundingDeviceId()) num_opt_info++;
	// IMAGEKEY_ORIGINAL_BASE_ADDRESS		0x00010001
	if(m_pXex->hasOriginalLoadAddress()) num_opt_info++;
	// IMAGEKEY_ENTRY_POINT					0x00010100
	if(m_pXex->hasEntryPoint()) num_opt_info++;
	// IMAGEKEY_IMAGE_BASE_ADDRESS			0x00010201
	if(m_pXex->isBasefilePE()) num_opt_info++;
	// IMAGEKEY_IMPORT_LIBRARIES			0x000103FF
	if(m_pXex->numImportLibraries()) num_opt_info++;
	// IMAGEKEY_IMAGE_CHECKSUM				0x00018002
	if(m_pXex->hasChecksum()) num_opt_info++;
	// IMAGEKEY_IMAGE_CALLCAP				0x00018102
	if(m_pXex->hasCallCap()) num_opt_info++;
	// IMAGEKEY_IMAGE_FASTCAP				0x00018200
	if(m_pXex->hasFastCap()) num_opt_info++;
	// IMAGEKEY_ORIGINAL_PE_NAME			0x000183FF
	if(m_pXex->hasOriginalPEName()) num_opt_info++;
	// IMAGEKEY_LIBRARY_VERSIONS			0x000200FF
	if(m_pXex->numLibraryVersions()) num_opt_info++;
	// IMAGEKEY_TLS_VALUES					0x00020104
	if(m_pXex->hasTLSInfo()) num_opt_info++;
	// IMAGEKEY_STACK_SIZE					0x00020200
	if(m_pXex->hasStackSize()) num_opt_info++;
	// IMAGEKEY_FILESYSTEM_CACHE_SIZE		0x00020301
	if(m_pXex->hasFilesystemCacheSize()) num_opt_info++;
	// IMAGEKEY_HEAP_SIZE					0x00020401
	if(m_pXex->hasHeapSize()) num_opt_info++;
	// IMAGEKEY_PAGE_HEAP					0x00028002
	if(m_pXex->hasPageHeapInfo()) num_opt_info++;
	// IMAGEKEY_SYSTEM_FLAGS				0x00030000
	if(m_pXex->hasSystemFlags()) num_opt_info++;
	// IMAGEKEY_SYSTEM_FLAGS2				0x00030100
	if(m_pXex->hasSystemFlags2()) num_opt_info++;
	// IMAGEKEY_EXECUTION_ID				0x00040006
	if(m_pXex->hasExecutionId()) num_opt_info++;
	// IMAGEKEY_TITLE_WORKSPACE_SIZE		0x00040201
	if(m_pXex->hasWorkspaceSize()) num_opt_info++;
	// IMAGEKEY_GAME_RATINGS				0x00040310
	if(m_pXex->hasGameRatings()) num_opt_info++;
	// IMAGEKEY_LAN_KEY						0x00040404
	if(m_pXex->hasLANKey()) num_opt_info++;
	// IMAGEKEY_LOGO_DATA					0x000405FF
	if(m_pXex->hasLogoData()) num_opt_info++;
	// IMAGEKEY_MULTIDISC_MEDIA_IDS			0x000406FF
	if(m_pXex->numMultidiscMediaIds()) num_opt_info++;
	// IMAGEKEY_ALT_TITLE_IDS				0x000407FF
	if(m_pXex->numAltTitleIds()) num_opt_info++;
	// IMAGEKEY_EXTRA_DEBUG_MEMORY			0x00040801
	if(m_pXex->hasExtraDebugMemory()) num_opt_info++;
	// IMAGEKEY_EXPORTS_BY_NAME				0x00E10402
	if(m_pXex->hasExportsByName()) num_opt_info++;
	
	num_opt_info += m_pXex->numUnknownImageEntries();
	return num_opt_info;
}
s32 XexHeader::getImageEntryDataSize()
{
	const s32 buffer_size = 256;
	u8 buffer[buffer_size];
	s32 opt_info_size = 0;
	
	// IMAGEKEY_RESOURCE_SECTION			0x000002FF
	if(m_pXex->numResources()) opt_info_size += 4 + (m_pXex->numResources() * sizeof(ResourceEntry));
	// IMAGEKEY_BASEFILE_FORMAT				0x000003FF
	opt_info_size += m_basefileFormat.size();
	// IMAGEKEY_BASE_REFERENCE				0x00000405
	if(m_pXex->hasBaseReference()) opt_info_size += sizeof(BaseReference);
	// IMAGEKEY_DELTA_PATCH_DESCRIPTOR		0x000005FF
	if(m_pXex->hasDeltaPatchDescriptor()) { DataBlock patch; m_pXex->getDeltaPatchDescriptor(patch); opt_info_size += patch.size(); }
	// IMAGEKEY_RESTRICT_KV_PRIVS			0x00004004
	if(m_pXex->hasRestrictKVPrivs()) opt_info_size += sizeof(RestrictKVPrivs);
	// IMAGEKEY_RESTRICT_DATES				0x00004104
	if(m_pXex->hasRestrictKVPrivs()) opt_info_size += sizeof(RestrictDates);
	// IMAGEKEY_RESTRICT_CONSOLE_IDS		0x000042FF
	if(m_pXex->numRestrictConsoleIds()) opt_info_size += sizeof(u32) + (m_pXex->numRestrictConsoleIds() * sizeof(ConsoleId));
	// IMAGEKEY_DISC_PROFILE_ID				0x00004304
	if(m_pXex->hasDiscProfileId())
	{
		// must be on a 0x10 byte alignment
		opt_info_size += sizeof(DiscProfileId);
		opt_info_size += 0x10;
	}
	// IMAGEKEY_BOUND_PATHNAME				0x000080FF
	if(m_pXex->hasBoundingPath()) { m_pXex->getBoundingPath((char*)buffer, buffer_size); opt_info_size += 4 + (((s32)strlen((char*)buffer) + 4) & (-4)); }
	// IMAGEKEY_BOUND_DEVICE_ID				0x00008105
	if(m_pXex->hasBoundingDeviceId()) opt_info_size += sizeof(BoundDeviceId);
	// IMAGEKEY_ORIGINAL_BASE_ADDRESS		0x00010001
		// data is stored in header
	// IMAGEKEY_ENTRY_POINT					0x00010100
		// data is stored in header
	// IMAGEKEY_IMAGE_BASE_ADDRESS			0x00010201
		// data is stored in header
	// IMAGEKEY_IMPORT_LIBRARIES			0x000103FF
	if(m_pXex->numImportLibraries())
	{
		// add import libraries header
		opt_info_size += 12;
		s32 lib_idx_handled = -1;
		for(s32 i=0; i<m_pXex->numImportLibraries(); i++)
		{
			DataBlock addresses;
			u32 module_number;
			u8  module_index;
			XexVersion32 version;
			XexVersion32 min_version;
			if( m_pXex->getImportLibrary(i, (char*)buffer, version, min_version, addresses, module_number, module_index) )
			{
				// only get name if it isnt already in the name string table
				if( module_index > lib_idx_handled )
				{
					lib_idx_handled = module_index;
					opt_info_size += ((s32)strlen((char*)buffer) + 4) & (-4);
				}
				// always get addresses
				opt_info_size += sizeof(ImportLibraryEntry) - 4 + addresses.size();
			}
		}
	}
	// IMAGEKEY_IMAGE_CHECKSUM				0x00018002
	if(m_pXex->hasChecksum()) opt_info_size += sizeof(CheckSumTime);
	// IMAGEKEY_IMAGE_CALLCAP				0x00018102
	if(m_pXex->hasCallCap()) opt_info_size += sizeof(CallCap);
	// IMAGEKEY_IMAGE_FASTCAP				0x00018200
		// data is stored in header
	// IMAGEKEY_ORIGINAL_PE_NAME			0x000183FF
	if(m_pXex->hasOriginalPEName()) { m_pXex->getOriginalPEName((char*)buffer, buffer_size); opt_info_size += 4 + (((s32)strlen((char*)buffer) + 4) & (-4)); }
	// IMAGEKEY_LIBRARY_VERSIONS			0x000200FF
	if(m_pXex->numLibraryVersions()) opt_info_size += sizeof(StaticLibraryDirectory) + (m_pXex->numLibraryVersions()-1) * sizeof(StaticLibraryEntry);
	// IMAGEKEY_TLS_VALUES					0x00020104
	if(m_pXex->hasTLSInfo()) opt_info_size += sizeof(TLSInfo);
	// IMAGEKEY_STACK_SIZE					0x00020200
		// data is stored in header
	// IMAGEKEY_FILESYSTEM_CACHE_SIZE		0x00020301
		// data is stored in header
	// IMAGEKEY_HEAP_SIZE					0x00020401
		// data is stored in header
	// IMAGEKEY_PAGE_HEAP					0x00028002
	if(m_pXex->hasPageHeapInfo()) opt_info_size += sizeof(PageHeap);
	// IMAGEKEY_SYSTEM_FLAGS				0x00030000
		// data is stored in header
	// IMAGEKEY_SYSTEM_FLAGS2				0x00030100
		// data is stored in header
	// IMAGEKEY_EXECUTION_ID				0x00040006
	if(m_pXex->hasExecutionId()) opt_info_size += sizeof(ExecutionId);
	// IMAGEKEY_TITLE_WORKSPACE_SIZE		0x00040201
		// data is stored in header
	// IMAGEKEY_GAME_RATINGS				0x00040310
	if(m_pXex->hasGameRatings()) opt_info_size += sizeof(GameRatings);
	// IMAGEKEY_LAN_KEY						0x00040404
	if(m_pXex->hasLANKey()) opt_info_size += sizeof(LANKey);
	// IMAGEKEY_LOGO_DATA					0x000405FF
	if(m_pXex->hasLogoData()) { DataBlock logo_data; m_pXex->getLogoData(logo_data); opt_info_size += logo_data.size() + 8; }
	// IMAGEKEY_MULTIDISC_MEDIA_IDS			0x000406FF
	if(m_pXex->numMultidiscMediaIds()) opt_info_size += 4 + m_pXex->numMultidiscMediaIds() * sizeof(MediaId);
	// IMAGEKEY_ALT_TITLE_IDS				0x000407FF
	if(m_pXex->numAltTitleIds()) opt_info_size += 4 + m_pXex->numAltTitleIds() * sizeof(u32);
	// IMAGEKEY_EXTRA_DEBUG_MEMORY			0x00040801
		// data is stored in header
	// IMAGEKEY_EXPORTS_BY_NAME				0x00E10402
	if(m_pXex->hasExportsByName()) opt_info_size += sizeof(ExportsByName);
	
	for(s32 i=0; i<m_pXex->numUnknownImageEntries(); i++)
	{
		XexImageEntry header;
		DataBlock data;
		if( m_pXex->getUnknownImageEntry(i, header, data) )
		{
			opt_info_size += data.size();
		}
	}
	return opt_info_size;
}
void XexHeader::getImageEntry(XexImageEntry* headers, u8* data, s32 dataSize, s32 dataOffset)
{
	// insert all image entries in order of id
	XexImageEntry info_header;
	XexImageEntry* headers_ptr = headers;
	u32 info_data_offset = 0;
	
	// IMAGEKEY_RESOURCE_SECTION			0x000002FF
	if(m_pXex->numResources())
	{
		DataBlock resources;
		for(s32 res_num=0; res_num<m_pXex->numResources(); res_num++)
		{
			u32 addr;
			s32 size;
			char name[12];
			memset(name, 0, 12);
			if( m_pXex->getResource(res_num, addr, size, name) )
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
		DataBlock format = m_basefileFormat;
		format.set32(format.size(), offsetof(BaseFileInfoHeader, infoSize));
		u16 type;
		(m_pXex->isEncrypted()) ? type = 1 : type = 0;
		format.set16(type, offsetof(BaseFileInfoHeader, encType));
		type = 0;
		if(		m_pXex->isBinary())				type = 1;
		else if(m_pXex->isRaw())				type = 1;
		else if(m_pXex->isCompressed())			type = 2;
		else if(m_pXex->isDeltaCompressed())	type = 3;
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
	if(m_pXex->hasBaseReference())
	{
		DataBlock base_ref;
		u8 ref[20];
		m_pXex->getBaseReference(ref);
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
	if(m_pXex->hasDeltaPatchDescriptor())
	{
		DataBlock patch;
		m_pXex->getDeltaPatchDescriptor(patch);
		// fix and set values
		info_header.key = IMAGEKEY_DELTA_PATCH_DESCRIPTOR;
		info_header.value = dataOffset + info_data_offset;
		convertImageEntryDataToBE(info_header, patch);
		convertImageEntryHeaderToBE(info_header);
		*headers_ptr++ = info_header;
		patch.get(data + info_data_offset, 0, patch.size());
		info_data_offset += patch.size();
	}
	// IMAGEKEY_RESTRICT_KV_PRIVS			0x00004004
	if(m_pXex->hasRestrictKVPrivs())
	{
		DataBlock kv_privs;
		u64 mask, val;
		m_pXex->getRestrictKVPrivs(mask, val);
		kv_privs.set64(mask, 0);
		kv_privs.set64(val, 8);
		// fix and set values
		info_header.key = IMAGEKEY_RESTRICT_KV_PRIVS;
		info_header.value = dataOffset + info_data_offset;
		convertImageEntryDataToBE(info_header, kv_privs);
		convertImageEntryHeaderToBE(info_header);
		*headers_ptr++ = info_header;
		kv_privs.get(data + info_data_offset, 0, kv_privs.size());
		info_data_offset += kv_privs.size();
	}
	// IMAGEKEY_RESTRICT_DATES				0x00004104
	if(m_pXex->hasRestrictDates())
	{
		DataBlock dates;
		u64 start, end;
		m_pXex->getRestrictDates(start, end);
		dates.set64(start, 0);
		dates.set64(end, 8);
		// fix and set values
		info_header.key = IMAGEKEY_RESTRICT_DATES;
		info_header.value = dataOffset + info_data_offset;
		convertImageEntryDataToBE(info_header, dates);
		convertImageEntryHeaderToBE(info_header);
		*headers_ptr++ = info_header;
		dates.get(data + info_data_offset, 0, dates.size());
		info_data_offset += dates.size();
	}
	// IMAGEKEY_RESTRICT_CONSOLE_IDS		0x000042FF
	if(m_pXex->numRestrictConsoleIds())
	{
		DataBlock console_ids;
		for(s32 i=0; i<m_pXex->numRestrictConsoleIds(); i++)
		{
			ConsoleId console_id;
			console_ids.set(console_id.id, 4 + i*sizeof(ConsoleId), sizeof(ConsoleId));
		}
		console_ids.set32(console_ids.size(), 0);
		// fix and set values
		info_header.key = IMAGEKEY_RESTRICT_CONSOLE_IDS;
		info_header.value = dataOffset + info_data_offset;
		convertImageEntryDataToBE(info_header, console_ids);
		convertImageEntryHeaderToBE(info_header);
		*headers_ptr++ = info_header;
		console_ids.get(data + info_data_offset, 0, console_ids.size());
		info_data_offset += console_ids.size();
	}
	// IMAGEKEY_DISC_PROFILE_ID				0x00004304
	if(m_pXex->hasDiscProfileId())
	{
		int aligned_offset = (0x10 - (dataOffset + info_data_offset)) & 0xF;
		DiscProfileId disc_profile_id;
		DataBlock id;
		m_pXex->getDiscProfileId(disc_profile_id);
		id.set(&disc_profile_id, aligned_offset, sizeof(disc_profile_id));
		// fix and set values
		info_header.key = IMAGEKEY_DISC_PROFILE_ID;
		info_header.value = dataOffset + info_data_offset + aligned_offset;
		convertImageEntryDataToBE(info_header, id);
		convertImageEntryHeaderToBE(info_header);
		*headers_ptr++ = info_header;
		id.get(data + info_data_offset, 0, id.size());
		info_data_offset += id.size();
	}
	// IMAGEKEY_BOUND_PATHNAME				0x000080FF
	if(m_pXex->hasBoundingPath())
	{
		DataBlock bound_path;
		const s32 bound_path_str_size = 1024;
		char* bound_path_str = new char[bound_path_str_size];
		memset(bound_path_str, 0, bound_path_str_size);
		m_pXex->getBoundingPath(bound_path_str, bound_path_str_size);
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
	// IMAGEKEY_BOUND_DEVICE_ID				0x00008105
	if(m_pXex->hasBoundingDeviceId())
	{
		DataBlock bound_id;
		u8 id[20];
		m_pXex->getBoundingDeviceId(id);
		bound_id.set(id, 0, 20);
		// fix and set values
		info_header.key = IMAGEKEY_BOUND_DEVICE_ID;
		info_header.value = dataOffset + info_data_offset;
		convertImageEntryDataToBE(info_header, bound_id);
		convertImageEntryHeaderToBE(info_header);
		*headers_ptr++ = info_header;
		bound_id.get(data + info_data_offset, 0, bound_id.size());
		info_data_offset += bound_id.size();
	}
	// IMAGEKEY_ORIGINAL_BASE_ADDRESS		0x00010001
	if(m_pXex->hasOriginalLoadAddress())
	{
		info_header.key = IMAGEKEY_ORIGINAL_BASE_ADDRESS;
		info_header.value = m_pXex->getOriginalLoadAddress();
		convertImageEntryHeaderToBE(info_header);
		*headers_ptr++ = info_header;
	}
	// IMAGEKEY_ENTRY_POINT					0x00010100
	if(m_pXex->hasEntryPoint())
	{
		info_header.key = IMAGEKEY_ENTRY_POINT;
		info_header.value = m_pXex->getEntryPoint();
		convertImageEntryHeaderToBE(info_header);
		*headers_ptr++ = info_header;
	}
	// IMAGEKEY_IMAGE_BASE_ADDRESS			0x00010201
	if(m_pXex->isBasefilePE())
	{
		info_header.key = IMAGEKEY_IMAGE_BASE_ADDRESS;
		info_header.value = m_pXex->getLoadAddress();
		convertImageEntryHeaderToBE(info_header);
		*headers_ptr++ = info_header;
	}
	// IMAGEKEY_IMPORT_LIBRARIES			0x000103FF
	if(m_pXex->numImportLibraries())
	{
		DataBlock import_libs;
		const s32 names_size = 1024;
		char* names = new char[names_size];
		char* name_ptr = names;
		memset(names, 0, names_size);
		XexVersion32 version;
		XexVersion32 min_version;
		DataBlock addresses;
		u32 module_number;
		u8  module_index;
		
		// we need to work out the name size first
		// we'll also get all the names here
		s32 lib_idx_handled = -1;
		s32 name_size = 0;
		for(s32 lib_num=0; lib_num<m_pXex->numImportLibraries(); lib_num++)
		{
			if( m_pXex->getImportLibrary(lib_num, name_ptr, version, min_version, addresses, module_number, module_index) )
			{
				// add import libraries name sizes
				if( module_index > lib_idx_handled )
				{
					name_size += ((s32)strlen(name_ptr) + 4) & (-4);
					name_ptr = names + name_size;
					lib_idx_handled = module_index;
				}
			}
		}
		
		// do the import libraries header
		import_libs.set32(0, offsetof(ImportLibraryDirectory, infoSize));	// fill in the actual size last
		import_libs.set32(name_size, offsetof(ImportLibraryDirectory, nameTableSize));
		import_libs.set32(lib_idx_handled+1, offsetof(ImportLibraryDirectory, numNames));
		import_libs.set(names, offsetof(ImportLibraryDirectory, names), name_size);
		
		// now do each import library
		s32 lib_offset = 12 + name_size;
		for(s32 lib_num=0; lib_num<m_pXex->numImportLibraries(); lib_num++)
		{
			if( !m_pXex->getImportLibrary(lib_num, names, version, min_version, addresses, module_number, module_index) )
				break;
			s32 num_addresses = addresses.size() / 4;
			s32 lib_size = sizeof(ImportLibraryEntry) - 4 + addresses.size();
			import_libs.set32(lib_size,				lib_offset + offsetof(ImportLibraryEntry, infoSize));
			import_libs.set32(module_number,		lib_offset + offsetof(ImportLibraryEntry, moduleNumber));
			import_libs.set32(*(u32*)&version,		lib_offset + offsetof(ImportLibraryEntry, version));
			import_libs.set32(*(u32*)&min_version,	lib_offset + offsetof(ImportLibraryEntry, minVersion));
			import_libs.set8( 0,					lib_offset + offsetof(ImportLibraryEntry, reserved));
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
	if(m_pXex->hasChecksum())
	{
		DataBlock checksum;
		checksum.set32(m_pXex->getChecksum(), 0);
		checksum.set32(m_pXex->getFiletime(), 4);
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
	if(m_pXex->hasCallCap())
	{
		u32 addr1, addr2;
		DataBlock callcap;
		m_pXex->getCallCap(addr1, addr2);
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
	if(m_pXex->hasFastCap())
	{
		u32 fastcap;
		m_pXex->getFastCap(fastcap);
		info_header.key = IMAGEKEY_IMAGE_FASTCAP;
		info_header.value = fastcap;
		convertImageEntryHeaderToBE(info_header);
		*headers_ptr++ = info_header;
	}
	// IMAGEKEY_ORIGINAL_PE_NAME			0x000183FF
	if(m_pXex->hasOriginalPEName())
	{
		DataBlock name;
		const s32 name_str_size = 1024;
		char* name_str = new char[name_str_size];
		memset(name_str, 0, name_str_size);
		m_pXex->getOriginalPEName(name_str, name_str_size);
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
	if(m_pXex->numLibraryVersions())
	{
		DataBlock lib_ver;
		for(s32 lib_num=0; lib_num<m_pXex->numLibraryVersions(); lib_num++)
		{
			char name[12];
			memset(name, 0, 12);
			XexVersionInfo version;
			if( m_pXex->getLibraryVersion(lib_num, name, version) )
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
	if(m_pXex->hasTLSInfo())
	{
		TLSInfo tls_info;
		DataBlock tls;
		m_pXex->getTLSInfo(tls_info);
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
	if(m_pXex->hasStackSize())
	{
		s32 stacksize;
		m_pXex->getStackSize(stacksize);
		info_header.key = IMAGEKEY_STACK_SIZE;
		info_header.value = stacksize;
		convertImageEntryHeaderToBE(info_header);
		*headers_ptr++ = info_header;
	}
	// IMAGEKEY_FILESYSTEM_CACHE_SIZE		0x00020301
	if(m_pXex->hasFilesystemCacheSize())
	{
		s32 size;
		m_pXex->getFilesystemCacheSize(size);
		info_header.key = IMAGEKEY_FILESYSTEM_CACHE_SIZE;
		info_header.value = size;
		convertImageEntryHeaderToBE(info_header);
		*headers_ptr++ = info_header;
	}
	// IMAGEKEY_HEAP_SIZE					0x00020401
	if(m_pXex->hasHeapSize())
	{
		s32 size;
		m_pXex->getHeapSize(size);
		info_header.key = IMAGEKEY_HEAP_SIZE;
		info_header.value = size;
		convertImageEntryHeaderToBE(info_header);
		*headers_ptr++ = info_header;
	}
	// IMAGEKEY_PAGE_HEAP					0x00028002
	if(m_pXex->hasPageHeapInfo())
	{
		DataBlock page_info;
		u32 size, flags;
		m_pXex->getPageHeapInfo(size, flags);
		page_info.set32(size, 0);
		page_info.set32(flags, 4);
		// fix and set values
		info_header.key = IMAGEKEY_PAGE_HEAP;
		info_header.value = dataOffset + info_data_offset;
		convertImageEntryDataToBE(info_header, page_info);
		convertImageEntryHeaderToBE(info_header);
		*headers_ptr++ = info_header;
		page_info.get(data + info_data_offset, 0, page_info.size());
		info_data_offset += page_info.size();
	}
	// IMAGEKEY_SYSTEM_FLAGS				0x00030000
	if(m_pXex->hasSystemFlags())
	{
		u32 sys_flags = 0;
		SET_FLAG(m_pXex->isNoForcedReboot(),			sys_flags, SYSFLAG_NO_FORCE_REBOOT);
		SET_FLAG(m_pXex->isForegroundTasks(),			sys_flags, SYSFLAG_FOREGROUND_TASKS);
		SET_FLAG(m_pXex->isNoODDMapping(),				sys_flags, SYSFLAG_NO_ODD_MAPPING);
		SET_FLAG(m_pXex->isMceInputHandler(),			sys_flags, SYSFLAG_HANDLE_MCE_INPUT);
		SET_FLAG(m_pXex->isRestrictedHudFeatures(),		sys_flags, SYSFLAG_RESTRICT_HUD_FEATURES);
		SET_FLAG(m_pXex->isGamepadDisconnectHandler(),	sys_flags, SYSFLAG_HANDLE_GAMEPAD_DISCONNECT);
		SET_FLAG(m_pXex->isInsecureSockets(),			sys_flags, SYSFLAG_INSECURE_SOCKETS);
		SET_FLAG(m_pXex->isXbox1Interoperability(),		sys_flags, SYSFLAG_XBOX_1_XSP_INTEROP);
		SET_FLAG(m_pXex->isDashContext(),				sys_flags, SYSFLAG_SET_DASH_CONTEXT);
		SET_FLAG(m_pXex->isGameVoiceChannelUser(),		sys_flags, SYSFLAG_USES_GAME_VOICE_CHANNEL);
		SET_FLAG(m_pXex->isPal50Incompatible(),			sys_flags, SYSFLAG_PAL50_INCOMPATIBLE);
		SET_FLAG(m_pXex->isInsecureUtilDriveUser(),		sys_flags, SYSFLAG_INSECURE_UTILITYDRIVE);
		SET_FLAG(m_pXex->isXamHooks(),					sys_flags, SYSFLAG_HAS_XAM_HOOKS);
		SET_FLAG(m_pXex->isPII(),						sys_flags, SYSFLAG_PII);
		SET_FLAG(m_pXex->isCrossPlatformSyslinkUser(),	sys_flags, SYSFLAG_CROSSPLATFORM_SYSLINK);
		SET_FLAG(m_pXex->isMultidiscSwap(),				sys_flags, SYSFLAG_MULTIDISC_SWAP);
		SET_FLAG(m_pXex->isMultidiscInsecureMedia(),	sys_flags, SYSFLAG_MULTIDISC_INSECURE_MEDIA);
		SET_FLAG(m_pXex->isAP25Media(),					sys_flags, SYSFLAG_AP25_MEDIA);
		SET_FLAG(m_pXex->isNoCofirmExit(),				sys_flags, SYSFLAG_NO_CONFIRM_EXIT);
		SET_FLAG(m_pXex->isAllowBackgroundDownload(),	sys_flags, SYSFLAG_ALLOW_BKGRND_DOWNLOAD);
		SET_FLAG(m_pXex->isCreatePersistRamdrive(),		sys_flags, SYSFLAG_CREATE_PERSIST_RAMDRIVE);
		SET_FLAG(m_pXex->isInheritPersistRamdrive(),	sys_flags, SYSFLAG_INHERIT_PERSIST_RAMDRIVE);
		SET_FLAG(m_pXex->isAllowHudVibration(),			sys_flags, SYSFLAG_ALLOW_HUD_VIBRATION);
		SET_FLAG(m_pXex->isBothUtilityPartitions(),		sys_flags, SYSFLAG_BOTH_UTILITY_PARTITIONS);
		SET_FLAG(m_pXex->isIptvInputHandler(),			sys_flags, SYSFLAG_HANDLE_IPTV_INPUT);
		SET_FLAG(m_pXex->isPreferBigButtonInput(),		sys_flags, SYSFLAG_PREFER_BIGBUTTON_INPUT);
		SET_FLAG(m_pXex->isAllowXsamReservation(),		sys_flags, SYSFLAG_ALLOW_XSAM_RESERVATION);
		SET_FLAG(m_pXex->isMultiDiscCrossTitle(),		sys_flags, SYSFLAG_MULTIDISC_CROSS_TITLE);
		SET_FLAG(m_pXex->isTitleInstallIncompatible(),	sys_flags, SYSFLAG_TITLE_INSTALL_INCOMPATIBLE);
		SET_FLAG(m_pXex->isAllowAvatarGetMetadata(),	sys_flags, SYSFLAG_ALLOW_AVATAR_GET_METADATA);
		SET_FLAG(m_pXex->isAllowControllerSwapping(),	sys_flags, SYSFLAG_ALLOW_CONTROLLER_SWAPPING);
		SET_FLAG(m_pXex->isDashExtensibilityModule(),	sys_flags, SYSFLAG_DASH_EXTENSIBILITY_MODULE);

		// fix and set values for sysflags
		sys_flags  |= m_pXex->getUnknownSystemFlags();
		info_header.key = IMAGEKEY_SYSTEM_FLAGS;
		info_header.value = sys_flags;
		convertImageEntryHeaderToBE(info_header);
		*headers_ptr++ = info_header;
	}
	// IMAGEKEY_SYSTEM_FLAGS2				0x00030100
	if(m_pXex->hasSystemFlags2())
	{
		u32 sys_flags2 = 0;
		SET_FLAG(m_pXex->isAllowNetworkReadCancel(),	sys_flags2, SYSFLAG2_ALLOW_NETWORK_READ_CANCEL);
		SET_FLAG(m_pXex->isUninterruptableReads(),		sys_flags2, SYSFLAG2_UNINTERRUPTABLE_READS);
		SET_FLAG(m_pXex->isRequiresNXE(),				sys_flags2, SYSFLAG2_REQUIRE_FULL_EXPERIENCE);
		SET_FLAG(m_pXex->isGamevoiceRequiredUI(),		sys_flags2, SYSFLAG2_GAMEVOICE_REQUIRED_UI);
		SET_FLAG(m_pXex->isTitleSetsPresenceString(),	sys_flags2, SYSFLAG2_TITLE_SET_PRESENCE_STRING);
		SET_FLAG(m_pXex->isNatalTiltControl(),			sys_flags2, SYSFLAG2_NATAL_TILTCONTROL);
		SET_FLAG(m_pXex->isSkeletalTrackingSupported(),	sys_flags2, SYSFLAG2_REQUIRES_SKELETAL_TRACKING);
		SET_FLAG(m_pXex->isSkeletalTrackingRequired(),	sys_flags2, SYSFLAG2_SUPPORTS_SKELETAL_TRACKING);
		SET_FLAG(m_pXex->isLargeHdsFileCacheUsed(),		sys_flags2, SYSFLAG2_USE_LARGE_HDS_FILE_CACHE);
		SET_FLAG(m_pXex->isTitleSupportsDeepLink(),		sys_flags2, SYSFLAG2_TITLE_SUPPORTS_DEEP_LINK);
		
		// fix and set values for sysflags2
		sys_flags2 |= m_pXex->getUnknownSystemFlags2();
		info_header.key = IMAGEKEY_SYSTEM_FLAGS2;
		info_header.value = sys_flags2;
		convertImageEntryHeaderToBE(info_header);
		*headers_ptr++ = info_header;
	}
	// IMAGEKEY_EXECUTION_ID				0x00040006
	if(m_pXex->hasExecutionId())
	{
		ExecutionId exec_id;
		DataBlock exec;
		m_pXex->getExecutionId(exec_id);
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
	if(m_pXex->hasWorkspaceSize())
	{
		s32 size;
		m_pXex->getWorkspaceSize(size);
		info_header.key = IMAGEKEY_TITLE_WORKSPACE_SIZE;
		info_header.value = size;
		convertImageEntryHeaderToBE(info_header);
		*headers_ptr++ = info_header;
	}
	// IMAGEKEY_GAME_RATINGS				0x00040310
	if(m_pXex->hasGameRatings())
	{
		GameRatings game_ratings;
		DataBlock ratings;
		m_pXex->getGameRatings(game_ratings);
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
	// IMAGEKEY_LAN_KEY						0x00040404
	if(m_pXex->hasLANKey())
	{
		LANKey lan_key;
		DataBlock lan;
		m_pXex->getLANKey(lan_key);
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
	if(m_pXex->hasLogoData())
	{
		// get logo pixel data
		DataBlock pixel_data;
		m_pXex->getLogoData(pixel_data);
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
	// IMAGEKEY_MULTIDISC_MEDIA_IDS			0x000406FF
	if(m_pXex->numMultidiscMediaIds())
	{
		DataBlock ids;
		for(s32 id_num=0; id_num<m_pXex->numMultidiscMediaIds(); id_num++)
		{
			s32 offset = 4 + id_num * sizeof(MediaId);
			MediaId media_id;
			m_pXex->getMultidiscMediaId(id_num, media_id);
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
	// IMAGEKEY_ALT_TITLE_IDS				0x000407FF
	if(m_pXex->numAltTitleIds())
	{
		DataBlock ids;
		for(s32 id_num=0; id_num<m_pXex->numAltTitleIds(); id_num++)
		{
			s32 offset = 4 + id_num * sizeof(u32);
			u32 title_id;
			m_pXex->getAltTitleId(id_num, title_id);
			ids.set(&title_id, offset, sizeof(title_id));
		}
		ids.set32(ids.size(), 0);
		// fix and set values
		info_header.key = IMAGEKEY_ALT_TITLE_IDS;
		info_header.value = dataOffset + info_data_offset;
		convertImageEntryDataToBE(info_header, ids);
		convertImageEntryHeaderToBE(info_header);
		*headers_ptr++ = info_header;
		ids.get(data + info_data_offset, 0, ids.size());
		info_data_offset += ids.size();
	}
	// IMAGEKEY_EXTRA_DEBUG_MEMORY			0x00040801
	if(m_pXex->hasExtraDebugMemory())
	{
		u32 size;
		m_pXex->getExtraDebugMemory(size);
		info_header.key = IMAGEKEY_EXTRA_DEBUG_MEMORY;
		info_header.value = size;
		convertImageEntryHeaderToBE(info_header);
		*headers_ptr++ = info_header;
	}
	// IMAGEKEY_EXPORTS_BY_NAME				0x00E10402
	if(m_pXex->hasExportsByName())
	{
		ExportsByName exports_by_name;
		DataBlock exports;
		m_pXex->getExportsByName(exports_by_name);
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
	for(s32 i=0; i<m_pXex->numUnknownImageEntries(); i++)
	{
		XexImageEntry info_header;
		DataBlock info_data;
		if( m_pXex->getUnknownImageEntry(i, info_header, info_data) )
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
bool XexHeader::addImageEntry(const XexImageEntry& header, const DataBlock& data)
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
			m_pXex->addResource(resource.addr, resource.size, name);
		}
		break;
	
	case 0x000003FF:		// IMAGEKEY_BASEFILE_FORMAT
		type = data.get16(offsetof(BaseFileInfoHeader, encType));
		m_pXex->setEncrypted( type == 1 );
		type = data.get16(offsetof(BaseFileInfoHeader, compType));
		if(		type == 1)	m_pXex->setRaw();
		else if(type == 2)	m_pXex->setCompressed();
		else if(type == 3)	m_pXex->setDeltaCompressed();
		else				m_pXex->setBinary();
		// store basefile info required to unpack file
		m_basefileFormat = data;
		break;
	
	case 0x00000405:		// IMAGEKEY_BASE_REFERENCE
	{
		u8 ref[20];
		data.get(ref, 0, 20);
		m_pXex->setBaseReference(ref);
		break;
	}
	
	case 0x000005FF:		// IMAGEKEY_DELTA_PATCH_DESCRIPTOR
	{
		DataBlock patch_headers;
		data.get(patch_headers, 0, 0, data.size());
		m_pXex->setDeltaPatchDescriptor(patch_headers);
		break;
	}
	
	case 0x00004004:		// IMAGEKEY_RESTRICT_KV_PRIVS
	{
		u64 mask, val;
		mask = data.get64(0);
		val  = data.get64(8);
		m_pXex->setRestrictKVPrivs(mask, val);
		break;
	}
	
	case 0x00004104:		// IMAGEKEY_RESTRICT_DATES
	{
		u64 start, end;
		start = data.get64(0);
		end  = data.get64(8);
		m_pXex->setRestrictDates(start, end);
		break;
	}
	
	case 0x000042FF:		// IMAGEKEY_RESTRICT_CONSOLE_IDS
	{
		s32 num_ids = data.get32(0);
		for(s32 i=0; i<num_ids; i++)
		{
			ConsoleId console_id;
			data.get(console_id.id, 4 + i*sizeof(ConsoleId), sizeof(ConsoleId));
			m_pXex->addRestrictConsoleId(console_id);
		}
		break;
	}
	
	case 0x00004304:		// IMAGEKEY_DISC_PROFILE_ID
	{
		DiscProfileId disc_profile_id;
		data.get(&disc_profile_id, 0, sizeof(DiscProfileId));
		m_pXex->setDiscProfileId(disc_profile_id);
		break;
	}
	
	case 0x000080FF:		// IMAGEKEY_BOUND_PATHNAME
	{
		s32 str_len = data.size() - offsetof(BoundPathname, pathname);
		char* bound_path = new char[str_len];
		data.get(bound_path, offsetof(BoundPathname, pathname), str_len);
		m_pXex->setBoundingPath(bound_path);
		delete[] bound_path;
		break;
	}
	
	case 0x00008105:		// IMAGEKEY_BOUND_DEVICE_ID
	{
		u8 device_id[20];
		data.get(device_id, 0, 20);
		m_pXex->setBoundingDeviceId(device_id);
		break;
	}
	
	case 0x00010001:		// IMAGEKEY_ORIGINAL_BASE_ADDRESS
		// ignore this as it can be calculated from the basefile
		// on second thought, store a flag as to whether to write
		// out an image entry for this in the header.
		m_pXex->setHasOriginalLoadAddress(true);
		break;
	
	case 0x00010100:		// IMAGEKEY_ENTRY_POINT
		// ignore this as it can be calculated from the basefile
		break;
	
	case 0x00010201:		// IMAGEKEY_IMAGE_BASE_ADDRESS
		// ignore this as its a field in XexSecurityInfo
		break;
	
	case 0x000103FF:		// IMAGEKEY_IMPORT_LIBRARIES
	{
		offset = offsetof(ImportLibraryDirectory, names) + data.get32(offsetof(ImportLibraryDirectory, nameTableSize));
		char* names = new char[data.get32(offsetof(ImportLibraryDirectory, nameTableSize))];
		data.get(names, offsetof(ImportLibraryDirectory, names), data.get32(offsetof(ImportLibraryDirectory, nameTableSize)));
		for(s32 lib_num=0; offset<info_size; lib_num++)
		{
			u8  module_index = data.get8(offset + offsetof(ImportLibraryEntry, moduleIndex));
			char* name = names;
			for(s32 i=0; i<module_index; i++)
			{
				s32 len = ((s32)strlen(name) + 1 + 3) & (-4);
				name += len;
			}
			u32 module_number = data.get32(offset + offsetof(ImportLibraryEntry, moduleNumber));
			u32 ver = data.get32(offset + offsetof(ImportLibraryEntry, version));
			XexVersion32 version = *(XexVersion32*)&ver;
			ver = data.get32(offset + offsetof(ImportLibraryEntry, minVersion));
			XexVersion32 min_version = *(XexVersion32*)&ver;
			DataBlock addresses;
			s32 num_addresses = data.get16(offset + offsetof(ImportLibraryEntry, numAddresses));
			data.get(addresses, 0, offset + offsetof(ImportLibraryEntry, addresses), num_addresses * 4);
			m_pXex->addImportLibrary(name, version, min_version, addresses, module_number, module_index);
			
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
		m_pXex->setCallCap(addr1, addr2);
		break;
	}
	
	case 0x00018200:		// IMAGEKEY_IMAGE_FASTCAP
		m_pXex->setFastCap(header.value);
		break;
	
	case 0x000183FF:		// IMAGEKEY_ORIGINAL_PE_NAME
	{
		s32 str_len = data.size() - offsetof(OriginalPEName, name);
		char* name = new char[str_len];
		data.get(name, offsetof(OriginalPEName, name), str_len);
		m_pXex->setOriginalPEName(name);
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
			m_pXex->addLibraryVersion(name, block.version);
		}
		break;
	}
	
	case 0x00020104:		// IMAGEKEY_TLS_VALUES
	{
		TLSInfo tls;
		data.get(&tls, 0, sizeof(TLSInfo));
		m_pXex->setTLSInfo(tls);
		break;
	}
	
	case 0x00020200:		// IMAGEKEY_STACK_SIZE
		m_pXex->setStackSize(header.value);
		break;
	
	case 0x00020301:		// IMAGEKEY_FILESYSTEM_CACHE_SIZE
		m_pXex->setFilesystemCacheSize(header.value);
		break;
	
	case 0x00020401:		// IMAGEKEY_HEAP_SIZE
		m_pXex->setHeapSize(header.value);
		break;
	
	case 0x00028002:		// IMAGEKEY_PAGE_HEAP
	{
		u32 size, flags;
		size = data.get32(0);
		flags  = data.get32(4);
		m_pXex->setPageHeapInfo(size, flags);
		break;
	}
	
	case 0x00030000:		// IMAGEKEY_SYSTEM_FLAGS
	{
		u32 sflags = header.value;
		m_pXex->setNoForcedReboot(			IS_FLAG_SET(sflags, SYSFLAG_NO_FORCE_REBOOT) );
		m_pXex->setForegroundTasks(			IS_FLAG_SET(sflags, SYSFLAG_FOREGROUND_TASKS) );
		m_pXex->setNoODDMapping(			IS_FLAG_SET(sflags, SYSFLAG_NO_ODD_MAPPING) );
		m_pXex->setMceInputHandler(			IS_FLAG_SET(sflags, SYSFLAG_HANDLE_MCE_INPUT) );
		m_pXex->setRestrictedHudFeatures(	IS_FLAG_SET(sflags, SYSFLAG_RESTRICT_HUD_FEATURES) );
		m_pXex->setGamepadDisconnectHandler(IS_FLAG_SET(sflags, SYSFLAG_HANDLE_GAMEPAD_DISCONNECT) );
		m_pXex->setInsecureSockets(			IS_FLAG_SET(sflags, SYSFLAG_INSECURE_SOCKETS) );
		m_pXex->setXbox1Interoperability(	IS_FLAG_SET(sflags, SYSFLAG_XBOX_1_XSP_INTEROP) );
		m_pXex->setDashContext(				IS_FLAG_SET(sflags, SYSFLAG_SET_DASH_CONTEXT) );
		m_pXex->setGameVoiceChannelUser(	IS_FLAG_SET(sflags, SYSFLAG_USES_GAME_VOICE_CHANNEL) );
		m_pXex->setPal50Incompatible(		IS_FLAG_SET(sflags, SYSFLAG_PAL50_INCOMPATIBLE) );
		m_pXex->setInsecureUtilDriveUser(	IS_FLAG_SET(sflags, SYSFLAG_INSECURE_UTILITYDRIVE) );
		m_pXex->setXamHooks(				IS_FLAG_SET(sflags, SYSFLAG_HAS_XAM_HOOKS) );
		m_pXex->setPII(						IS_FLAG_SET(sflags, SYSFLAG_PII) );
		m_pXex->setCrossPlatformSyslinkUser(IS_FLAG_SET(sflags, SYSFLAG_CROSSPLATFORM_SYSLINK) );
		m_pXex->setMultidiscSwap(			IS_FLAG_SET(sflags, SYSFLAG_MULTIDISC_SWAP) );
		m_pXex->setMultidiscInsecureMedia(	IS_FLAG_SET(sflags, SYSFLAG_MULTIDISC_INSECURE_MEDIA) );
		m_pXex->setAP25Media(				IS_FLAG_SET(sflags, SYSFLAG_AP25_MEDIA) );
		m_pXex->setNoCofirmExit(			IS_FLAG_SET(sflags, SYSFLAG_NO_CONFIRM_EXIT) );
		m_pXex->setAllowBackgroundDownload(	IS_FLAG_SET(sflags, SYSFLAG_ALLOW_BKGRND_DOWNLOAD) );
		m_pXex->setCreatePersistRamdrive(	IS_FLAG_SET(sflags, SYSFLAG_CREATE_PERSIST_RAMDRIVE) );
		m_pXex->setInheritPersistRamdrive(	IS_FLAG_SET(sflags, SYSFLAG_INHERIT_PERSIST_RAMDRIVE) );
		m_pXex->setAllowHudVibration(		IS_FLAG_SET(sflags, SYSFLAG_ALLOW_HUD_VIBRATION) );
		m_pXex->setBothUtilityPartitions(	IS_FLAG_SET(sflags, SYSFLAG_BOTH_UTILITY_PARTITIONS) );
		m_pXex->setIptvInputHandler(		IS_FLAG_SET(sflags, SYSFLAG_HANDLE_IPTV_INPUT) );
		m_pXex->setPreferBigButtonInput(	IS_FLAG_SET(sflags, SYSFLAG_PREFER_BIGBUTTON_INPUT) );
		m_pXex->setAllowXsamReservation(	IS_FLAG_SET(sflags, SYSFLAG_ALLOW_XSAM_RESERVATION) );
		m_pXex->setMultiDiscCrossTitle(		IS_FLAG_SET(sflags, SYSFLAG_MULTIDISC_CROSS_TITLE) );
		m_pXex->setTitleInstallIncompatible(IS_FLAG_SET(sflags, SYSFLAG_TITLE_INSTALL_INCOMPATIBLE) );
		m_pXex->setAllowAvatarGetMetadata(	IS_FLAG_SET(sflags, SYSFLAG_ALLOW_AVATAR_GET_METADATA) );
		m_pXex->setAllowControllerSwapping(	IS_FLAG_SET(sflags, SYSFLAG_ALLOW_CONTROLLER_SWAPPING) );
		m_pXex->setDashExtensibilityModule(	IS_FLAG_SET(sflags, SYSFLAG_DASH_EXTENSIBILITY_MODULE) );
		m_pXex->setUnknownSystemFlags(sflags & SYSFLAG_UNKNOWN);
		break;
	}
	
	case 0x00030100:		// IMAGEKEY_SYSTEM_FLAGS2
	{
		u32 sflags = header.value;
		m_pXex->setAllowNetworkReadCancel(	IS_FLAG_SET(sflags, SYSFLAG2_ALLOW_NETWORK_READ_CANCEL) );
		m_pXex->setUninterruptableReads(	IS_FLAG_SET(sflags, SYSFLAG2_UNINTERRUPTABLE_READS) );
		m_pXex->setRequiresNXE(				IS_FLAG_SET(sflags, SYSFLAG2_REQUIRE_FULL_EXPERIENCE) );
		m_pXex->setGamevoiceRequiredUI(		IS_FLAG_SET(sflags, SYSFLAG2_GAMEVOICE_REQUIRED_UI) );
		m_pXex->setTitleSetsPresenceString(	IS_FLAG_SET(sflags, SYSFLAG2_TITLE_SET_PRESENCE_STRING) );
		m_pXex->setNatalTiltControl(		IS_FLAG_SET(sflags, SYSFLAG2_NATAL_TILTCONTROL) );
		m_pXex->setSkeletalTrackingSupported(IS_FLAG_SET(sflags,SYSFLAG2_REQUIRES_SKELETAL_TRACKING) );
		m_pXex->setSkeletalTrackingRequired(IS_FLAG_SET(sflags, SYSFLAG2_SUPPORTS_SKELETAL_TRACKING) );
		m_pXex->setLargeHdsFileCacheUsed(	IS_FLAG_SET(sflags, SYSFLAG2_USE_LARGE_HDS_FILE_CACHE) );
		m_pXex->setTitleSupportsDeepLink(	IS_FLAG_SET(sflags, SYSFLAG2_TITLE_SUPPORTS_DEEP_LINK) );
		m_pXex->setUnknownSystemFlags2(sflags & SYSFLAG2_UNKNOWN);
		break;
	}
	
	case 0x00040006:		// IMAGEKEY_EXECUTION_ID
	{
		ExecutionId exec_id;
		data.get(&exec_id, 0, sizeof(ExecutionId));
		m_pXex->setExecutionId(exec_id);
		break;
	}
	
	case 0x00040201:		// IMAGEKEY_TITLE_WORKSPACE_SIZE
		m_pXex->setWorkspaceSize(header.value);
		break;
	
	case 0x00040310:		// IMAGEKEY_GAME_RATINGS
	{
		GameRatings ratings;
		data.get(&ratings, 0, sizeof(GameRatings));
		m_pXex->setGameRatings(ratings);
		break;
	}
	
	case 0x00040404:		// IMAGEKEY_LAN_KEY
	{
		LANKey lan_key;
		data.get(&lan_key, 0, sizeof(LANKey));
		m_pXex->setLANKey(lan_key);
		break;
	}
	
	case 0x000405FF:		// IMAGEKEY_LOGO_DATA
	{
		DataBlock logo_data;
//		data.get(logo_data, 0, offsetof(LogoData, logoSize), info_size -  offsetof(LogoData, logoSize));
		data.get(logo_data, 0, offsetof(LogoData, logoData), info_size -  offsetof(LogoData, logoData));
		m_pXex->setLogoData(logo_data);
		break;
	}
	
	case 0x000406FF:		// IMAGEKEY_MULTIDISC_MEDIA_IDS
	{
		s32 num_media_ids = (info_size - offsetof(MultidiscMediaIds, mediaId)) / sizeof(MediaId);
		for(s32 i=0; i<num_media_ids; i++)
		{
			MediaId media_id;
			data.get(&media_id, offsetof(MultidiscMediaIds, mediaId) + i * sizeof(MediaId), sizeof(MediaId));
			m_pXex->addMultidiscMediaId(media_id);
		}
		break;
	}
	
	case 0x000407FF:		// IMAGEKEY_ALT_TITLE_IDS
	{
		s32 num_title_ids = (info_size - 4) / sizeof(u32);
		for(s32 i=0; i<num_title_ids; i++)
		{
			u32 title_id;
			data.get(&title_id, 4 + i*sizeof(title_id), sizeof(title_id));
			m_pXex->addAltTitleId(title_id);
		}
		break;
	}
	
	case 0x00040801:		// IMAGEKEY_EXTRA_DEBUG_MEMORY
	{
		m_pXex->setExtraDebugMemory(header.value);
		break;
	}
	
	case 0x00E10402:		// IMAGEKEY_EXPORTS_BY_NAME
	{
		ExportsByName exports;
		data.get(&exports, 0, sizeof(ExportsByName));
		m_pXex->setExportsByName(exports);
		break;
	}
	
	default:
		m_pXex->addUnknownImageEntry(header, data);
		break;
	}
	
	return true;
}


bool XexHeader::encKey(XexKey& dataKey, const XexKey& cryptKey)
{
	XeAesContext aes_ctx;
	XeCryptAesKey(&aes_ctx, cryptKey.data);
	XeCryptAesEcb(&aes_ctx, dataKey.data, dataKey.data, XE_CRYPT_ENC);
	return true;
}
bool XexHeader::encRetailKey(XexKey& dataKey)
{
	XexKey retail_key;
	memcpy(retail_key.data, XexData::XEX_RETAIL_KEY, sizeof(XexKey));
	return encKey(dataKey, retail_key);
}
bool XexHeader::encDebugKey(XexKey& dataKey)
{
	XexKey debug_key;
	memcpy(debug_key.data, XexData::XEX_DEBUG_KEY, sizeof(XexKey));
	return encKey(dataKey, debug_key);
}
bool XexHeader::encMfgRetailKey(XexKey& dataKey)
{
	XexKey mfg_retail_key;
	memcpy(mfg_retail_key.data, XexData::XEX_MFG_RETAIL_KEY, sizeof(XexKey));
	return encKey(dataKey, mfg_retail_key);
}
bool XexHeader::encMfgDebugKey(XexKey& dataKey)
{
	XexKey mfg_debug_key;
	memcpy(mfg_debug_key.data, XexData::XEX_MFG_DEBUG_KEY, sizeof(XexKey));
	return encKey(dataKey, mfg_debug_key);
}
bool XexHeader::encKey(XexKey& dataKey, bool isRetail, bool isMfg)
{
	if( isMfg )
	{
		// manufacturing mode
		if( !isRetail )
			return encMfgDebugKey(dataKey);
		else
			return encMfgRetailKey(dataKey);
	}
	else
	{
		// normal mode
		if( !isRetail )
			return encDebugKey(dataKey);
		else
			return encRetailKey(dataKey);
	}
}

bool XexHeader::decKey(XexKey& dataKey, const XexKey& cryptKey)
{
	XeAesContext aes_ctx;
	XeCryptAesKey(&aes_ctx, cryptKey.data);
	XeCryptAesEcb(&aes_ctx, dataKey.data, dataKey.data, XE_CRYPT_DEC);
	return true;
}
bool XexHeader::decRetailKey(XexKey& dataKey)
{
	XexKey retail_key;
	memcpy(retail_key.data, XexData::XEX_RETAIL_KEY, sizeof(XexKey));
	return decKey(dataKey, retail_key);
}
bool XexHeader::decDebugKey(XexKey& dataKey)
{
	XexKey debug_key;
	memcpy(debug_key.data, XexData::XEX_DEBUG_KEY, sizeof(XexKey));
	return decKey(dataKey, debug_key);
}
bool XexHeader::decMfgRetailKey(XexKey& dataKey)
{
	XexKey mfg_retail_key;
	memcpy(mfg_retail_key.data, XexData::XEX_MFG_RETAIL_KEY, sizeof(XexKey));
	return decKey(dataKey, mfg_retail_key);
}
bool XexHeader::decMfgDebugKey(XexKey& dataKey)
{
	XexKey mfg_debug_key;
	memcpy(mfg_debug_key.data, XexData::XEX_MFG_DEBUG_KEY, sizeof(XexKey));
	return decKey(dataKey, mfg_debug_key);
}
bool XexHeader::decKey(XexKey& dataKey, bool isRetail, bool isMfg)
{
	if( isMfg )
	{
		// manufacturing mode
		if( !isRetail )
			return decMfgDebugKey(dataKey);
		else
			return decMfgRetailKey(dataKey);
	}
	else
	{
		// normal mode
		if( !isRetail )
			return decDebugKey(dataKey);
		else
			return decRetailKey(dataKey);
	}
}


// signs xex file with debug key
bool XexHeader::updateSign(XexSecurityInfo& secInfo, const u8* publicKey, const u8* privateKey)
{
	u8* sig_ptr = secInfo.imageInfo.signature;
	s32 sig_size = 0x100;
	s32 hash_size = GET32BE(&secInfo.imageInfo.infoSize) - sig_size;
	XexHash hash;
	XeCryptRotSumSha(sig_ptr+sig_size, hash_size,
		0, 0,
		hash.data, sizeof(XexHash));
	
	const u8* salt;
	if( m_pXex->hasRequiredRevocationCheck() )
		salt = XexData::XEX_SALT_REV;
	else
		salt = XexData::XEX_SALT_XEX;
	XeCryptBnQwBeSigCreate((u64*)sig_ptr, hash.data, salt, (XeRsaKey*)publicKey);
	if( !XeCryptBnQwNeModExp((u64*)sig_ptr, (u64*)sig_ptr, (u64*)(privateKey+0x390), (u64*)(privateKey+0x10), 0x20) )
		return false;
	memcpy(secInfo.imageInfo.signature, sig_ptr, sig_size);
	return true;
}
bool XexHeader::updateRetailSign(XexSecurityInfo& secInfo)
{
	// dont have retail private key, so can't sign it
	//return updateSign(secInfo, XexData::XEX_RETAIL_PUBLIC_KEY, XexData::XEX_RETAIL_PRIVATE_KEY);
	
	memset(secInfo.imageInfo.signature, 0, sizeof(secInfo.imageInfo.signature));
	return false;
}
bool XexHeader::updateDebugSign(XexSecurityInfo& secInfo)
{
	return updateSign(secInfo, XexData::XEX_DEBUG_PUBLIC_KEY, XexData::XEX_DEBUG_PRIVATE_KEY);
}
// checks that signature is valid
bool XexHeader::verifySign(const u8* publicKey, const XexSecurityInfo& secInfo)
{
	const u8* sig_ptr = secInfo.imageInfo.signature;
	s32 sig_size = 0x100;
	s32 hash_size = GET32BE(&secInfo.imageInfo.infoSize) - sig_size;
	u8 hash[20];
	XeCryptRotSumSha(sig_ptr+sig_size, hash_size,
		0, 0,
		hash, 20);
	
	// the salt used for signatures depends on whether revocation is set
	const u8* salt;
	u32 image_flags = GET32BE(&secInfo.imageInfo.imageFlags);
	if( IS_FLAG_SET(image_flags, IMAGEFLAG_REVOCATION_CHECK_REQ) )
		salt = XexData::XEX_SALT_REV;
	else
		salt = XexData::XEX_SALT_XEX;
	
	if( !XeCryptBnQwBeSigVerify((u64*)sig_ptr, hash, salt, (XeRsaKey*)publicKey) )
		return false;
	return true;
}


// fix all section hashes
bool XexHeader::updateSectionHashes(XexHvSectionInfo* sections, XexSecurityInfo& secInfo)
{
	XexHash hash;
	memset(hash.data, 0, sizeof(XexHash));
	s32 image_size_left = m_pXex->getImageSize();
	if(image_size_left <= 0)
		return true;
	
	DataBlock basefile;
	if(m_pXex->isPatchModule())
		m_pXex->getPatchData(basefile);
	else
		m_pXex->getBasefile(basefile);
	
	for(s32 hash_flag_index = m_pXex->numSections()-1;
		hash_flag_index >= 0;
		hash_flag_index--)
	{
		// get next section and fill its hash with
		// the result of the last hash calculated
		memcpy(sections[hash_flag_index].hash.data, hash.data, sizeof(XexHash));
		
		// calculate bounds for calculating hash over
		s32 hash_size;
		u8  type;
		m_pXex->getSection(hash_flag_index, hash_size, type);
		image_size_left -= hash_size;
		s32 hash_offset = image_size_left;
		
		// get data to calculate hash over, then calculate hash
		u8* block_data = new u8[hash_size];
		basefile.get(block_data, hash_offset, hash_size);
		XeShaContext sha_ctx;
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
bool XexHeader::updateImportHash(u8* headersData)
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
	
	// get the offset of each import library info
	std::vector<int> import_offsets;
	for(s32 offset=12 + GET32BE(import_data + 4); offset<(s32)GET32BE(import_data); offset+=GET32BE(import_data + offset))
	{
		import_offsets.push_back(offset);
	}
	
	// generate the hashes from the last hash to the first hash
	XexHash hash;
	memset(hash.data, 0, sizeof(XexHash));
	for(s32 lib_num=(s32)(import_offsets.size()-1); lib_num>=0; lib_num--)
	{
		// get pointer to current lib import
		u8* import_ptr = import_data + import_offsets[lib_num];
		s32 import_size = 0x24 + GET16BE(import_ptr + 0x26) * 4;
		
		// insert hash for the next import info
		memcpy(import_ptr + 4, hash.data, sizeof(XexHash));
		
		// get the hash for this import info
		XeCryptSha(import_ptr + 4, import_size,
			0,0,
			0,0,
			hash.data, sizeof(XexHash));
	}
	
	// insert hash for the next import info
	memcpy(sec_info->imageInfo.importHash.data, hash.data, sizeof(XexHash));
	return true;
}

// updates hash over various parts of the xex headers
bool XexHeader::updateHeaderHash(u8* headersData, s32 headersSize, s32 securityInfoOffset)
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

