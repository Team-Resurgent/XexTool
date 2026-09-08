// 
// handles contents of xex
// 
// this includes all the data parts that make up an xex.
// it doesn't know anything about placement, ordering or
// file layout and endian of these contents however.
// 

#ifndef _XEX_H_
#define _XEX_H_

#include "types.h"
#include <stdio.h>
#include <vector>
#include "DataBlock.h"
#include "XexDefines.h"
#include "XexImageEntryTypes.h"

typedef struct UnknownImageEntry {
	XexImageEntry header;
	DataBlock data;
} UnknownImageEntry;

typedef struct BasefileSection {
	s32 size;
	u8  type;
} BasefileSection;

typedef struct ImportLibraryData {
	char name[64];
	XexVersion32 version;
	XexVersion32 minVersion;
	DataBlock addresses;
	u32 moduleNumber;
	u8  moduleIndex;
} ImportLibraryData;


class Xex {
public:
	Xex();
	Xex(Xex const& source);
	Xex& operator=(Xex const& source);
	~Xex();
	
	void clear();
	
	bool isDebug() const;
	bool isRetail() const;
	
	void setDebug();
	void setRetail();
	
	
	// 
	// X E X  H E A D E R
	// 
	
	bool isTitleModule() const;
	bool isDllModule() const;
	bool isPatchModule() const;
	bool isTitleExports() const;
	bool isSystemDebugger() const;
	bool isPatchFull() const;
	bool isPatchDelta() const;
	bool isUserMode() const;
	
	void setTitleModule(bool flag);
	void setDllModule(bool flag);
	void setPatchModule(bool flag);
	void setTitleExports(bool flag);
	void setSystemDebugger(bool flag);
	void setPatchFull(bool flag);
	void setPatchDelta(bool flag);
	void setUserMode(bool flag);
	
	u32  getUnknownModuleFlags() const;
	void setUnknownModuleFlags(u32 flags);
	u32  getModuleFlags();
	void setModuleFlags(u32 flags);
	
	s32  getDiscardableHeaderSize() const;
	void setDiscardableHeaderSize(s32 size);
	
	void getOriginalHeaders(DataBlock& headers) const;
	void setOriginalHeaders(const DataBlock& headers);
	
	void getPatchData(DataBlock& patchData) const;
	void setPatchData(const DataBlock& patchData);
	
	void getPatchInfo(DataBlock& patchInfo) const;
	void setPatchInfo(const DataBlock& patchInfo);
	
	
	// 
	// S E C U R I T Y   I N F O
	// 
	
	bool isManufacturingUtility() const;
	bool isManufacturingSupportTool() const;
	bool isXGD2Only() const;
	bool isCardeaKey() const;
	bool isXeikaKey() const;
	bool isTitleUsermode() const;
	bool isSystemUsermode() const;
	bool isOrange0() const;
	bool isOrange1() const;
	bool isOrange2() const;
//	bool isTestkitRestricted() const;
	bool isSignedKeyvaultRestricted() const;
	bool isIptvSignupApp() const;
	bool isIptvTitleApp() const;
	bool isNccpKeys() const;
	bool isActivationReq() const;
	bool isPageSize4KB() const;
	bool isPageSize64KB() const;
	bool isNoGameRegion() const;
	bool hasOptionalRevocationCheck() const;
	bool hasRequiredRevocationCheck() const;
	
	void setManufacturingUtility(bool flag);
	void setManufacturingSupportTool(bool flag);
	void setXGD2Only(bool flag);
	void setCardeaKey(bool flag);
	void setXeikaKey(bool flag);
	void setTitleUsermode(bool flag);
	void setSystemUsermode(bool flag);
	void setOrange0(bool flag);
	void setOrange1(bool flag);
	void setOrange2(bool flag);
//	void setTestkitRestricted(bool flag);
	void setSignedKeyvaultRestricted(bool flag);
	void setIptvSignupApp(bool flag);
	void setIptvTitleApp(bool flag);
	void setNccpKeys(bool flag);
	void setActivationReq(bool flag);
	void setPageSize4KB();
	void setPageSize64KB();
	void setNoGameRegion(bool flag);
	void setOptionalRevocationCheck(bool flag);
	void setRequiredRevocationCheck(bool flag);
	
