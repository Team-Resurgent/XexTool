// 
// xex object
// 
// an object to store and handle access to xex data
// 

#include <assert.h>
#include <string.h>
#include "Xex.h"
#include "XexDefines.h"
#include "XeCryptCompat.h"
#include "PEParser.h"

#define SET_FLAG(toggle, flags, flag)	(toggle) ? ((flags) |= (flag)) : ((flags) &= (~(flag)))
#define IS_FLAG_SET(flags, flag)		(((flags) & (flag)) != 0)

Xex::Xex() :
	m_isTitleModule(false),
	m_isTitleExports(false),
	m_isSystemDebugger(false),
	m_isDllModule(false),
	m_isPatchModule(false),
	m_isPatchFull(false),
	m_isPatchDelta(false),
	m_isUserMode(false),
	m_unknownModuleFlags(0),
	m_discardableHeaderSize(0),
	
	m_imageFlags(0),
	m_systemFlags(0),
	m_systemFlags2(0),
	m_regions(0),
	m_mediaTypes(0),
	m_loadAddress(0),
	m_exportTableAddress(0),
	m_isDebug(true),
	m_isEncrypted(false),
	m_isBinary(true),
	m_isRaw(false),
	m_isCompressed(false),
	m_isDeltaCompressed(false),
	m_hasOriginalLoadAddress(false),
	m_baseReference(NULL),
	m_discProfileId(NULL),
	m_boundingPath(NULL),
	m_boundingDeviceId(NULL),
	m_callCap(NULL),
	m_fastCap(NULL),
	m_extraDebugMemory(NULL),
	m_pageheapInfo(NULL),
	m_restrictKVPrivs(NULL),
	m_restrictDates(NULL),
	m_originalPEName(NULL),
	m_tlsInfo(NULL),
	m_stackSize(NULL),
	m_filesystemCacheSize(NULL),
	m_heapSize(NULL),
	m_executionId(NULL),
	m_workspaceSize(NULL),
	m_ratings(NULL),
	m_lanKey(NULL),
	m_logoData(NULL),
	m_deltaPatch(NULL),
	m_exportsByName(NULL)
{
	memset(m_mediaId.data, 0, sizeof(MediaId));
	memset(m_imageKey.data, 0, sizeof(XexKey));
}
Xex::Xex(Xex const& source)
{
	copyData(source);
}

Xex& Xex::operator=(Xex const& source)
{
	// watch out for self assignment
	if(this != &source)
	{
		freeData();
		copyData(source);
	}
	return *this;
}
Xex::~Xex()
{
	freeData();
}
void Xex::copyData(Xex const& source)
{
	m_originalHeaders	= source.m_originalHeaders;
	m_patchData			= source.m_patchData;
	m_patchInfo			= source.m_patchInfo;
	
	m_isTitleModule = m_isTitleModule;
	m_isTitleExports = m_isTitleExports;
	m_isSystemDebugger = m_isSystemDebugger;
	m_isDllModule = m_isDllModule;
	m_isPatchModule = m_isPatchModule;
	m_isPatchFull = m_isPatchFull;
	m_isPatchDelta = m_isPatchDelta;
	m_isUserMode = m_isUserMode;
	m_unknownModuleFlags= source.m_unknownModuleFlags;
	m_discardableHeaderSize= source.m_discardableHeaderSize;
	
	m_imageFlags		= source.m_imageFlags;
	m_systemFlags		= source.m_systemFlags;
	m_systemFlags2		= source.m_systemFlags2;
	m_regions			= source.m_regions;
	m_mediaTypes		= source.m_mediaTypes;
	m_loadAddress		= source.m_loadAddress;
	m_exportTableAddress= source.m_exportTableAddress;
	m_basefile			= source.m_basefile;
	m_isDebug			= source.m_isDebug;
	m_isEncrypted		= source.m_isEncrypted;
	m_isBinary			= source.m_isBinary;
	m_isRaw				= source.m_isRaw;
	m_isCompressed		= source.m_isCompressed;
	m_isDeltaCompressed	= source.m_isDeltaCompressed;
	m_mediaId			= source.m_mediaId;
	m_imageKey			= source.m_imageKey;
	m_sections			= source.m_sections;
	m_hasOriginalLoadAddress = source.m_hasOriginalLoadAddress;
	if(source.m_baseReference)
	{
		m_baseReference = new u8[20];
		memcpy(m_baseReference, source.m_baseReference, 20);
	}
	if(source.m_discProfileId)
	{
		m_discProfileId = new DiscProfileId;
		*m_discProfileId = *source.m_discProfileId;
	}
	if(source.m_boundingPath)
	{
		m_boundingPath = new char[strlen(source.m_boundingPath)+1];
		strcpy(m_boundingPath, source.m_boundingPath);
	}
	if(source.m_boundingDeviceId)
	{
		m_boundingDeviceId = new u8[20];
		memcpy(m_boundingDeviceId, source.m_boundingDeviceId, 20);
	}
	if(source.m_callCap)
	{
		m_callCap = new u32[2];
		m_callCap[0] = source.m_callCap[0];
		m_callCap[1] = source.m_callCap[1];
	}
	if(source.m_fastCap)
	{
		m_fastCap = new u32;
		*m_fastCap = *source.m_fastCap;
	}
	if(source.m_extraDebugMemory)
	{
		m_extraDebugMemory = new u32;
		*m_extraDebugMemory = *source.m_extraDebugMemory;
	}
	if(source.m_pageheapInfo)
	{
		m_pageheapInfo = new u32[2];
		m_pageheapInfo[0] = source.m_pageheapInfo[0];
		m_pageheapInfo[1] = source.m_pageheapInfo[1];
	}
	if(source.m_restrictKVPrivs)
	{
		m_restrictKVPrivs = new u64[2];
		m_restrictKVPrivs[0] = source.m_restrictKVPrivs[0];
		m_restrictKVPrivs[1] = source.m_restrictKVPrivs[1];
	}
	if(source.m_restrictDates)
	{
		m_restrictDates = new u64[2];
		m_restrictDates[0] = source.m_restrictDates[0];
		m_restrictDates[1] = source.m_restrictDates[1];
	}
	if(source.m_originalPEName)
	{
		m_originalPEName = new char[strlen(source.m_originalPEName)+1];
		strcpy(m_originalPEName, source.m_originalPEName);
	}
	if(source.m_tlsInfo)
	{
		m_tlsInfo = new TLSInfo;
		*m_tlsInfo = *source.m_tlsInfo;
	}
	if(source.m_stackSize)
	{
		m_stackSize = new s32;
		*m_stackSize = *source.m_stackSize;
	}
	if(source.m_filesystemCacheSize)
	{
		m_filesystemCacheSize = new s32;
		*m_filesystemCacheSize = *source.m_filesystemCacheSize;
	}
	if(source.m_heapSize)
	{
		m_heapSize = new s32;
		*m_heapSize = *source.m_heapSize;
	}
	if(source.m_executionId)
	{
		m_executionId = new ExecutionId;
		*m_executionId = *source.m_executionId;
	}
	if(source.m_workspaceSize)
	{
		m_workspaceSize = new s32;
		*m_workspaceSize = *source.m_workspaceSize;
	}
	if(source.m_ratings)
	{
		m_ratings = new GameRatings;
		*m_ratings = *source.m_ratings;
	}
	if(source.m_lanKey)
	{
		m_lanKey = new LANKey;
		*m_lanKey = *source.m_lanKey;
	}
	if(source.m_logoData)
	{
		m_logoData = new DataBlock();
		*m_logoData = *source.m_logoData;
	}
	if(source.m_deltaPatch)
	{
		m_deltaPatch = new DataBlock();
		*m_deltaPatch = *source.m_deltaPatch;
	}
	if(source.m_exportsByName)
	{
		m_exportsByName = new ExportsByName;
		*m_exportsByName = *source.m_exportsByName;
	}
	m_resources			= source.m_resources;
	m_importLibraries	= source.m_importLibraries;
	m_staticLibraries	= source.m_staticLibraries;
	m_multidiscMediaIds	= source.m_multidiscMediaIds;
	m_altTitleIds		= source.m_altTitleIds;
	m_restrictConsoleIds  = source.m_restrictConsoleIds;
	m_unknownImageEntries = source.m_unknownImageEntries;
}
void Xex::freeData()
{
	clearSections();
	clearBasefile();
	clearResources();
	clearDeltaPatchDescriptor();
	clearBaseReference();
	clearDiscProfileId();
	clearBoundingPath();
	clearBoundingPath();
	clearImportLibraries();
	clearCallCap();
	clearFastCap();
	clearOriginalPEName();
	clearLibraryVersions();
	clearTLSInfo();
	clearStackSize();
	clearFilesystemCacheSize();
	clearHeapSize();
	clearExecutionId();
	clearWorkspaceSize();
	clearGameRatings();
	clearLANKey();
	clearLogoData();
	clearMultidiscMediaIds();
	clearAltTitleIds();
	clearExportsByName();
	clearUnknownImageEntries();
}

void Xex::clear()
{
	freeData();
}

bool Xex::isDebug() const	{ return m_isDebug; }
bool Xex::isRetail() const	{ return !m_isDebug;}

void Xex::setDebug()		{ m_isDebug = true; }
void Xex::setRetail()		{ m_isDebug = false; }



// 
// X E X  H E A D E R
// 