	u32  getUnknownImageFlags() const;
	void setUnknownImageFlags(u32 flags);
	u32  getImageFlags();
	void setImageFlags(u32 flags);

	s32  getImageSize() const;
	void setLoadAddress(u32 addr);
	u32  getLoadAddress() const;
	void getMediaId(MediaId& id) const;
	void setMediaId(const MediaId& id);
	void getImageKey(XexKey& key) const;
	void setImageKey(const XexKey& key);
	u32  getExportTableAddress() const;
	void setExportTableAddress(u32 addr);
	
	bool isRegionNorthAmerica() const;
	bool isRegionJapan() const;
	bool isRegionChina() const;
	bool isRegionRestOfAsia() const;
	bool isRegionAustNZ() const;
	bool isRegionRestOfEurope() const;
	bool isRegionRestOfWorld() const;
	
	void setRegionNorthAmerica(bool flag);
	void setRegionJapan(bool flag);
	void setRegionChina(bool flag);
	void setRegionRestOfAsia(bool flag);
	void setRegionAustNZ(bool flag);
	void setRegionRestOfEurope(bool flag);
	void setRegionRestOfWorld(bool flag);
	
	bool isAllRegions() const;
	void setAllRegions();
	u32  getRegions();
	void setRegions(u32 regions);

	bool isMediaHardDisk() const;
	bool isMediaDvdX2() const;
	bool isMediaDvdCd() const;
	bool isMediaDvd5() const;
	bool isMediaDvd9() const;
	bool isMediaSystemFlash() const;
	bool isMediaMemoryUnit() const;
	bool isMediaMassStorage() const;
	bool isMediaSMB() const;
	bool isMediaRam() const;
	bool isMediaRamDrive() const;
	bool isMediaSecureVirtOD() const;
	bool isMediaWirelessNStorage() const;
	bool isMediaSystemExtPartition() const;
	bool isMediaSystemAuxPartition() const;
	bool isMediaInsecurePackage() const;
	bool isMediaSavegamePackage() const;
	bool isMediaLocallySignedPackage() const;
	bool isMediaLiveSignedPackage() const;
	bool isMediaXboxPackage() const;
	
	void setMediaHardDisk(bool flag);
	void setMediaDvdX2(bool flag);
	void setMediaDvdCd(bool flag);
	void setMediaDvd5(bool flag);
	void setMediaDvd9(bool flag);
	void setMediaSystemFlash(bool flag);
	void setMediaMemoryUnit(bool flag);
	void setMediaMassStorage(bool flag);
	void setMediaSMB(bool flag);
	void setMediaRam(bool flag);
	void setMediaRamDrive(bool flag);
	void setMediaSecureVirtOD(bool flag);
	void setMediaWirelessNStorage(bool flag);
	void setMediaSystemExtPartition(bool flag);
	void setMediaSystemAuxPartition(bool flag);
	void setMediaInsecurePackage(bool flag);
	void setMediaSavegamePackage(bool flag);
	void setMediaLocallySignedPackage(bool flag);
	void setMediaLiveSignedPackage(bool flag);
	void setMediaXboxPackage(bool flag);
	
	bool isAllMediaTypes() const;
	void setAllMediaTypes();
	u32  getUnknownMediaTypes() const;
	void setUnknownMediaTypes(u32 flags);
	u32  getMediaTypes();
	void setMediaTypes(u32 mtypes);
	
	s32  numSections() const;
	void clearSections();
	bool removeSection(s32 index);
	void addSection(s32 size, u8 type);
	bool getSection(s32 index, s32 &size, u8 &type) const;
	bool setSection(s32 index, s32 size, u8 type);
	
	
	// 
	// B A S E F I L E   I N F O
	// 
	
	s32  basefileSize() const;
	void clearBasefile();
	void getBasefile(DataBlock& basefile) const;
	bool setBasefile(const DataBlock& basefile);
	bool isBasefilePE() const;
	bool isBasefileExe() const;
	bool isBasefileDll() const;
	bool isBasefileXUIZ() const;
	
	
	// 
	// S Y S T E M   F L A G S
	// 
	
	bool isNoForcedReboot() const;
	bool isForegroundTasks() const;
	bool isNoODDMapping() const;
	bool isMceInputHandler() const;
	bool isRestrictedHudFeatures() const;
	bool isGamepadDisconnectHandler() const;
	bool isInsecureSockets() const;
	bool isXbox1Interoperability() const;
	bool isDashContext() const;
	bool isGameVoiceChannelUser() const;
	bool isPal50Incompatible() const;
	bool isInsecureUtilDriveUser() const;
	bool isXamHooks() const;
	bool isPII() const;
	bool isCrossPlatformSyslinkUser() const;
	bool isMultidiscSwap() const;
	bool isMultidiscInsecureMedia() const;
	bool isAP25Media() const;
	bool isNoCofirmExit() const;
	bool isAllowBackgroundDownload() const;
	bool isCreatePersistRamdrive() const;
	bool isInheritPersistRamdrive() const;
	bool isAllowHudVibration() const;
	bool isBothUtilityPartitions() const;
	bool isIptvInputHandler() const;
	bool isPreferBigButtonInput() const;
	bool isAllowXsamReservation() const;
	bool isMultiDiscCrossTitle() const;
	bool isTitleInstallIncompatible() const;
	bool isAllowAvatarGetMetadata() const;
	bool isAllowControllerSwapping() const;
	bool isDashExtensibilityModule() const;
	bool isAllowNetworkReadCancel() const;
	bool isUninterruptableReads() const;
	bool isRequiresNXE() const;
	bool isGamevoiceRequiredUI() const;
	bool isTitleSetsPresenceString() const;
	bool isNatalTiltControl() const;
	bool isSkeletalTrackingSupported() const;
	bool isSkeletalTrackingRequired() const;
	bool isLargeHdsFileCacheUsed() const;
	bool isTitleSupportsDeepLink() const;
	
	void setNoForcedReboot(bool flag);
	void setForegroundTasks(bool flag);
	void setNoODDMapping(bool flag);
	void setMceInputHandler(bool flag);
	void setRestrictedHudFeatures(bool flag);
	void setGamepadDisconnectHandler(bool flag);
	void setInsecureSockets(bool flag);
	void setXbox1Interoperability(bool flag);
	void setDashContext(bool flag);
	void setGameVoiceChannelUser(bool flag);
	void setPal50Incompatible(bool flag);
	void setInsecureUtilDriveUser(bool flag);
	void setXamHooks(bool flag);
	void setPII(bool flag);
	void setCrossPlatformSyslinkUser(bool flag);
	void setMultidiscSwap(bool flag);
	void setMultidiscInsecureMedia(bool flag);
	void setAP25Media(bool flag);
	void setNoCofirmExit(bool flag);
	void setAllowBackgroundDownload(bool flag);
	void setCreatePersistRamdrive(bool flag);
	void setInheritPersistRamdrive(bool flag);
	void setAllowHudVibration(bool flag);
	void setBothUtilityPartitions(bool flag);
	void setIptvInputHandler(bool flag);
	void setPreferBigButtonInput(bool flag);
	void setAllowXsamReservation(bool flag);
	void setMultiDiscCrossTitle(bool flag);
	void setTitleInstallIncompatible(bool flag);
	void setAllowAvatarGetMetadata(bool flag);
	void setAllowControllerSwapping(bool flag);
	void setDashExtensibilityModule(bool flag);
	void setAllowNetworkReadCancel(bool flag);
	void setUninterruptableReads(bool flag);
	void setRequiresNXE(bool flag);
	void setGamevoiceRequiredUI(bool flag);
	void setTitleSetsPresenceString(bool flag);
	void setNatalTiltControl(bool flag);
	void setSkeletalTrackingSupported(bool flag);
	void setSkeletalTrackingRequired(bool flag);
	void setLargeHdsFileCacheUsed(bool flag);
	void setTitleSupportsDeepLink(bool flag);