bool Xex::isTitleModule() const			{ return m_isTitleModule; }
bool Xex::isDllModule() const			{ return m_isDllModule; }
bool Xex::isPatchModule() const			{ return m_isPatchModule; }
bool Xex::isTitleExports() const		{ return m_isTitleExports; }
bool Xex::isSystemDebugger() const		{ return m_isSystemDebugger; }
bool Xex::isPatchFull() const			{ return m_isPatchFull; }
bool Xex::isPatchDelta() const			{ return m_isPatchDelta; }
bool Xex::isUserMode() const			{ return m_isUserMode; }

void Xex::setTitleModule(bool flag)		{ m_isTitleModule = flag; }
void Xex::setDllModule(bool flag)		{ m_isDllModule = flag; }
void Xex::setPatchModule(bool flag)		{ m_isPatchModule = flag; }
void Xex::setTitleExports(bool flag)	{ m_isTitleExports = flag; }
void Xex::setSystemDebugger(bool flag)	{ m_isSystemDebugger = flag; }
void Xex::setPatchFull(bool flag)		{ m_isPatchFull = flag; }
void Xex::setPatchDelta(bool flag)		{ m_isPatchDelta = flag; }
void Xex::setUserMode(bool flag)		{ m_isUserMode = flag; }

u32  Xex::getModuleFlags()
{
	u32 flags = 0;
	if( isTitleModule() )	flags |= MODULEFLAG_TITLE_MODULE;
	if( isDllModule() )		flags |= MODULEFLAG_DLL_MODULE;
	if( isPatchModule() )	flags |= MODULEFLAG_PATCH_MODULE;
	if( isTitleExports() )	flags |= MODULEFLAG_EXPORTS_TO_TITLE;
	if( isSystemDebugger() )flags |= MODULEFLAG_SYSTEM_DEBUGGER;
	if( isPatchFull() )		flags |= MODULEFLAG_PATCH_FULL;
	if( isPatchDelta() )	flags |= MODULEFLAG_PATCH_DELTA;
	if( isUserMode() )		flags |= MODULEFLAG_USER_MODE;
	flags |= getUnknownModuleFlags();
	return flags;
}
void Xex::setModuleFlags(u32 flags)
{
	setTitleModule( !!(flags & MODULEFLAG_TITLE_MODULE) );
	setDllModule( !!(flags & MODULEFLAG_DLL_MODULE) );
	setPatchModule( !!(flags & MODULEFLAG_PATCH_MODULE) );
	setTitleExports( !!(flags & MODULEFLAG_EXPORTS_TO_TITLE) );
	setSystemDebugger( !!(flags & MODULEFLAG_SYSTEM_DEBUGGER) );
	setPatchFull( !!(flags & MODULEFLAG_PATCH_FULL) );
	setPatchDelta( !!(flags & MODULEFLAG_PATCH_DELTA) );
	setUserMode( !!(flags & MODULEFLAG_USER_MODE) );
	setUnknownModuleFlags( flags & MODULEFLAG_UNKNOWN );
}

s32  Xex::getDiscardableHeaderSize() const	{ return m_discardableHeaderSize;	}
void Xex::setDiscardableHeaderSize(s32 size){ m_discardableHeaderSize = size;	}

u32  Xex::getUnknownModuleFlags() const		{ return m_unknownModuleFlags; }

void Xex::setUnknownModuleFlags(u32 flags)	{ m_unknownModuleFlags = flags; }


void Xex::getOriginalHeaders(DataBlock& headers) const
{ headers = m_originalHeaders; }

void Xex::setOriginalHeaders(const DataBlock& headers)
{ m_originalHeaders = headers; }


void Xex::getPatchData(DataBlock& patchData) const
{ patchData = m_patchData; }

void Xex::setPatchData(const DataBlock& patchData)
{ m_patchData = patchData; }


void Xex::getPatchInfo(DataBlock& patchInfo) const
{ patchInfo = m_patchInfo; }

void Xex::setPatchInfo(const DataBlock& patchInfo)
{ m_patchInfo = patchInfo; }


// 
// S E C U R I T Y   I N F O
// 

bool Xex::isManufacturingUtility() const	{ return IS_FLAG_SET(m_imageFlags, IMAGEFLAG_MANUFACTURING_UTILITY); }
bool Xex::isManufacturingSupportTool() const{ return IS_FLAG_SET(m_imageFlags, IMAGEFLAG_MANUFACTURING_TOOL); }
bool Xex::isXGD2Only() const			{ return IS_FLAG_SET(m_imageFlags, IMAGEFLAG_XGD2); }
bool Xex::isCardeaKey() const			{ return IS_FLAG_SET(m_imageFlags, IMAGEFLAG_CARDEA_KEY); }
bool Xex::isXeikaKey() const			{ return IS_FLAG_SET(m_imageFlags, IMAGEFLAG_XEIKA_KEY); }
bool Xex::isTitleUsermode() const		{ return IS_FLAG_SET(m_imageFlags, IMAGEFLAG_TITLE_USERMODE); }
bool Xex::isSystemUsermode() const		{ return IS_FLAG_SET(m_imageFlags, IMAGEFLAG_SYSTEM_USERMODE); }
bool Xex::isOrange0() const				{ return IS_FLAG_SET(m_imageFlags, IMAGEFLAG_ORANGE0); }
bool Xex::isOrange1() const				{ return IS_FLAG_SET(m_imageFlags, IMAGEFLAG_ORANGE1); }
bool Xex::isOrange2() const				{ return IS_FLAG_SET(m_imageFlags, IMAGEFLAG_ORANGE2); }
//bool Xex::isTestkitRestricted() const	{ return IS_FLAG_SET(m_imageFlags, IMAGEFLAG_TESTKIT_RESTRICTED); }
bool Xex::isSignedKeyvaultRestricted() const{ return IS_FLAG_SET(m_imageFlags, IMAGEFLAG_SIGNED_KV_RESTRICTED); }
bool Xex::isIptvSignupApp() const		{ return IS_FLAG_SET(m_imageFlags, IMAGEFLAG_IPTV_SIGNUP_APP); }
bool Xex::isIptvTitleApp() const		{ return IS_FLAG_SET(m_imageFlags, IMAGEFLAG_IPTV_TITLE_APP); }
bool Xex::isNccpKeys() const			{ return IS_FLAG_SET(m_imageFlags, IMAGEFLAG_NCCP_KEYS); }
bool Xex::isActivationReq() const		{ return IS_FLAG_SET(m_imageFlags, IMAGEFLAG_ACTIVATION_REQ); }
bool Xex::isPageSize4KB() const			{ return IS_FLAG_SET(m_imageFlags, IMAGEFLAG_4K_PAGES); }
bool Xex::isPageSize64KB() const		{ return !isPageSize4KB(); }
bool Xex::isNoGameRegion() const		{ return IS_FLAG_SET(m_imageFlags, IMAGEFLAG_NO_GAME_REGION); }
bool Xex::hasOptionalRevocationCheck() const{ return IS_FLAG_SET(m_imageFlags, IMAGEFLAG_REVOCATION_CHECK_OPT); }
bool Xex::hasRequiredRevocationCheck() const{ return IS_FLAG_SET(m_imageFlags, IMAGEFLAG_REVOCATION_CHECK_REQ); }

void Xex::setManufacturingUtility(bool flag)	{ SET_FLAG(flag, m_imageFlags, IMAGEFLAG_MANUFACTURING_UTILITY); }
void Xex::setManufacturingSupportTool(bool flag){ SET_FLAG(flag, m_imageFlags, IMAGEFLAG_MANUFACTURING_TOOL); }
void Xex::setXGD2Only(bool flag)		{ SET_FLAG(flag, m_imageFlags, IMAGEFLAG_XGD2); }
void Xex::setCardeaKey(bool flag)		{ SET_FLAG(flag, m_imageFlags, IMAGEFLAG_CARDEA_KEY); }
void Xex::setXeikaKey(bool flag)		{ SET_FLAG(flag, m_imageFlags, IMAGEFLAG_XEIKA_KEY); }
void Xex::setTitleUsermode(bool flag)	{ SET_FLAG(flag, m_imageFlags, IMAGEFLAG_TITLE_USERMODE); }
void Xex::setSystemUsermode(bool flag)	{ SET_FLAG(flag, m_imageFlags, IMAGEFLAG_SYSTEM_USERMODE); }
void Xex::setOrange0(bool flag)			{ SET_FLAG(flag, m_imageFlags, IMAGEFLAG_ORANGE0); }
void Xex::setOrange1(bool flag)			{ SET_FLAG(flag, m_imageFlags, IMAGEFLAG_ORANGE1); }
void Xex::setOrange2(bool flag)			{ SET_FLAG(flag, m_imageFlags, IMAGEFLAG_ORANGE2); }
//void Xex::setTestkitRestricted(bool flag){SET_FLAG(flag, m_imageFlags, IMAGEFLAG_TESTKIT_RESTRICTED); }
void Xex::setSignedKeyvaultRestricted(bool flag){SET_FLAG(flag, m_imageFlags, IMAGEFLAG_SIGNED_KV_RESTRICTED); }
void Xex::setIptvSignupApp(bool flag)	{ SET_FLAG(flag, m_imageFlags, IMAGEFLAG_IPTV_SIGNUP_APP); }
void Xex::setIptvTitleApp(bool flag)	{ SET_FLAG(flag, m_imageFlags, IMAGEFLAG_IPTV_TITLE_APP); }
void Xex::setNccpKeys(bool flag)		{ SET_FLAG(flag, m_imageFlags, IMAGEFLAG_NCCP_KEYS); }
void Xex::setActivationReq(bool flag)	{ SET_FLAG(flag, m_imageFlags, IMAGEFLAG_ACTIVATION_REQ); }
void Xex::setPageSize4KB()				{ SET_FLAG(true, m_imageFlags, IMAGEFLAG_4K_PAGES); }
void Xex::setPageSize64KB()				{ SET_FLAG(false,m_imageFlags, IMAGEFLAG_4K_PAGES); }
void Xex::setNoGameRegion(bool flag)	{ SET_FLAG(flag, m_imageFlags, IMAGEFLAG_NO_GAME_REGION); }
void Xex::setOptionalRevocationCheck(bool flag)	{ SET_FLAG(flag, m_imageFlags, IMAGEFLAG_REVOCATION_CHECK_OPT); }
void Xex::setRequiredRevocationCheck(bool flag)	{ SET_FLAG(flag, m_imageFlags, IMAGEFLAG_REVOCATION_CHECK_REQ); }

u32  Xex::getUnknownImageFlags() const
{ return (m_imageFlags & IMAGEFLAG_UNKNOWN); }

void Xex::setUnknownImageFlags(u32 flags)
{ m_imageFlags |= (flags & IMAGEFLAG_UNKNOWN); }

u32  Xex::getImageFlags()			{ return m_imageFlags; }
void Xex::setImageFlags(u32 flags)	{ m_imageFlags = flags; }

s32  Xex::getImageSize() const			{ return m_basefile.size(); }
void Xex::setLoadAddress(u32 addr)		{ m_loadAddress = addr; }
u32  Xex::getLoadAddress() const		{ return m_loadAddress; }
void Xex::getMediaId(MediaId& id) const	{ id = m_mediaId; }
void Xex::setMediaId(const MediaId& id)	{ m_mediaId = id; }
void Xex::getImageKey(XexKey& key) const{ key = m_imageKey; }
void Xex::setImageKey(const XexKey& key){ m_imageKey = key; }
u32  Xex::getExportTableAddress() const	{ return m_exportTableAddress; }
void Xex::setExportTableAddress(u32 addr){m_exportTableAddress = addr; }

bool Xex::isRegionNorthAmerica() const	{ return IS_FLAG_SET(m_regions, REGION_NORTH_AMERICA); }
bool Xex::isRegionJapan() const			{ return IS_FLAG_SET(m_regions, REGION_JAPAN); }
bool Xex::isRegionChina() const			{ return IS_FLAG_SET(m_regions, REGION_CHINA); }
bool Xex::isRegionRestOfAsia() const	{ return IS_FLAG_SET(m_regions, REGION_REST_OF_ASIA); }
bool Xex::isRegionAustNZ() const		{ return IS_FLAG_SET(m_regions, REGION_AUST_NZ); }
bool Xex::isRegionRestOfEurope() const	{ return IS_FLAG_SET(m_regions, REGION_REST_OF_EUROPE); }
bool Xex::isRegionRestOfWorld() const	{ return IS_FLAG_SET(m_regions, REGION_REST_OF_WORLD); }

void Xex::setRegionNorthAmerica(bool flag){ SET_FLAG(flag, m_regions, REGION_NORTH_AMERICA); }
void Xex::setRegionJapan(bool flag)		{ SET_FLAG(flag, m_regions, REGION_JAPAN); }
void Xex::setRegionChina(bool flag)		{ SET_FLAG(flag, m_regions, REGION_CHINA); }
void Xex::setRegionRestOfAsia(bool flag){ SET_FLAG(flag, m_regions, REGION_REST_OF_ASIA); }
void Xex::setRegionAustNZ(bool flag)	{ SET_FLAG(flag, m_regions, REGION_AUST_NZ); }
void Xex::setRegionRestOfEurope(bool flag){ SET_FLAG(flag, m_regions, REGION_REST_OF_EUROPE); }
void Xex::setRegionRestOfWorld(bool flag){ SET_FLAG(flag, m_regions, REGION_REST_OF_WORLD); }

bool Xex::isAllRegions() const			{ return m_regions == REGION_ALL; }
void Xex::setAllRegions()				{ m_regions = REGION_ALL; }

u32  Xex::getRegions()					{ return m_regions; }
void Xex::setRegions(u32 regions)		{ m_regions = regions; }

bool Xex::isMediaHardDisk() const		{ return IS_FLAG_SET(m_mediaTypes, MEDIATYPE_HARD_DISK); }
bool Xex::isMediaDvdX2() const			{ return IS_FLAG_SET(m_mediaTypes, MEDIATYPE_DVDX2); }
bool Xex::isMediaDvdCd() const			{ return IS_FLAG_SET(m_mediaTypes, MEDIATYPE_DVD_CD); }
bool Xex::isMediaDvd5() const			{ return IS_FLAG_SET(m_mediaTypes, MEDIATYPE_DVD5); }
bool Xex::isMediaDvd9() const			{ return IS_FLAG_SET(m_mediaTypes, MEDIATYPE_DVD9); }
bool Xex::isMediaSystemFlash() const	{ return IS_FLAG_SET(m_mediaTypes, MEDIATYPE_SYS_FLASH); }
bool Xex::isMediaMemoryUnit() const		{ return IS_FLAG_SET(m_mediaTypes, MEDIATYPE_MEM_UNIT); }
bool Xex::isMediaMassStorage() const	{ return IS_FLAG_SET(m_mediaTypes, MEDIATYPE_MASS_STORAGE); }
bool Xex::isMediaSMB() const			{ return IS_FLAG_SET(m_mediaTypes, MEDIATYPE_SMB); }
bool Xex::isMediaRam() const			{ return IS_FLAG_SET(m_mediaTypes, MEDIATYPE_RAM); }
bool Xex::isMediaRamDrive() const		{ return IS_FLAG_SET(m_mediaTypes, MEDIATYPE_RAM_DRIVE); }
bool Xex::isMediaSecureVirtOD() const	{ return IS_FLAG_SET(m_mediaTypes, MEDIATYPE_SECURE_VIRT_OD); }
bool Xex::isMediaWirelessNStorage() const{return IS_FLAG_SET(m_mediaTypes, MEDIATYPE_WIRELESS_N_STORAGE); }
bool Xex::isMediaSystemExtPartition() const{return IS_FLAG_SET(m_mediaTypes, MEDIATYPE_SYS_EXT_PARTITION); }
bool Xex::isMediaSystemAuxPartition() const{return IS_FLAG_SET(m_mediaTypes, MEDIATYPE_SYS_AUX_PARTITION); }
bool Xex::isMediaInsecurePackage() const{ return IS_FLAG_SET(m_mediaTypes, MEDIATYPE_INSECURE_PKG); }
bool Xex::isMediaSavegamePackage() const{ return IS_FLAG_SET(m_mediaTypes, MEDIATYPE_SAVEGAME_PKG); }
bool Xex::isMediaLocallySignedPackage() const{ return IS_FLAG_SET(m_mediaTypes, MEDIATYPE_LOCALSIGN_PKG); }
bool Xex::isMediaLiveSignedPackage() const{ return IS_FLAG_SET(m_mediaTypes, MEDIATYPE_LIVESIGN_PKG); }
bool Xex::isMediaXboxPackage() const	{ return IS_FLAG_SET(m_mediaTypes, MEDIATYPE_XBOX_PKG); }

void Xex::setMediaHardDisk(bool flag)	{ SET_FLAG(flag, m_mediaTypes, MEDIATYPE_HARD_DISK); }
void Xex::setMediaDvdX2(bool flag)		{ SET_FLAG(flag, m_mediaTypes, MEDIATYPE_DVDX2); }
void Xex::setMediaDvdCd(bool flag)		{ SET_FLAG(flag, m_mediaTypes, MEDIATYPE_DVD_CD); }
void Xex::setMediaDvd5(bool flag)		{ SET_FLAG(flag, m_mediaTypes, MEDIATYPE_DVD5); }
void Xex::setMediaDvd9(bool flag)		{ SET_FLAG(flag, m_mediaTypes, MEDIATYPE_DVD9); }
void Xex::setMediaSystemFlash(bool flag){ SET_FLAG(flag, m_mediaTypes, MEDIATYPE_SYS_FLASH); }
void Xex::setMediaMemoryUnit(bool flag)	{ SET_FLAG(flag, m_mediaTypes, MEDIATYPE_MEM_UNIT); }
void Xex::setMediaMassStorage(bool flag){ SET_FLAG(flag, m_mediaTypes, MEDIATYPE_MASS_STORAGE); }
void Xex::setMediaSMB(bool flag)		{ SET_FLAG(flag, m_mediaTypes, MEDIATYPE_SMB); }
void Xex::setMediaRam(bool flag)		{ SET_FLAG(flag, m_mediaTypes, MEDIATYPE_RAM); }
void Xex::setMediaRamDrive(bool flag)	{ SET_FLAG(flag, m_mediaTypes, MEDIATYPE_RAM_DRIVE); }
void Xex::setMediaSecureVirtOD(bool flag)	{ SET_FLAG(flag, m_mediaTypes, MEDIATYPE_SECURE_VIRT_OD); }
void Xex::setMediaWirelessNStorage(bool flag){SET_FLAG(flag, m_mediaTypes, MEDIATYPE_WIRELESS_N_STORAGE); }
void Xex::setMediaSystemExtPartition(bool flag){SET_FLAG(flag, m_mediaTypes, MEDIATYPE_SYS_EXT_PARTITION); }
void Xex::setMediaSystemAuxPartition(bool flag){SET_FLAG(flag, m_mediaTypes, MEDIATYPE_SYS_AUX_PARTITION); }
void Xex::setMediaInsecurePackage(bool flag){ SET_FLAG(flag, m_mediaTypes, MEDIATYPE_INSECURE_PKG); }
void Xex::setMediaSavegamePackage(bool flag){ SET_FLAG(flag, m_mediaTypes, MEDIATYPE_SAVEGAME_PKG); }
void Xex::setMediaLocallySignedPackage(bool flag){ SET_FLAG(flag, m_mediaTypes, MEDIATYPE_LOCALSIGN_PKG); }
void Xex::setMediaLiveSignedPackage(bool flag){ SET_FLAG(flag, m_mediaTypes, MEDIATYPE_LIVESIGN_PKG); }
void Xex::setMediaXboxPackage(bool flag){ SET_FLAG(flag, m_mediaTypes, MEDIATYPE_XBOX_PKG); }