	u32  getUnknownSystemFlags() const;
	void setUnknownSystemFlags(u32 flags);
	bool hasSystemFlags() const;
	
	u32  getUnknownSystemFlags2() const;
	void setUnknownSystemFlags2(u32 flags);
	bool hasSystemFlags2() const;
	
	u32  getSystemFlags();
	void setSystemFlags(u32 flags);
	u32  getSystemFlags2();
	void setSystemFlags2(u32 flags);

	// 
	// O P T I O N A L  I N F O
	// 
	
	bool isBinary() const;
	bool isRaw() const;
	bool isCompressed() const;
	bool isDeltaCompressed() const;
	void setBinary();
	void setRaw();
	void setCompressed();
	void setDeltaCompressed();
	bool isEncrypted() const;
	void setEncrypted(bool toggle);
	
	// these are read-only features since their values come from the basefile
	// these only exist if the basefile is a PE file
	bool hasEntryPoint() const;
	bool hasOriginalLoadAddress() const;
	bool hasChecksum() const;
	bool hasFiletime() const;
	u32  getEntryPoint() const;
	u32  getOriginalLoadAddress() const;
	u32  getChecksum() const;
	u32  getFiletime() const;
	s32  getPageSize() const;
	void setHasOriginalLoadAddress(bool flag);
	
	s32  numResources() const;
	void clearResources();
	bool removeResource(s32 index);
	void addResource(u32 addr, s32 size, const char* name);
	bool getResource(s32 index, u32& addr, s32& size, char* name) const;
	bool setResource(s32 index, u32 addr, s32 size, const char* name);
	
	bool hasDeltaPatchDescriptor() const;
	void clearDeltaPatchDescriptor();
	void setDeltaPatchDescriptor(const DataBlock& patch);
	bool getDeltaPatchDescriptor(DataBlock& patch) const;

	bool hasBaseReference() const;
	void clearBaseReference();
	void setBaseReference(const u8 ref[20]);
	bool getBaseReference(u8 ref[20]) const;
	
	bool hasDiscProfileId() const;
	void clearDiscProfileId();
	void setDiscProfileId(const DiscProfileId& id);
	bool getDiscProfileId(DiscProfileId& id) const;
	
	bool hasBoundingPath() const;
	void clearBoundingPath();
	void setBoundingPath(const char* path);
	bool getBoundingPath(char* path, s32 maxSize) const;
	
	bool hasBoundingDeviceId() const;
	void clearBoundingDeviceId();
	void setBoundingDeviceId(const u8 id[20]);
	bool getBoundingDeviceId(u8 id[20]) const;
	
	s32  numImportLibraries() const;
	void clearImportLibraries();
	bool removeImportLibrary(s32 index);
	void addImportLibrary(const char* name, XexVersion32 version, XexVersion32 minVersion, const DataBlock& addresses, u32 moduleNumber, u8 moduleIndex);
	bool getImportLibrary(s32 index, char* name, XexVersion32& version, XexVersion32& minVersion, DataBlock& addresses, u32& moduleNumber, u8& moduleIndex) const;
	bool setImportLibrary(s32 index, const char* name, XexVersion32 version, XexVersion32 minVersion, const DataBlock& addresses, u32 moduleNumber, u8 moduleIndex);
	
	bool hasCallCap() const;
	void clearCallCap();
	void setCallCap(u32 addr1, u32 addr2);
	bool getCallCap(u32& addr1, u32& addr2) const;
	