bool Xex::isAllMediaTypes() const		{ return m_mediaTypes == MEDIATYPE_ALL; }
void Xex::setAllMediaTypes()			{ m_mediaTypes = MEDIATYPE_ALL; }
u32  Xex::getUnknownMediaTypes() const	{ return (m_mediaTypes & MEDIATYPE_UNKNOWN); }
void Xex::setUnknownMediaTypes(u32 flags){ m_mediaTypes |= (flags & MEDIATYPE_UNKNOWN); }

u32  Xex::getMediaTypes()				{ return m_mediaTypes; }
void Xex::setMediaTypes(u32 mtypes)		{ m_mediaTypes = mtypes; }


s32  Xex::numSections() const
{
	return (s32)m_sections.size();
}
void Xex::clearSections()
{
	m_sections.clear();
}
bool Xex::removeSection(s32 index)
{
	if(index < 0 || index >= (s32)m_sections.size())
		return false;
	std::vector <BasefileSection>::iterator iter;
	s32 i = index;
	for( iter = m_sections.begin(); iter != m_sections.end(); iter++ )	if(i-- == 0) break;
	m_sections.erase(iter);
	return true;
}
void Xex::addSection(s32 size, u8 type)
{
	BasefileSection section;
	section.size = size;
	section.type = type;
	m_sections.push_back(section);
}
bool Xex::getSection(s32 index, s32 &size, u8 &type) const
{
	if(index < 0 || index >= (s32)m_sections.size())
		return false;
	BasefileSection section = m_sections.at(index);
	size = section.size;
	type = section.type;
	return true;
}
bool Xex::setSection(s32 index, s32 size, u8 type)
{
	if(index < 0 || index >= (s32)m_sections.size())
		return false;
	BasefileSection section;
	section.size = size;
	section.type = type;
	m_sections.at(index) = section;
	return true;
}


// 
// B A S E F I L E   I N F O
// 

s32  Xex::basefileSize() const
{
	return m_basefile.size();
}
void Xex::clearBasefile()
{
	m_basefile.clear();
}
void Xex::getBasefile(DataBlock& basefile) const
{
	basefile = m_basefile;
}
// sets basefile from any format
bool Xex::setBasefile(const DataBlock& basefile)
{
	// clear current basefile
	m_basefile.clear();
	
	// set new basefile
	m_basefile = basefile;
	return true;
}
bool Xex::isBasefilePE() const
{
	PEParser pe_parser(m_basefile);
	return pe_parser.isPeFile();
}
bool Xex::isBasefileExe() const
{
	if( !isBasefilePE() )
		return false;
	PEParser pe_parser(m_basefile);
	u16 characteristics = pe_parser.getCharacteristics();
	return (characteristics & 0x2000) == 0;
}
bool Xex::isBasefileDll() const
{
	if( !isBasefilePE() )
		return false;
	PEParser pe_parser(m_basefile);
	u16 characteristics = pe_parser.getCharacteristics();
	return (characteristics & 0x2000) != 0;
}
bool Xex::isBasefileXUIZ() const
{
	u8 magic[4];
	m_basefile.get(magic, 0, 4);
	return memcmp(magic, "XUIZ", 4) == 0;
}


// 
// S Y S T E M   F L A G S
// 

bool Xex::isNoForcedReboot() const			{ return IS_FLAG_SET(m_systemFlags, SYSFLAG_NO_FORCE_REBOOT); }
bool Xex::isForegroundTasks() const			{ return IS_FLAG_SET(m_systemFlags, SYSFLAG_FOREGROUND_TASKS); }
bool Xex::isNoODDMapping() const			{ return IS_FLAG_SET(m_systemFlags, SYSFLAG_NO_ODD_MAPPING); }
bool Xex::isMceInputHandler() const			{ return IS_FLAG_SET(m_systemFlags, SYSFLAG_HANDLE_MCE_INPUT); }
bool Xex::isRestrictedHudFeatures() const	{ return IS_FLAG_SET(m_systemFlags, SYSFLAG_RESTRICT_HUD_FEATURES); }
bool Xex::isGamepadDisconnectHandler() const{ return IS_FLAG_SET(m_systemFlags, SYSFLAG_HANDLE_GAMEPAD_DISCONNECT); }
bool Xex::isInsecureSockets() const			{ return IS_FLAG_SET(m_systemFlags, SYSFLAG_INSECURE_SOCKETS); }
bool Xex::isXbox1Interoperability() const	{ return IS_FLAG_SET(m_systemFlags, SYSFLAG_XBOX_1_XSP_INTEROP); }
bool Xex::isDashContext() const				{ return IS_FLAG_SET(m_systemFlags, SYSFLAG_SET_DASH_CONTEXT); }
bool Xex::isGameVoiceChannelUser() const	{ return IS_FLAG_SET(m_systemFlags, SYSFLAG_USES_GAME_VOICE_CHANNEL); }
bool Xex::isPal50Incompatible() const		{ return IS_FLAG_SET(m_systemFlags, SYSFLAG_PAL50_INCOMPATIBLE); }
bool Xex::isInsecureUtilDriveUser() const	{ return IS_FLAG_SET(m_systemFlags, SYSFLAG_INSECURE_UTILITYDRIVE); }
bool Xex::isXamHooks() const				{ return IS_FLAG_SET(m_systemFlags, SYSFLAG_HAS_XAM_HOOKS); }
bool Xex::isPII() const						{ return IS_FLAG_SET(m_systemFlags, SYSFLAG_PII); }
bool Xex::isCrossPlatformSyslinkUser() const{ return IS_FLAG_SET(m_systemFlags, SYSFLAG_CROSSPLATFORM_SYSLINK); }
bool Xex::isMultidiscSwap() const			{ return IS_FLAG_SET(m_systemFlags, SYSFLAG_MULTIDISC_SWAP); }
bool Xex::isMultidiscInsecureMedia() const	{ return IS_FLAG_SET(m_systemFlags, SYSFLAG_MULTIDISC_INSECURE_MEDIA); }
bool Xex::isAP25Media() const				{ return IS_FLAG_SET(m_systemFlags, SYSFLAG_AP25_MEDIA); }
bool Xex::isNoCofirmExit() const			{ return IS_FLAG_SET(m_systemFlags, SYSFLAG_NO_CONFIRM_EXIT); }
bool Xex::isAllowBackgroundDownload() const	{ return IS_FLAG_SET(m_systemFlags, SYSFLAG_ALLOW_BKGRND_DOWNLOAD); }
bool Xex::isCreatePersistRamdrive() const	{ return IS_FLAG_SET(m_systemFlags, SYSFLAG_CREATE_PERSIST_RAMDRIVE); }
bool Xex::isInheritPersistRamdrive() const	{ return IS_FLAG_SET(m_systemFlags, SYSFLAG_INHERIT_PERSIST_RAMDRIVE); }
bool Xex::isAllowHudVibration() const		{ return IS_FLAG_SET(m_systemFlags, SYSFLAG_ALLOW_HUD_VIBRATION); }
bool Xex::isBothUtilityPartitions() const	{ return IS_FLAG_SET(m_systemFlags, SYSFLAG_BOTH_UTILITY_PARTITIONS); }
bool Xex::isIptvInputHandler() const		{ return IS_FLAG_SET(m_systemFlags, SYSFLAG_HANDLE_IPTV_INPUT); }
bool Xex::isPreferBigButtonInput() const	{ return IS_FLAG_SET(m_systemFlags, SYSFLAG_PREFER_BIGBUTTON_INPUT); }
bool Xex::isAllowXsamReservation() const	{ return IS_FLAG_SET(m_systemFlags, SYSFLAG_ALLOW_XSAM_RESERVATION); }
bool Xex::isMultiDiscCrossTitle() const		{ return IS_FLAG_SET(m_systemFlags, SYSFLAG_MULTIDISC_CROSS_TITLE); }
bool Xex::isTitleInstallIncompatible() const{ return IS_FLAG_SET(m_systemFlags, SYSFLAG_TITLE_INSTALL_INCOMPATIBLE); }
bool Xex::isAllowAvatarGetMetadata() const	{ return IS_FLAG_SET(m_systemFlags, SYSFLAG_ALLOW_AVATAR_GET_METADATA); }
bool Xex::isAllowControllerSwapping() const	{ return IS_FLAG_SET(m_systemFlags, SYSFLAG_ALLOW_CONTROLLER_SWAPPING); }
bool Xex::isDashExtensibilityModule() const	{ return IS_FLAG_SET(m_systemFlags, SYSFLAG_DASH_EXTENSIBILITY_MODULE); }
bool Xex::isAllowNetworkReadCancel() const	{ return IS_FLAG_SET(m_systemFlags2, SYSFLAG2_ALLOW_NETWORK_READ_CANCEL); }
bool Xex::isUninterruptableReads() const	{ return IS_FLAG_SET(m_systemFlags2, SYSFLAG2_UNINTERRUPTABLE_READS); }
bool Xex::isRequiresNXE() const				{ return IS_FLAG_SET(m_systemFlags2, SYSFLAG2_REQUIRE_FULL_EXPERIENCE); }
bool Xex::isGamevoiceRequiredUI() const		{ return IS_FLAG_SET(m_systemFlags2, SYSFLAG2_GAMEVOICE_REQUIRED_UI); }
bool Xex::isTitleSetsPresenceString() const	{ return IS_FLAG_SET(m_systemFlags2, SYSFLAG2_TITLE_SET_PRESENCE_STRING); }
bool Xex::isNatalTiltControl() const		{ return IS_FLAG_SET(m_systemFlags2, SYSFLAG2_NATAL_TILTCONTROL); }
bool Xex::isSkeletalTrackingSupported() const{return IS_FLAG_SET(m_systemFlags2, SYSFLAG2_REQUIRES_SKELETAL_TRACKING); }
bool Xex::isSkeletalTrackingRequired() const{ return IS_FLAG_SET(m_systemFlags2, SYSFLAG2_SUPPORTS_SKELETAL_TRACKING); }
bool Xex::isLargeHdsFileCacheUsed() const	{ return IS_FLAG_SET(m_systemFlags2, SYSFLAG2_USE_LARGE_HDS_FILE_CACHE); }
bool Xex::isTitleSupportsDeepLink() const	{ return IS_FLAG_SET(m_systemFlags2, SYSFLAG2_TITLE_SUPPORTS_DEEP_LINK); }

void Xex::setNoForcedReboot(bool flag)			{ SET_FLAG(flag, m_systemFlags, SYSFLAG_NO_FORCE_REBOOT); }
void Xex::setForegroundTasks(bool flag)			{ SET_FLAG(flag, m_systemFlags, SYSFLAG_FOREGROUND_TASKS); }
void Xex::setNoODDMapping(bool flag)			{ SET_FLAG(flag, m_systemFlags, SYSFLAG_NO_ODD_MAPPING); }
void Xex::setMceInputHandler(bool flag)			{ SET_FLAG(flag, m_systemFlags, SYSFLAG_HANDLE_MCE_INPUT); }
void Xex::setRestrictedHudFeatures(bool flag)	{ SET_FLAG(flag, m_systemFlags, SYSFLAG_RESTRICT_HUD_FEATURES); }
void Xex::setGamepadDisconnectHandler(bool flag){ SET_FLAG(flag, m_systemFlags, SYSFLAG_HANDLE_GAMEPAD_DISCONNECT); }
void Xex::setInsecureSockets(bool flag)			{ SET_FLAG(flag, m_systemFlags, SYSFLAG_INSECURE_SOCKETS); }
void Xex::setXbox1Interoperability(bool flag)	{ SET_FLAG(flag, m_systemFlags, SYSFLAG_XBOX_1_XSP_INTEROP); }
void Xex::setDashContext(bool flag)				{ SET_FLAG(flag, m_systemFlags, SYSFLAG_SET_DASH_CONTEXT); }
void Xex::setGameVoiceChannelUser(bool flag)	{ SET_FLAG(flag, m_systemFlags, SYSFLAG_USES_GAME_VOICE_CHANNEL); }
void Xex::setPal50Incompatible(bool flag)		{ SET_FLAG(flag, m_systemFlags, SYSFLAG_PAL50_INCOMPATIBLE); }
void Xex::setInsecureUtilDriveUser(bool flag)	{ SET_FLAG(flag, m_systemFlags, SYSFLAG_INSECURE_UTILITYDRIVE); }
void Xex::setXamHooks(bool flag)				{ SET_FLAG(flag, m_systemFlags, SYSFLAG_HAS_XAM_HOOKS); }
void Xex::setPII(bool flag)						{ SET_FLAG(flag, m_systemFlags, SYSFLAG_PII); }
void Xex::setCrossPlatformSyslinkUser(bool flag){ SET_FLAG(flag, m_systemFlags, SYSFLAG_CROSSPLATFORM_SYSLINK); }
void Xex::setMultidiscSwap(bool flag)			{ SET_FLAG(flag, m_systemFlags, SYSFLAG_MULTIDISC_SWAP); }
void Xex::setMultidiscInsecureMedia(bool flag)	{ SET_FLAG(flag, m_systemFlags, SYSFLAG_MULTIDISC_INSECURE_MEDIA); }
void Xex::setAP25Media(bool flag)				{ SET_FLAG(flag, m_systemFlags, SYSFLAG_AP25_MEDIA); }
void Xex::setNoCofirmExit(bool flag)			{ SET_FLAG(flag, m_systemFlags, SYSFLAG_NO_CONFIRM_EXIT); }
void Xex::setAllowBackgroundDownload(bool flag)	{ SET_FLAG(flag, m_systemFlags, SYSFLAG_ALLOW_BKGRND_DOWNLOAD); }
void Xex::setCreatePersistRamdrive(bool flag)	{ SET_FLAG(flag, m_systemFlags, SYSFLAG_CREATE_PERSIST_RAMDRIVE); }
void Xex::setInheritPersistRamdrive(bool flag)	{ SET_FLAG(flag, m_systemFlags, SYSFLAG_INHERIT_PERSIST_RAMDRIVE); }
void Xex::setAllowHudVibration(bool flag)		{ SET_FLAG(flag, m_systemFlags, SYSFLAG_ALLOW_HUD_VIBRATION); }
void Xex::setBothUtilityPartitions(bool flag)	{ SET_FLAG(flag, m_systemFlags, SYSFLAG_BOTH_UTILITY_PARTITIONS); }
void Xex::setIptvInputHandler(bool flag)		{ SET_FLAG(flag, m_systemFlags, SYSFLAG_HANDLE_IPTV_INPUT); }
void Xex::setPreferBigButtonInput(bool flag)	{ SET_FLAG(flag, m_systemFlags, SYSFLAG_PREFER_BIGBUTTON_INPUT); }
void Xex::setAllowXsamReservation(bool flag)	{ SET_FLAG(flag, m_systemFlags, SYSFLAG_ALLOW_XSAM_RESERVATION); }
void Xex::setMultiDiscCrossTitle(bool flag)		{ SET_FLAG(flag, m_systemFlags, SYSFLAG_MULTIDISC_CROSS_TITLE); }
void Xex::setTitleInstallIncompatible(bool flag){ SET_FLAG(flag, m_systemFlags, SYSFLAG_TITLE_INSTALL_INCOMPATIBLE); }
void Xex::setAllowAvatarGetMetadata(bool flag)	{ SET_FLAG(flag, m_systemFlags, SYSFLAG_ALLOW_AVATAR_GET_METADATA); }
void Xex::setAllowControllerSwapping(bool flag)	{ SET_FLAG(flag, m_systemFlags, SYSFLAG_ALLOW_CONTROLLER_SWAPPING); }
void Xex::setDashExtensibilityModule(bool flag)	{ SET_FLAG(flag, m_systemFlags, SYSFLAG_DASH_EXTENSIBILITY_MODULE); }
void Xex::setAllowNetworkReadCancel(bool flag)	{ SET_FLAG(flag, m_systemFlags2, SYSFLAG2_ALLOW_NETWORK_READ_CANCEL); }
void Xex::setUninterruptableReads(bool flag)	{ SET_FLAG(flag, m_systemFlags2, SYSFLAG2_UNINTERRUPTABLE_READS); }
void Xex::setRequiresNXE(bool flag)				{ SET_FLAG(flag, m_systemFlags2, SYSFLAG2_REQUIRE_FULL_EXPERIENCE); }
void Xex::setGamevoiceRequiredUI(bool flag)		{ SET_FLAG(flag, m_systemFlags2, SYSFLAG2_GAMEVOICE_REQUIRED_UI); }
void Xex::setTitleSetsPresenceString(bool flag)	{ SET_FLAG(flag, m_systemFlags2, SYSFLAG2_TITLE_SET_PRESENCE_STRING); }
void Xex::setNatalTiltControl(bool flag)		{ SET_FLAG(flag, m_systemFlags2, SYSFLAG2_NATAL_TILTCONTROL); }
void Xex::setSkeletalTrackingSupported(bool flag){SET_FLAG(flag, m_systemFlags2, SYSFLAG2_REQUIRES_SKELETAL_TRACKING); }
void Xex::setSkeletalTrackingRequired(bool flag){ SET_FLAG(flag, m_systemFlags2, SYSFLAG2_SUPPORTS_SKELETAL_TRACKING); }
void Xex::setLargeHdsFileCacheUsed(bool flag)	{ SET_FLAG(flag, m_systemFlags2, SYSFLAG2_USE_LARGE_HDS_FILE_CACHE); }
void Xex::setTitleSupportsDeepLink(bool flag)	{ SET_FLAG(flag, m_systemFlags2, SYSFLAG2_TITLE_SUPPORTS_DEEP_LINK); }