	bool hasFastCap() const;
	void clearFastCap();
	void setFastCap(u32 flag);
	bool getFastCap(u32& flag) const;
	
	bool hasExtraDebugMemory() const;
	void clearExtraDebugMemory();
	void setExtraDebugMemory(u32 size);
	bool getExtraDebugMemory(u32& size) const;
	
	bool hasPageHeapInfo() const;
	void clearPageHeapInfo();
	void setPageHeapInfo(u32 size, u32 flags);
	bool getPageHeapInfo(u32& size, u32& flags) const;
	
	bool hasRestrictKVPrivs() const;
	void clearRestrictKVPrivs();
	void setRestrictKVPrivs(u64 mask, u64 val);
	bool getRestrictKVPrivs(u64& mask, u64& val) const;
	
	bool hasRestrictDates() const;
	void clearRestrictDates();
	void setRestrictDates(u64 start, u64 end);
	bool getRestrictDates(u64& start, u64& end) const;
	
	bool hasOriginalPEName() const;
	void clearOriginalPEName();
	void setOriginalPEName(const char* name);
	bool getOriginalPEName(char* name, s32 maxSize) const;
	
	s32  numLibraryVersions() const;
	void clearLibraryVersions();
	bool removeLibraryVersion(s32 index);
	void addLibraryVersion(const char* name, const XexVersionInfo& version);
	bool getLibraryVersion(s32 index, char* name, XexVersionInfo& version) const;
	bool setLibraryVersion(s32 index, const char* name, const XexVersionInfo& version);
	
	bool hasTLSInfo() const;
	void clearTLSInfo();
	void setTLSInfo(const TLSInfo& tls);
	bool getTLSInfo(TLSInfo& tls) const;
	
	bool hasStackSize() const;
	void clearStackSize();
	void setStackSize(s32 size);
	bool getStackSize(s32& size) const;
	
	bool hasFilesystemCacheSize() const;
	void clearFilesystemCacheSize();
	void setFilesystemCacheSize(s32 size);
	bool getFilesystemCacheSize(s32& size) const;
	
	bool hasHeapSize() const;
	void clearHeapSize();
	void setHeapSize(s32 size);
	bool getHeapSize(s32& size) const;
	
	bool hasExecutionId() const;
	void clearExecutionId();
	void setExecutionId(const ExecutionId& exec_id);
	bool getExecutionId(ExecutionId& exec_id) const;
	
	bool hasWorkspaceSize() const;
	void clearWorkspaceSize();
	void setWorkspaceSize(s32 size);
	bool getWorkspaceSize(s32& size) const;
	
	bool hasGameRatings() const;
	void clearGameRatings();
	void setGameRatings(const GameRatings& ratings);
	bool getGameRatings(GameRatings& ratings) const;
	
	bool hasLANKey() const;
	void clearLANKey();
	void setLANKey(const LANKey& lan_key);
	bool getLANKey(LANKey& lan_key) const;
	
	bool hasLogoData() const;
	void clearLogoData();
	void setLogoData(const DataBlock& logo_data);
	bool getLogoData(DataBlock& logo_data) const;
	
	s32  numMultidiscMediaIds() const;
	void clearMultidiscMediaIds();
	bool removeMultidiscMediaId(s32 index);
	void addMultidiscMediaId(const MediaId& media_id);
	bool getMultidiscMediaId(s32 index, MediaId& media_id) const;
	bool setMultidiscMediaId(s32 index, const MediaId& media_id);
	
	s32  numAltTitleIds() const;
	void clearAltTitleIds();
	bool removeAltTitleId(s32 index);
	void addAltTitleId(const u32& title_id);
	bool getAltTitleId(s32 index, u32& title_id) const;
	bool setAltTitleId(s32 index, const u32& title_id);
	
	bool hasExportsByName() const;
	void clearExportsByName();
	void setExportsByName(const ExportsByName& exports);
	bool getExportsByName(ExportsByName& exports) const;
	