u32  Xex::getUnknownSystemFlags() const
{ return m_systemFlags & SYSFLAG_UNKNOWN; }

void Xex::setUnknownSystemFlags(u32 flags)
{ m_systemFlags |= (flags & SYSFLAG_UNKNOWN); }

bool Xex::hasSystemFlags() const
{
	return m_systemFlags != 0;
}

u32  Xex::getUnknownSystemFlags2() const
{ return m_systemFlags2 & SYSFLAG2_UNKNOWN; }

void Xex::setUnknownSystemFlags2(u32 flags)
{ m_systemFlags2 |= (flags & SYSFLAG2_UNKNOWN); }

bool Xex::hasSystemFlags2() const
{
	return m_systemFlags2 != 0;
}

u32  Xex::getSystemFlags()			{ return m_systemFlags;  }
void Xex::setSystemFlags(u32 flags)	{ m_systemFlags = flags; }
u32  Xex::getSystemFlags2()			{ return m_systemFlags2; }
void Xex::setSystemFlags2(u32 flags)	{ m_systemFlags2 = flags;}

// 
// O P T I O N A L   I N F O
// 

bool Xex::isBinary() const			{ return m_isBinary; }
bool Xex::isRaw() const				{ return m_isRaw; }
bool Xex::isCompressed() const		{ return m_isCompressed; }
bool Xex::isDeltaCompressed() const	{ return m_isDeltaCompressed; }

void Xex::setBinary()				{ m_isBinary = true;  m_isRaw = false; m_isCompressed = false; m_isDeltaCompressed = false; }
void Xex::setRaw()					{ m_isBinary = false; m_isRaw = true;  m_isCompressed = false; m_isDeltaCompressed = false; }
void Xex::setCompressed()			{ m_isBinary = false; m_isRaw = false; m_isCompressed = true;  m_isDeltaCompressed = false; }
void Xex::setDeltaCompressed()		{ m_isBinary = false; m_isRaw = false; m_isCompressed = false; m_isDeltaCompressed = true;  }

bool Xex::isEncrypted() const		{ return m_isEncrypted; }
void Xex::setEncrypted(bool toggle)	{ m_isEncrypted = toggle; }

bool Xex::hasEntryPoint() const
{
	// only PE files can have entry points
	// this also filters out dlls whose entry points are effectively zero
	return isBasefilePE() &&
		(getEntryPoint() != getLoadAddress()) &&
		(getEntryPoint() != 0);
}
bool Xex::hasOriginalLoadAddress() const
{
	return m_hasOriginalLoadAddress && isBasefilePE();
}
bool Xex::hasChecksum() const
{
	return isBasefilePE();
}
bool Xex::hasFiletime() const
{
	return isBasefilePE();
}
u32 Xex::getEntryPoint() const
{
	if( !isBasefilePE() )
		return false;
	PEParser pe_parser(m_basefile);
	return pe_parser.getEntryPoint() + getLoadAddress();
}
u32 Xex::getOriginalLoadAddress() const
{
	if( !isBasefilePE() )
		return false;
	PEParser pe_parser(m_basefile);
	return pe_parser.getLoadAddress();
}
u32 Xex::getChecksum() const
{
	if( !isBasefilePE() )
		return false;
	PEParser pe_parser(m_basefile);
	return pe_parser.getChecksum();
}
u32 Xex::getFiletime() const
{
	if( !isBasefilePE() )
		return false;
	PEParser pe_parser(m_basefile);
	return pe_parser.getFiletime();
}
s32 Xex::getPageSize() const
{
	if(isPageSize4KB())
		return 4 * 1024;
	else
		return 64 * 1024;
}
void Xex::setHasOriginalLoadAddress(bool flag)
{
	m_hasOriginalLoadAddress = flag;
}


s32  Xex::numResources() const
{
	return (s32)m_resources.size();
}
void Xex::clearResources()
{
	m_resources.clear();
}
bool Xex::removeResource(s32 index)
{
	if(index < 0 || index >= (s32)m_resources.size())
		return false;
	std::vector <ResourceEntry>::iterator iter;
	s32 i = index;
	for( iter = m_resources.begin(); iter != m_resources.end(); iter++ )	if(i-- == 0) break;
	m_resources.erase(iter);
	return true;
}
void Xex::addResource(u32 addr, s32 size, const char* name)
{
	ResourceEntry resource;
	strncpy(resource.name, name, 8);
	resource.addr = addr;
	resource.size = size;
	m_resources.push_back(resource);
}
bool Xex::getResource(s32 index, u32& addr, s32& size, char* name) const
{
	if(index < 0 || index >= (s32)m_resources.size())
		return false;
	ResourceEntry resource = m_resources.at(index);
	size = resource.size;
	addr = resource.addr;
	strncpy(name, resource.name, 8);
	name[8] = 0;
	return true;
}
bool Xex::setResource(s32 index, u32 addr, s32 size, const char* name)
{
	if(index < 0 || index >= (s32)m_resources.size())
		return false;
	ResourceEntry resource;
	strncpy(resource.name, name, 8);
	resource.size = size;
	resource.addr = addr;
	m_resources.at(index) = resource;
	return true;
}


bool Xex::hasDeltaPatchDescriptor() const
{
	return m_deltaPatch != NULL;
}
void Xex::clearDeltaPatchDescriptor()
{
	if(m_deltaPatch)
	{
		delete m_deltaPatch;
		m_deltaPatch = NULL;
	}
}
void Xex::setDeltaPatchDescriptor(const DataBlock& patch)
{
	clearDeltaPatchDescriptor();
	m_deltaPatch = new DataBlock(patch);
}
bool Xex::getDeltaPatchDescriptor(DataBlock& patch) const
{
	if(m_deltaPatch == NULL)
		return false;
	patch = *m_deltaPatch;
	return true;
}


bool Xex::hasBaseReference() const
{
	return m_baseReference != NULL;
}
void Xex::clearBaseReference()
{
	if(m_baseReference)
	{
		delete[] m_baseReference;
		m_baseReference = NULL;
	}
}
void Xex::setBaseReference(const u8 ref[20])
{
	clearBaseReference();
	if(ref)
	{
		m_baseReference = new u8[20];
		memcpy(m_baseReference, ref, 20);
	}
}
bool Xex::getBaseReference(u8 ref[20]) const
{
	if(m_baseReference == NULL)
		return false;
	memcpy(ref, m_baseReference, 20);
	return true;
}


bool Xex::hasDiscProfileId() const
{
	return m_discProfileId != NULL;
}
void Xex::clearDiscProfileId()
{
	if(m_discProfileId)
	{
		delete[] m_discProfileId;
		m_discProfileId = NULL;
	}
}
void Xex::setDiscProfileId(const DiscProfileId& id)
{
	clearDiscProfileId();
	m_discProfileId = new DiscProfileId;
	*m_discProfileId = id;
}
bool Xex::getDiscProfileId(DiscProfileId& id) const
{
	if(m_discProfileId == NULL)
		return false;
	id = *m_discProfileId;
	return true;
}


bool Xex::hasBoundingPath() const
{
	return m_boundingPath != NULL;
}
void Xex::clearBoundingPath()
{
	if(m_boundingPath)
	{
		delete[] m_boundingPath;
		m_boundingPath = NULL;
	}
}
void Xex::setBoundingPath(const char* path)
{
	clearBoundingPath();
	if(path)
	{
		m_boundingPath = new char[strlen(path) + 1];
		strcpy(m_boundingPath, path);
	}
}
bool Xex::getBoundingPath(char* path, s32 maxSize) const
{
	if(m_boundingPath == NULL)
		return false;
	strncpy(path, m_boundingPath, maxSize-1);
	path[maxSize-1] = 0;
	return true;
}


bool Xex::hasBoundingDeviceId() const
{
	return m_boundingDeviceId != NULL;
}
void Xex::clearBoundingDeviceId()
{
	if(m_boundingDeviceId)
	{
		delete[] m_boundingDeviceId;
		m_boundingDeviceId = NULL;
	}
}
void Xex::setBoundingDeviceId(const u8 id[20])
{
	clearBoundingDeviceId();
	if(id)
	{
		m_boundingDeviceId = new u8[20];
		memcpy(m_boundingDeviceId, id, 20);
	}
}
bool Xex::getBoundingDeviceId(u8 id[20]) const
{
	if(m_boundingDeviceId == NULL)
		return false;
	memcpy(id, m_boundingDeviceId, 20);
	return true;
}


s32  Xex::numImportLibraries() const
{
	return (s32)m_importLibraries.size();
}
void Xex::clearImportLibraries()
{
	m_importLibraries.clear();
}
bool Xex::removeImportLibrary(s32 index)
{
	if(index < 0 || index >= (s32)m_importLibraries.size())
		return false;
	std::vector <ImportLibraryData>::iterator iter;
	s32 i = index;
	for( iter = m_importLibraries.begin(); iter != m_importLibraries.end(); iter++ )	if(i-- == 0) break;
	m_importLibraries.erase(iter);
	return true;
}
void Xex::addImportLibrary(const char* name, XexVersion32 version, XexVersion32 minVersion, const DataBlock& addresses, u32 moduleNumber, u8 moduleIndex)
{
	ImportLibraryData lib_data;
	strncpy(lib_data.name, name, sizeof(lib_data.name));
	lib_data.name[sizeof(lib_data.name)-1] = 0;
	lib_data.version = version;
	lib_data.minVersion = minVersion;
	lib_data.addresses = addresses;
	lib_data.moduleNumber = moduleNumber;
	lib_data.moduleIndex = moduleIndex;
	m_importLibraries.push_back(lib_data);
}
bool Xex::getImportLibrary(s32 index, char* name, XexVersion32& version, XexVersion32& minVersion, DataBlock& addresses, u32& moduleNumber, u8& moduleIndex) const
{
	if(index < 0 || index >= (s32)m_importLibraries.size())
		return false;
	ImportLibraryData lib_data = m_importLibraries.at(index);
	strncpy(name, lib_data.name, sizeof(lib_data.name));
	name[sizeof(lib_data.name)-1] = 0;
	version = lib_data.version;
	minVersion = lib_data.minVersion;
	addresses = lib_data.addresses;
	moduleNumber = lib_data.moduleNumber;
	moduleIndex = lib_data.moduleIndex;
	return true;
}
bool Xex::setImportLibrary(s32 index, const char* name, XexVersion32 version, XexVersion32 minVersion, const DataBlock& addresses, u32 moduleNumber, u8 moduleIndex)
{
	if(index < 0 || index >= (s32)m_importLibraries.size())
		return false;
	ImportLibraryData lib_data;
	strncpy(lib_data.name, name, sizeof(lib_data.name));
	lib_data.name[sizeof(lib_data.name)-1] = 0;
	lib_data.version = version;
	lib_data.minVersion = minVersion;
	lib_data.addresses = addresses;
	lib_data.moduleNumber = moduleNumber;
	lib_data.moduleIndex = moduleIndex;
	m_importLibraries.at(index) = lib_data;
	return true;
}


bool Xex::hasCallCap() const
{
	return m_callCap != NULL;
}
void Xex::clearCallCap()
{
	if(m_callCap)
	{
		delete[] m_callCap;
		m_callCap = NULL;
	}
}
void Xex::setCallCap(u32 addr1, u32 addr2)
{
	clearCallCap();
	m_callCap = new u32[2];
	m_callCap[0] = addr1;
	m_callCap[1] = addr2;
}
bool Xex::getCallCap(u32& addr1, u32& addr2) const
{
	if(m_callCap == NULL)
		return false;
	addr1 = m_callCap[0];
	addr2 = m_callCap[1];
	return true;
}


bool Xex::hasFastCap() const
{
	return m_fastCap != NULL;
}
void Xex::clearFastCap()
{
	if(m_fastCap)
	{
		delete m_fastCap;
		m_fastCap = NULL;
	}
}
void Xex::setFastCap(u32 flag)
{
	clearFastCap();
	m_fastCap = new u32;
	*m_fastCap = flag;
}
bool Xex::getFastCap(u32& flag) const
{
	if(m_fastCap == NULL)
		return false;
	flag = *m_fastCap;
	return true;
}


bool Xex::hasExtraDebugMemory() const
{
	return m_extraDebugMemory != NULL;
}
void Xex::clearExtraDebugMemory()
{
	if(m_extraDebugMemory)
	{
		delete m_extraDebugMemory;
		m_extraDebugMemory = NULL;
	}
}
void Xex::setExtraDebugMemory(u32 size)
{
	clearExtraDebugMemory();
	m_extraDebugMemory = new u32;
	*m_extraDebugMemory = size;
}
bool Xex::getExtraDebugMemory(u32& size) const
{
	if(m_extraDebugMemory == NULL)
		return false;
	size = *m_extraDebugMemory;
	return true;
}


bool Xex::hasPageHeapInfo() const
{
	return m_pageheapInfo != NULL;
}
void Xex::clearPageHeapInfo()
{
	if(m_pageheapInfo)
	{
		delete m_pageheapInfo;
		m_pageheapInfo = NULL;
	}
}
void Xex::setPageHeapInfo(u32 size, u32 flags)
{
	clearPageHeapInfo();
	m_pageheapInfo = new u32[2];
	m_pageheapInfo[0] = size;
	m_pageheapInfo[1] = flags;
}
bool Xex::getPageHeapInfo(u32& size, u32& flags) const
{
	if(m_pageheapInfo == NULL)
		return false;
	size = m_pageheapInfo[0];
	flags = m_pageheapInfo[1];
	return true;
}


bool Xex::hasRestrictKVPrivs() const
{
	return m_restrictKVPrivs != NULL;
}
void Xex::clearRestrictKVPrivs()
{
	if(m_restrictKVPrivs)
	{
		delete m_restrictKVPrivs;
		m_restrictKVPrivs = NULL;
	}
}
void Xex::setRestrictKVPrivs(u64 mask, u64 val)
{
	clearRestrictKVPrivs();
	m_restrictKVPrivs = new u64[2];
	m_restrictKVPrivs[0] = mask;
	m_restrictKVPrivs[1] = val;
}
bool Xex::getRestrictKVPrivs(u64& mask, u64& val) const
{
	if(m_restrictKVPrivs == NULL)
		return false;
	mask = m_restrictKVPrivs[0];
	val = m_restrictKVPrivs[1];
	return true;
}


bool Xex::hasRestrictDates() const
{
	return m_restrictDates != NULL;
}
void Xex::clearRestrictDates()
{
	if(m_restrictDates)
	{
		delete m_restrictDates;
		m_restrictDates = NULL;
	}
}
void Xex::setRestrictDates(u64 start, u64 end)
{
	clearRestrictDates();
	m_restrictDates = new u64[2];
	m_restrictDates[0] = start;
	m_restrictDates[1] = end;
}
bool Xex::getRestrictDates(u64& start, u64& end) const
{
	if(m_restrictDates == NULL)
		return false;
	start = m_restrictDates[0];
	end = m_restrictDates[1];
	return true;
}


bool Xex::hasOriginalPEName() const
{
	return m_originalPEName != NULL;
}
void Xex::clearOriginalPEName()
{
	if(m_originalPEName)
	{
		delete[] m_originalPEName;
		m_originalPEName = NULL;
	}
}
void Xex::setOriginalPEName(const char* name)
{
	clearOriginalPEName();
	if(name)
	{
		m_originalPEName = new char[strlen(name) + 1];
		strcpy(m_originalPEName, name);
	}
}
bool Xex::getOriginalPEName(char* name, s32 maxSize) const
{
	if(m_originalPEName == NULL)
		return false;
	strncpy(name, m_originalPEName, maxSize-1);
	name[maxSize-1] = 0;
	return true;
}


s32  Xex::numLibraryVersions() const
{
	return (s32)m_staticLibraries.size();
}
void Xex::clearLibraryVersions()
{
	m_staticLibraries.clear();
}
bool Xex::removeLibraryVersion(s32 index)
{
	if(index < 0 || index >= (s32)m_staticLibraries.size())
		return false;
	std::vector <StaticLibraryEntry>::iterator iter;
	s32 i = index;
	for( iter = m_staticLibraries.begin(); iter != m_staticLibraries.end(); iter++ )	if(i-- == 0) break;
	m_staticLibraries.erase(iter);
	return true;
}
void Xex::addLibraryVersion(const char* name, const XexVersionInfo& version)
{
	StaticLibraryEntry lib_block;
	strncpy(lib_block.name, name, 8);
	lib_block.version = version;
	m_staticLibraries.push_back(lib_block);
}
bool Xex::getLibraryVersion(s32 index, char* name, XexVersionInfo& version) const
{
	if(index < 0 || index >= (s32)m_staticLibraries.size())
		return false;
	StaticLibraryEntry lib_block = m_staticLibraries.at(index);
	strncpy(name, lib_block.name, 8);
	name[8] = 0;
	version = lib_block.version;
	return true;
}
bool Xex::setLibraryVersion(s32 index, const char* name, const XexVersionInfo& version)
{
	if(index < 0 || index >= (s32)m_staticLibraries.size())
		return false;
	StaticLibraryEntry lib_block;
	strncpy(lib_block.name, name, 8);
	lib_block.version = version;
	m_staticLibraries.at(index) = lib_block;
	return true;
}


bool Xex::hasTLSInfo() const
{
	return m_tlsInfo != NULL;
}
void Xex::clearTLSInfo()
{
	if(m_tlsInfo)
	{
		delete m_tlsInfo;
		m_tlsInfo = NULL;
	}
}
void Xex::setTLSInfo(const TLSInfo& tls)
{
	clearTLSInfo();
	m_tlsInfo = new TLSInfo;
	*m_tlsInfo = tls;
}
bool Xex::getTLSInfo(TLSInfo& tls) const
{
	if(m_tlsInfo == NULL)
		return false;
	tls = *m_tlsInfo;
	return true;
}


bool Xex::hasStackSize() const
{
	return m_stackSize != NULL;
}
void Xex::clearStackSize()
{
	if(m_stackSize)
	{
		delete m_stackSize;
		m_stackSize = NULL;
	}
}
void Xex::setStackSize(s32 size)
{
	clearStackSize();
	m_stackSize = new s32;
	*m_stackSize = size;
}
bool Xex::getStackSize(s32& size) const
{
	if(m_stackSize == NULL)
		return false;
	size = *m_stackSize;
	return true;
}


bool Xex::hasFilesystemCacheSize() const
{
	return m_filesystemCacheSize != NULL;
}
void Xex::clearFilesystemCacheSize()
{
	if(m_filesystemCacheSize)
	{
		delete m_filesystemCacheSize;
		m_filesystemCacheSize = NULL;
	}
}
void Xex::setFilesystemCacheSize(s32 size)
{
	clearFilesystemCacheSize();
	m_filesystemCacheSize = new s32;
	*m_filesystemCacheSize = size;
}
bool Xex::getFilesystemCacheSize(s32& size) const
{
	if(m_filesystemCacheSize == NULL)
		return false;
	size = *m_filesystemCacheSize;
	return true;
}