	s32  numRestrictConsoleIds() const;
	void clearRestrictConsoleIds();
	bool removeRestrictConsoleId(s32 index);
	void addRestrictConsoleId(const ConsoleId& id);
	bool getRestrictConsoleId(s32 index, ConsoleId& id) const;
	bool setRestrictConsoleId(s32 index, const ConsoleId& id);
	
	s32  numUnknownImageEntries() const;
	void clearUnknownImageEntries();
	bool removeUnknownImageEntry(s32 index);
	void addUnknownImageEntry(const XexImageEntry& header, const DataBlock& data);
	bool getUnknownImageEntry(s32 index, XexImageEntry& header, DataBlock& data) const;
	bool setUnknownImageEntry(s32 index, const XexImageEntry& header, const DataBlock& data);
	
private:
	bool basefilePEData(void* data, s32 offset, s32 size) const;
	
	void copyData(Xex const& source);
	void freeData();
	
	// this is stored to allow for patches
	DataBlock m_originalHeaders;	// untouched headers from original xex file
	DataBlock m_patchData;			// untouched basefile from a patch file
	DataBlock m_patchInfo;			// untouched basefile info from a patch file
	
	// these exist for every xex
	bool m_isTitleModule;
	bool m_isTitleExports;
	bool m_isSystemDebugger;
	bool m_isDllModule;
	bool m_isPatchModule;
	bool m_isPatchFull;
	bool m_isPatchDelta;
	bool m_isUserMode;
	u32  m_unknownModuleFlags;
	s32  m_discardableHeaderSize;
	
	u32 m_imageFlags;
	u32 m_systemFlags;
	u32 m_systemFlags2;
	u32 m_regions;
	u32 m_mediaTypes;
	u32 m_loadAddress;
	u32 m_exportTableAddress;
	DataBlock m_basefile;		// unencrypted, uncompressed basefile data
	bool m_isDebug;				// flag for whether xex is debug
	bool m_isEncrypted;			// flag for whether basefile is encrypted
	bool m_isBinary;			// flag for whether basefile is binary
	bool m_isRaw;				// flag for whether basefile is raw
	bool m_isCompressed;		// flag for whether basefile is compressed
	bool m_isDeltaCompressed;	// flag for whether basefile is delta compressed
	MediaId m_mediaId;			// id used to lock xex to a specific DVD-XGD2 media
	XexKey m_imageKey;			// unencrypted key used for xex encryption
	std::vector<BasefileSection> m_sections;
	bool m_hasOriginalLoadAddress;
	
	// these dont necessarily exist for every xex
	u8*  m_baseReference;		// base reference for xex
	DiscProfileId* m_discProfileId;	// maybe XGD3 or AP25 related
	char* m_boundingPath;		// the only device path the xex is allowed to execute from
	u8*  m_boundingDeviceId;	// the id of the device that the xex must be executed from
	u32* m_callCap;
	u32* m_fastCap;
	u32* m_extraDebugMemory;
	u32* m_pageheapInfo;
	u64* m_restrictKVPrivs;
	u64* m_restrictDates;
	char* m_originalPEName;		// filename of PE that xex was generated from
	TLSInfo* m_tlsInfo;
	s32* m_stackSize;
	s32* m_filesystemCacheSize;
	s32* m_heapSize;
	ExecutionId* m_executionId;
	s32* m_workspaceSize;
	GameRatings* m_ratings;
	LANKey* m_lanKey;
	DataBlock* m_logoData;
	DataBlock* m_deltaPatch;
	ExportsByName* m_exportsByName;
	std::vector<ResourceEntry> m_resources;
	std::vector<ImportLibraryData> m_importLibraries;
	std::vector<StaticLibraryEntry> m_staticLibraries;
	std::vector<MediaId> m_multidiscMediaIds;
	std::vector<u32> m_altTitleIds;
	std::vector<ConsoleId> m_restrictConsoleIds;
	std::vector<UnknownImageEntry> m_unknownImageEntries;
};

#endif // _XEX_H_