bool Xex::hasHeapSize() const
{
	return m_heapSize != NULL;
}
void Xex::clearHeapSize()
{
	if(m_heapSize)
	{
		delete m_heapSize;
		m_heapSize = NULL;
	}
}
void Xex::setHeapSize(s32 size)
{
	clearHeapSize();
	m_heapSize = new s32;
	*m_heapSize = size;
}
bool Xex::getHeapSize(s32& size) const
{
	if(m_heapSize == NULL)
		return false;
	size = *m_heapSize;
	return true;
}


bool Xex::hasExecutionId() const
{
	return m_executionId != NULL;
}
void Xex::clearExecutionId()
{
	if(m_executionId)
	{
		delete m_executionId;
		m_executionId = NULL;
	}
}
void Xex::setExecutionId(const ExecutionId& exec_id)
{
	clearExecutionId();
	m_executionId = new ExecutionId;
	*m_executionId = exec_id;
}
bool Xex::getExecutionId(ExecutionId& exec_id) const
{
	if(m_executionId == NULL)
		return false;
	exec_id = *m_executionId;
	return true;
}


bool Xex::hasWorkspaceSize() const
{
	return m_workspaceSize != NULL;
}
void Xex::clearWorkspaceSize()
{
	if(m_workspaceSize)
	{
		delete m_workspaceSize;
		m_workspaceSize = NULL;
	}
}
void Xex::setWorkspaceSize(s32 size)
{
	clearWorkspaceSize();
	m_workspaceSize = new s32;
	*m_workspaceSize = size;
}
bool Xex::getWorkspaceSize(s32& size) const
{
	if(m_workspaceSize == NULL)
		return false;
	size = *m_workspaceSize;
	return true;
}


bool Xex::hasGameRatings() const
{
	return m_ratings != NULL;
}
void Xex::clearGameRatings()
{
	if(m_ratings)
	{
		delete m_ratings;
		m_ratings = NULL;
	}
}
void Xex::setGameRatings(const GameRatings& ratings)
{
	clearGameRatings();
	m_ratings = new GameRatings;
	*m_ratings = ratings;
}
bool Xex::getGameRatings(GameRatings& ratings) const
{
	if(m_ratings == NULL)
		return false;
	ratings = *m_ratings;
	return true;
}


bool Xex::hasLANKey() const
{
	return m_lanKey != NULL;
}
void Xex::clearLANKey()
{
	if(m_lanKey)
	{
		delete m_lanKey;
		m_lanKey = NULL;
	}
}
void Xex::setLANKey(const LANKey& lan_key)
{
	clearLANKey();
	m_lanKey = new LANKey;
	*m_lanKey = lan_key;
}
bool Xex::getLANKey(LANKey& lan_key) const
{
	if(m_lanKey == NULL)
		return false;
	lan_key = *m_lanKey;
	return true;
}


bool Xex::hasLogoData() const
{
	return m_logoData != NULL;
}
void Xex::clearLogoData()
{
	if(m_logoData)
	{
		delete m_logoData;
		m_logoData = NULL;
	}
}
void Xex::setLogoData(const DataBlock& logo_data)
{
	clearLogoData();
	m_logoData = new DataBlock();
	*m_logoData = logo_data;
}
bool Xex::getLogoData(DataBlock& logo_data) const
{
	if(m_logoData == NULL)
		return false;
	logo_data = *m_logoData;
	return true;
}


s32  Xex::numMultidiscMediaIds() const
{
	return (s32)m_multidiscMediaIds.size();
}
void Xex::clearMultidiscMediaIds()
{
	m_multidiscMediaIds.clear();
}


bool Xex::removeMultidiscMediaId(s32 index)
{
	if(index < 0 || index >= (s32)m_multidiscMediaIds.size())
		return false;
	std::vector <MediaId>::iterator iter;
	s32 i = index;
	for( iter = m_multidiscMediaIds.begin(); iter != m_multidiscMediaIds.end(); iter++ )	if(i-- == 0) break;
	m_multidiscMediaIds.erase(iter);
	return true;
}
void Xex::addMultidiscMediaId(const MediaId& media_id)
{
	m_multidiscMediaIds.push_back(media_id);
}
bool Xex::getMultidiscMediaId(s32 index, MediaId& media_id) const
{
	if(index < 0 || index >= (s32)m_multidiscMediaIds.size())
		return false;
	media_id = m_multidiscMediaIds.at(index);
	return true;
}
bool Xex::setMultidiscMediaId(s32 index, const MediaId& media_id)
{
	if(index < 0 || index >= (s32)m_multidiscMediaIds.size())
		return false;
	m_multidiscMediaIds.at(index) = media_id;
	return true;
}


s32  Xex::numAltTitleIds() const
{
	return (s32)m_altTitleIds.size();
}
void Xex::clearAltTitleIds()
{
	m_altTitleIds.clear();
}


bool Xex::removeAltTitleId(s32 index)
{
	if(index < 0 || index >= (s32)m_altTitleIds.size())
		return false;
	std::vector <u32>::iterator iter;
	s32 i = index;
	for( iter = m_altTitleIds.begin(); iter != m_altTitleIds.end(); iter++ )	if(i-- == 0) break;
	m_altTitleIds.erase(iter);
	return true;
}
void Xex::addAltTitleId(const u32& title_id)
{
	m_altTitleIds.push_back(title_id);
}
bool Xex::getAltTitleId(s32 index, u32& title_id) const
{
	if(index < 0 || index >= (s32)m_altTitleIds.size())
		return false;
	title_id = m_altTitleIds.at(index);
	return true;
}
bool Xex::setAltTitleId(s32 index, const u32& title_id)
{
	if(index < 0 || index >= (s32)m_altTitleIds.size())
		return false;
	m_altTitleIds.at(index) = title_id;
	return true;
}


bool Xex::hasExportsByName() const
{
	return m_exportsByName != NULL;
}
void Xex::clearExportsByName()
{
	if(m_exportsByName)
	{
		delete m_exportsByName;
		m_exportsByName = NULL;
	}
}
void Xex::setExportsByName(const ExportsByName& exports)
{
	clearExportsByName();
	m_exportsByName = new ExportsByName;
	*m_exportsByName = exports;
}
bool Xex::getExportsByName(ExportsByName& exports) const
{
	if(m_exportsByName == NULL)
		return false;
	exports = *m_exportsByName;
	return true;
}


s32  Xex::numRestrictConsoleIds() const
{
	return (s32)m_restrictConsoleIds.size();
}
void Xex::clearRestrictConsoleIds()
{
	m_restrictConsoleIds.clear();
}


bool Xex::removeRestrictConsoleId(s32 index)
{
	if(index < 0 || index >= (s32)m_restrictConsoleIds.size())
		return false;
	std::vector <ConsoleId>::iterator iter;
	s32 i = index;
	for( iter = m_restrictConsoleIds.begin(); iter != m_restrictConsoleIds.end(); iter++ )	if(i-- == 0) break;
	m_restrictConsoleIds.erase(iter);
	return true;
}
void Xex::addRestrictConsoleId(const ConsoleId& id)
{
	m_restrictConsoleIds.push_back(id);
}
bool Xex::getRestrictConsoleId(s32 index, ConsoleId& id) const
{
	if(index < 0 || index >= (s32)m_restrictConsoleIds.size())
		return false;
	id = m_restrictConsoleIds.at(index);
	return true;
}
bool Xex::setRestrictConsoleId(s32 index, const ConsoleId& id)
{
	if(index < 0 || index >= (s32)m_restrictConsoleIds.size())
		return false;
	m_restrictConsoleIds.at(index) = id;
	return true;
}


s32  Xex::numUnknownImageEntries() const
{
	return (s32)m_unknownImageEntries.size();
}
void Xex::clearUnknownImageEntries()
{
	m_unknownImageEntries.clear();
}
bool Xex::removeUnknownImageEntry(s32 index)
{
	if(index < 0 || index >= (s32)m_unknownImageEntries.size())
		return false;
	std::vector <UnknownImageEntry>::iterator iter;
	s32 i = index;
	for( iter = m_unknownImageEntries.begin(); iter != m_unknownImageEntries.end(); iter++ )	if(i-- == 0) break;
	m_unknownImageEntries.erase(iter);
	return true;
}
void Xex::addUnknownImageEntry(const XexImageEntry& header, const DataBlock& data)
{
	UnknownImageEntry info;
	info.header = header;
	info.data = data;
	m_unknownImageEntries.push_back(info);
}
bool Xex::getUnknownImageEntry(s32 index, XexImageEntry& header, DataBlock& data) const
{
	if(index < 0 || index >= (s32)m_unknownImageEntries.size())
		return false;
	UnknownImageEntry info = m_unknownImageEntries.at(index);
	header = info.header;
	data = info.data;
	return true;
}
bool Xex::setUnknownImageEntry(s32 index, const XexImageEntry& header, const DataBlock& data)
{
	if(index < 0 || index >= (s32)m_unknownImageEntries.size())
		return false;
	UnknownImageEntry info;
	info.header = header;
	info.data = data;
	m_unknownImageEntries.at(index) = info;
	return true;
}

