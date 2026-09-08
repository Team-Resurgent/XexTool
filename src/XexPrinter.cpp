// 
// XexPrinter.cpp
// 
// prints xex info to iostream
// 

#include "XexPrinter.h"
#include "XexGameInfo.h"
#include "Xex.h"
#include <time.h>

XexPrinter::XexPrinter()
{
}
XexPrinter::~XexPrinter()
{
}

// print shortened info to iostream
void XexPrinter::printShort(const Xex& xex, FILE* stream) const
{
	fprintf(stream, "Xex Info\n");
	
	// basic text values
	if( xex.isDebug() )						fprintf(stream, "  %s\n", "Devkit");
	else									fprintf(stream, "  %s\n", "Retail");
	if( xex.isRaw() )						fprintf(stream, "  %s\n", "Uncompressed");
	if( xex.isBinary() )					fprintf(stream, "  %s\n", "Binary (No Compression)");
	if( xex.isCompressed() )				fprintf(stream, "  %s\n", "Compressed");
	if( xex.isDeltaCompressed() )			fprintf(stream, "  %s\n", "Delta Compressed");
	if( xex.isEncrypted() )					fprintf(stream, "  %s\n", "Encrypted");
	else									fprintf(stream, "  %s\n", "Not-Encrypted");
	
	if( xex.isTitleModule() )				fprintf(stream, "  %s\n", "Title Module");
	if( xex.isTitleExports() )				fprintf(stream, "  %s\n", "Title Exports");
	if( xex.isSystemDebugger() )			fprintf(stream, "  %s\n", "System Debugger");
	if( xex.isDllModule() )					fprintf(stream, "  %s\n", "DLL Module");
	if( xex.isPatchFull() )					fprintf(stream, "  %s\n", "Full Patch");
	if( xex.isPatchDelta() )				fprintf(stream, "  %s\n", "Delta Patch");
	if( xex.isUserMode() )					fprintf(stream, "  %s\n", "User Mode");
	if( xex.getUnknownModuleFlags() )		fprintf(stream, "  Unknown Module Flags: %08X\n", xex.getUnknownModuleFlags() );
	
	if( xex.isManufacturingUtility() )		fprintf(stream, "  %s\n", "Manufacturing Utility");
	if( xex.isManufacturingSupportTool() )	fprintf(stream, "  %s\n", "Manufacturing Support Tool");
	if( xex.isXGD2Only() )					fprintf(stream, "  %s\n", "XGD2 Only");
	if( xex.isCardeaKey() )					fprintf(stream, "  %s\n", "Cardea Key (WMDRM-ND)");
	if( xex.isXeikaKey() )					fprintf(stream, "  %s\n", "Xeika Key (AP25?)");
	if( xex.isTitleUsermode() )				fprintf(stream, "  %s\n", "Title Usermode");
	if( xex.isSystemUsermode() )			fprintf(stream, "  %s\n", "System Usermode");
	if( xex.isOrange0() )					fprintf(stream, "  %s\n", "Orange0");
	if( xex.isOrange1() )					fprintf(stream, "  %s\n", "Orange1");
	if( xex.isOrange2() )					fprintf(stream, "  %s\n", "Orange2");
//	if( xex.isTestkitRestricted() )			fprintf(stream, "  %s\n", "Restricted to Testkit");
	if( xex.isSignedKeyvaultRestricted() )	fprintf(stream, "  %s\n", "Restricted to Signed Kayvault");
	if( xex.isIptvSignupApp() )				fprintf(stream, "  %s\n", "IPTV Signup Application");
	if( xex.isIptvTitleApp() )				fprintf(stream, "  %s\n", "IPTV Title Application");
	if( xex.isNccpKeys() )					fprintf(stream, "  %s\n", "NCCP Keys");
	if( xex.isActivationReq() )				fprintf(stream, "  %s\n", "Activation Required");
	if( xex.isNoGameRegion() )				fprintf(stream, "  %s\n", "No Game Region");
	if( xex.hasOptionalRevocationCheck() )	fprintf(stream, "  %s\n", "Revocation Check Optional");
	if( xex.hasRequiredRevocationCheck() )	fprintf(stream, "  %s\n", "Revocation Check Required");
	if( xex.getUnknownImageFlags() )		fprintf(stream, "  Unknown Image Flags: %08X\n", xex.getUnknownImageFlags() );
	
	if( xex.isNoForcedReboot() )			fprintf(stream, "  %s\n", "No Forced Reboot");
	if( xex.isForegroundTasks() )			fprintf(stream, "  %s\n", "Foreground Tasks");
	if( xex.isNoODDMapping() )				fprintf(stream, "  %s\n", "No ODD Mapping");
	if( xex.isMceInputHandler() )			fprintf(stream, "  %s\n", "Handles MCE Input");
	if( xex.isRestrictedHudFeatures() )		fprintf(stream, "  %s\n", "Restricted HUD Features");
	if( xex.isGamepadDisconnectHandler() )	fprintf(stream, "  %s\n", "Handles Gampad Disconnect");
	if( xex.isInsecureSockets() )			fprintf(stream, "  %s\n", "Has Insecure Sockets");
	if( xex.isXbox1Interoperability() )		fprintf(stream, "  %s\n", "Xbox1 Interoperability");
	if( xex.isDashContext() )				fprintf(stream, "  %s\n", "Dash Context");
	if( xex.isGameVoiceChannelUser() )		fprintf(stream, "  %s\n", "Uses Game Voice Channel");
	if( xex.isPal50Incompatible() )			fprintf(stream, "  %s\n", "Pal50 Incompatible");
	if( xex.isInsecureUtilDriveUser() )		fprintf(stream, "  %s\n", "Insecure Utility Drive Support");
	if( xex.isXamHooks() )					fprintf(stream, "  %s\n", "Xam Hooks");
	if( xex.isPII() )						fprintf(stream, "  %s\n", "PII");
	if( xex.isCrossPlatformSyslinkUser() )	fprintf(stream, "  %s\n", "Cross Platform System Link");
	if( xex.isMultidiscSwap() )				fprintf(stream, "  %s\n", "Multidisc Swap");
	if( xex.isMultidiscInsecureMedia() )	fprintf(stream, "  %s\n", "Supports Insecure Multidisc Media");
	if( xex.isAP25Media() )					fprintf(stream, "  %s\n", "AP25 Media");
	if( xex.isNoCofirmExit() )				fprintf(stream, "  %s\n", "No Confirm Exit");
	if( xex.isAllowBackgroundDownload() )	fprintf(stream, "  %s\n", "Allow Background Downloading");
	if( xex.isCreatePersistRamdrive() )		fprintf(stream, "  %s\n", "Create Persistable Ram Drive");
	if( xex.isInheritPersistRamdrive() )	fprintf(stream, "  %s\n", "Inherit Persistent Ram Drive");
	if( xex.isAllowHudVibration() )			fprintf(stream, "  %s\n", "Allow HUD Vibration");
	if( xex.isBothUtilityPartitions() )		fprintf(stream, "  %s\n", "Allow Access to Both Utility Partitions");
	if( xex.isIptvInputHandler() )			fprintf(stream, "  %s\n", "Handles Input for IPTV");
	if( xex.isPreferBigButtonInput() )		fprintf(stream, "  %s\n", "Prefers Big Button Input");
	if( xex.isAllowXsamReservation() )		fprintf(stream, "  %s\n", "Allow Xsam Reservation");
	if( xex.isMultiDiscCrossTitle() )		fprintf(stream, "  %s\n", "Multidisc Cross Title");
	if( xex.isTitleInstallIncompatible() )	fprintf(stream, "  %s\n", "Title Install Incompatible");
	if( xex.isAllowAvatarGetMetadata() )	fprintf(stream, "  %s\n", "Allow Avatar Get Metadata By XUID");
	if( xex.isAllowControllerSwapping() )	fprintf(stream, "  %s\n", "Allow Controller Swapping");
	if( xex.isDashExtensibilityModule() )	fprintf(stream, "  %s\n", "Dash Extensibility Module");
	if( xex.isAllowNetworkReadCancel() )	fprintf(stream, "  %s\n", "Allow Network Read Cancel");
	if( xex.isUninterruptableReads() )		fprintf(stream, "  %s\n", "Uninterruptable Reads");
	if( xex.isRequiresNXE() )				fprintf(stream, "  %s\n", "Requires NXE");
	if( xex.isGamevoiceRequiredUI() )		fprintf(stream, "  %s\n", "Gamevoice Required UI");
	if( xex.isTitleSetsPresenceString() )	fprintf(stream, "  %s\n", "Title Sets Presence String");
	if( xex.isNatalTiltControl() )			fprintf(stream, "  %s\n", "Natal Tilt Control");
	if( xex.isSkeletalTrackingSupported() )	fprintf(stream, "  %s\n", "Supports Skeletal Tracking");
	if( xex.isSkeletalTrackingRequired() )	fprintf(stream, "  %s\n", "Requires Skeletal Tracking");
	if( xex.isLargeHdsFileCacheUsed() )		fprintf(stream, "  %s\n", "Uses Large HDS File Cache");
	if( xex.isTitleSupportsDeepLink() )		fprintf(stream, "  %s\n", "Title Supports Deep Link");
	
	if( xex.getUnknownSystemFlags() )		fprintf(stream, "  Unknown System Flags: %08X\n", xex.getUnknownSystemFlags() );
	
	if( xex.hasLogoData() )					fprintf(stream, "  Xbox360 Logo Data Present\n");
	if( xex.hasExportsByName() )			fprintf(stream, "  Exports by Name\n");
	
	if( xex.hasCallCap() )
	{
		u32 addr1, addr2;
		xex.getCallCap(addr1, addr2);
		fprintf(stream, "  Has Call Cap Data   (%08X %08X)\n", addr1, addr2);
	}
	if( xex.hasFastCap() )
	{
		u32 flags;
		xex.getFastCap(flags);
		fprintf(stream, "  Has Fast Cap Data   (%08X)\n", flags);
	}
	
	if( xex.hasExtraDebugMemory() )
	{
		u32 size;
		xex.getExtraDebugMemory(size);
		fprintf(stream, "  Requires %dMB Extra Debug Memory\n", size);
	}
	
	
	// basefile info
	fprintf(stream, "\nBasefile Info\n");
	// game name
	char gamename[1024] = "";
	XexGameInfo gameinfo;
	if( gameinfo.GameName(xex, gamename, sizeof(gamename)) )
	{
		fprintf(stream, "  Xex Name:           %s\n", gamename);
	}
	// original PE name
	if( xex.hasOriginalPEName() )
	{
		char name[128];
		xex.getOriginalPEName(name, 128);
		fprintf(stream, "  Original PE Name:   %s\n", name);
	}
	if(xex.isBasefilePE())
	{
		time_t filetime = xex.getFiletime();
		fprintf(stream, "  Load Address:       %08X\n", xex.getLoadAddress());
		fprintf(stream, "  Entry Point:        %08X\n", xex.getEntryPoint());
		fprintf(stream, "  Image Size:         %8X\n", xex.getImageSize());
		fprintf(stream, "  Page Size:          %8X\n", xex.getPageSize());
		fprintf(stream, "  Checksum:           %08X\n", xex.getChecksum());
//		fprintf(stream, "  ImportTableCount:   %08X\n", xex.getImportTableCount());
		if( xex.getExportTableAddress() )
			fprintf(stream, "  Export Table:       %08X\n", xex.getExportTableAddress());
		fprintf(stream, "  Filetime:           %08X - %s", xex.getFiletime(), ctime(&filetime));
//		fprintf(stream, "  Original Load Address: %08X\n", xex.getOrignalLoadAddress());
	}
	else if(xex.isBasefileXUIZ())
	{
		fprintf(stream, "  Basefile is a XUIZ file\n");
	}
	else
	{
		fprintf(stream, "  Unknown basefile type\n");
	}
	if( xex.hasStackSize() )
	{
		s32 size;
		xex.getStackSize(size);
		fprintf(stream, "  Stack Size:         %8X\n", size);
	}
	if( xex.hasHeapSize() )
	{
		s32 size;
		xex.getHeapSize(size);
		fprintf(stream, "  Heap Size:          %8X\n", size);
	}
	if( xex.hasPageHeapInfo() )
	{
		u32 size, flags;
		xex.getPageHeapInfo(size, flags);
		fprintf(stream, "  Page Heap Size:     %8X\n", size);
		fprintf(stream, "  Page Heap Flags:    %8X\n", flags);
	}
	if( xex.hasWorkspaceSize() )
	{
		s32 size;
		xex.getWorkspaceSize(size);
		fprintf(stream, "  Workspace Size:     %8X\n", size);
	}
	if( xex.hasFilesystemCacheSize() )
	{
		s32 size;
		xex.getFilesystemCacheSize(size);
		fprintf(stream, "  Filesystem Cache Size:  %8X\n", size);
	}
	
	fprintf(stream, "\nRegions\n");
	if( xex.isNoGameRegion() ||
		xex.isAllRegions() )
	{
		fprintf(stream, "  All Regions\n");
	}
	else
	{
		if( xex.isRegionNorthAmerica() )fprintf(stream, "  North America\n");
		if( xex.isRegionJapan() )		fprintf(stream, "  Japan\n");
		if( xex.isRegionChina() )		fprintf(stream, "  China\n");
		if( xex.isRegionRestOfAsia() )	fprintf(stream, "  Rest of Asia\n");
		if( xex.isRegionAustNZ() )		fprintf(stream, "  Australia / New Zealand\n");
		if( xex.isRegionRestOfEurope() )fprintf(stream, "  Rest of Europe\n");
		if( xex.isRegionRestOfWorld() )	fprintf(stream, "  Rest of the World\n");
	}
	
	fprintf(stream, "\nAllowed Media\n");
	if( !xex.isXGD2Only() && xex.isAllMediaTypes() )
	{
		fprintf(stream, "  All Media Types\n");
	}
	else if( xex.isXGD2Only() && xex.isMediaDvdCd() )
	{
		fprintf(stream, "  DVD-XGD2 (Xbox360 Original Disc)\n");
	}
	else if( xex.isXGD2Only() && !xex.isMediaDvdCd() )
	{
		fprintf(stream, "  Updated DVD-XGD2 (Updated version of Xbox360 Original Disc)\n");
	}
	else
	{
		if( xex.isMediaHardDisk() )				fprintf(stream, "  Hard Disk\n");
		if( xex.isMediaDvdX2() )				fprintf(stream, "  DVD-X2 (Xbox1 Original Disc)\n");
		if( xex.isMediaDvdCd() )				fprintf(stream, "  DVD / CD\n");
		if( xex.isMediaDvd5() )					fprintf(stream, "  DVD5\n");
		if( xex.isMediaDvd9() )					fprintf(stream, "  DVD9\n");
		if( xex.isMediaSystemFlash() )			fprintf(stream, "  System Flash\n");
		if( xex.isMediaMemoryUnit() )			fprintf(stream, "  Memory Unit\n");
		if( xex.isMediaMassStorage() )			fprintf(stream, "  Usb Mass Storage\n");
		if( xex.isMediaSMB() )					fprintf(stream, "  Networked SMB Share\n");
		if( xex.isMediaRam() )					fprintf(stream, "  Direct from Ram\n");
		if( xex.isMediaRamDrive() )				fprintf(stream, "  Ram Drive\n");
		if( xex.isMediaSecureVirtOD() )			fprintf(stream, "  Secure Virtual Optical Device\n");
		if( xex.isMediaWirelessNStorage() )		fprintf(stream, "  Wireless N Storage\n");
		if( xex.isMediaSystemExtPartition() )	fprintf(stream, "  System Extended Partition\n");
		if( xex.isMediaSystemAuxPartition() )	fprintf(stream, "  System Auxillary Partition\n");
		if( xex.isMediaInsecurePackage() )		fprintf(stream, "  Insecure Package (\"CONS\")\n");
		if( xex.isMediaSavegamePackage() )		fprintf(stream, "  Savegame Package (\"CONS\")\n");
		if( xex.isMediaLocallySignedPackage() )	fprintf(stream, "  Locally Signed Package (\"CONS\")\n");
		if( xex.isMediaLiveSignedPackage() )	fprintf(stream, "  Live Signed Package (\"LIVE\")\n");
		if( xex.isMediaXboxPackage() )			fprintf(stream, "  Xbox Package (\"PIRS\")\n");
		if( xex.getUnknownMediaTypes() )		fprintf(stream, "  Unknown Media: %08X\n", xex.getUnknownMediaTypes());
	}
	
	// media id
	MediaId media_id;
	xex.getMediaId(media_id);
	fprintf(stream, "\nMedia Id \n  ");	for(s32 i=0; i<16; i++)	fprintf(stream, "%02X ", media_id.data[i]); fprintf(stream, "\n");
	if( xex.numMultidiscMediaIds() )
	{
		fprintf(stream, "\nMultidisc Media Ids \n");
		for(s32 id_num=0; id_num<xex.numMultidiscMediaIds(); id_num++)
		{
			MediaId media_id;
			xex.getMultidiscMediaId(id_num, media_id);
			fprintf(stream, "  ");
			for(s32 i=0; i<16; i++)	fprintf(stream, "%02X ", media_id.data[i]);
			fprintf(stream, "\n");
		}
	}
	
	// disc profile id
	if( xex.hasDiscProfileId() )
	{
		DiscProfileId disc_profile_id;
		xex.getDiscProfileId(disc_profile_id);
		fprintf(stream, "\nDisc Profile Id \n  ");	for(s32 i=0; i<16; i++)	fprintf(stream, "%02X ", disc_profile_id.data[i]); fprintf(stream, "\n");
	}
	
	// image encryption key
	XexKey image_key;
	xex.getImageKey(image_key);
	fprintf(stream, "\nEncryption Key \n  ");	for(s32 i=0; i<16; i++)	fprintf(stream, "%02X ", image_key.data[i]); fprintf(stream, "\n");
	
	// LAN key
	if( xex.hasLANKey() )
	{
		LANKey lan_key;
		xex.getLANKey(lan_key);
		fprintf(stream, "\nLAN Key \n  ");	for(s32 i=0; i<16; i++)	fprintf(stream, "%02X ", lan_key.data[i]); fprintf(stream, "\n");
	}
	
	// base reference
	if( xex.hasBaseReference() )
	{
		fprintf(stream, "\nBase Reference\n  ");
		u8 ref[20];
		xex.getBaseReference(ref);
		for(s32 i=0; i<20; i++)	fprintf(stream, "%02X ", ref[i]);
		fprintf(stream, "\n");
	}
	
	
	if( xex.hasRestrictDates() ||
		xex.hasRestrictKVPrivs() ||
		xex.numRestrictConsoleIds() )
	{
		fprintf(stream, "\nRestrictions for Use\n");
	}

	// date restrictions
	if( xex.hasRestrictDates() )
	{
		u64 start, end;
		xex.getRestrictDates(start, end);
		fprintf(stream, "  Start Date:         %s\n", "");
		fprintf(stream, "  End Date:           %s\n", "");
	}
	
	// kv priv restrictions
	if( xex.hasRestrictKVPrivs() )
	{
		u64 mask, val;
		xex.getRestrictKVPrivs(mask, val);
		fprintf(stream, "  KeyVault Mask:      %s\n", "");
		fprintf(stream, "  KeyVault Value:     %s\n", "");
	}
	
	// console id restrictions
	if( xex.numRestrictConsoleIds() )
	{
		for(s32 i=0; i<xex.numRestrictConsoleIds(); i++)
		{
			ConsoleId console_id;
			xex.getRestrictConsoleId(i, console_id);
			fprintf(stream, "  ConsoleId %3d:      %s\n", i, "");
		}
	}
	
	// bounding path
	if( xex.hasBoundingPath() )
	{
		char* path = new char[1024];
		xex.getBoundingPath(path, 1024);
		fprintf(stream, "\nBounding Path\n  %s\n", path);
		delete[] path;
	}
	
	// bounding device id
	if( xex.hasBoundingDeviceId() )
	{
		fprintf(stream, "\nBounding Device Id\n  ");
		u8 id[20];
		xex.getBoundingDeviceId(id);
		for(s32 i=0; i<20; i++)	fprintf(stream, "%02X ", id[i]);
		fprintf(stream, "\n");
	}
	
	// tls info
	if( xex.hasTLSInfo() )
	{
		TLSInfo tls_info;
		xex.getTLSInfo(tls_info);
		fprintf(stream, "\nTLS Info\n");
		fprintf(stream, "  Number of Slots:    %d\n", tls_info.numSlots);
		fprintf(stream, "  Data Size:          %d\n", tls_info.dataSize);
		fprintf(stream, "  Raw Data Address:   %08X\n", tls_info.rawAddress);
		fprintf(stream, "  Raw Data Size:      %d\n", tls_info.rawSize);
	}
	
	// execution id
	if( xex.hasExecutionId() )
	{
		ExecutionId exec_id;
		xex.getExecutionId(exec_id);
		fprintf(stream, "\nExecution Id\n");
		fprintf(stream, "  Media Id:           %08X\n", exec_id.mediaId);
		unsigned char ch1 = (exec_id.titleId>>24)&0xFF;
		unsigned char ch2 = (exec_id.titleId>>16)&0xFF;
		if(	ch1 > ' ' && ch1 <= 'z' &&
			ch2 > ' ' && ch2 <= 'z' )
			fprintf(stream, "  Title Id:           %08X  (%c%c-%d)\n", exec_id.titleId, (exec_id.titleId>>24)&0xFF, (exec_id.titleId>>16)&0xFF, exec_id.titleId&0xFFFF);
		else
			fprintf(stream, "  Title Id:           %08X\n", exec_id.titleId);
		fprintf(stream, "  Savegame Id:        %08X\n", exec_id.saveGameId);
		fprintf(stream, "  Version:            v%d.%d.%d.%d\n", exec_id.version.major, exec_id.version.minor, exec_id.version.build, exec_id.version.qfe);
		fprintf(stream, "  Base Version:       v%d.%d.%d.%d\n", exec_id.baseVersion.major, exec_id.baseVersion.minor, exec_id.baseVersion.build, exec_id.baseVersion.qfe);
		fprintf(stream, "  Platform:           %X\n", exec_id.platform);
		fprintf(stream, "  Executable Type:    %X\n", exec_id.execType);
		fprintf(stream, "  Disc Number:        %d\n", exec_id.numDisc);
		fprintf(stream, "  Number of Discs:    %d\n", exec_id.maxDiscs);
	}
	
	// alternate title ids
	if( xex.numAltTitleIds() )
	{
		fprintf(stream, "\nAlternate Title Ids\n");
		for(s32 i=0; i<xex.numAltTitleIds(); i++)
		{
			u32 title_id = 0;
			xex.getAltTitleId(i, title_id);
			unsigned char ch1 = (title_id>>24)&0xFF;
			unsigned char ch2 = (title_id>>16)&0xFF;
			if(	ch1 > ' ' && ch1 <= 'z' &&
				ch2 > ' ' && ch2 <= 'z' )
				fprintf(stream, "  Title Id:           %08X  (%c%c-%d)\n", title_id, ch1, ch2, title_id&0xFFFF);
			else
				fprintf(stream, "  Title Id:           %08X\n", title_id);
		}
	}
	
	// game ratings
	if( xex.hasGameRatings() )
	{
		GameRatings ratings;
		xex.getGameRatings(ratings);
		printGameRatings(ratings, stream);
	}

	// unknown optional info
	if( xex.numUnknownImageEntries() )
	{
		fprintf(stream, "\n");
		fprintf(stream, "Unknown Image Entries\n");
		for(s32 entry_num=0; entry_num<xex.numUnknownImageEntries(); entry_num++)
		{
			XexImageEntry entry_header;
			DataBlock entry_data;
			xex.getUnknownImageEntry(entry_num, entry_header, entry_data);
			if( IS_IMAGEENTRY_DATA_OFFSET(entry_header) )
			{
				fprintf(stream, "  %3d)  %08X\n", entry_num, entry_header.key);
				for(s32 line_num=0; line_num*16<entry_data.size(); line_num++)
				{
					fprintf(stream, "  ");
					for(s32 col_num=0; col_num<16; col_num++)
					{
						if(line_num*16 + col_num > entry_data.size())
							break;
						fprintf(stream, "%02X ", entry_data.get8(line_num*16 + col_num));
					}
					fprintf(stream, "\n");
				}
			}
			else
			{
				fprintf(stream, "  %3d)  %08X - %08X\n", entry_num, entry_header.key, entry_header.value);
			}
		}
	}
}

// prints all xex info to fd
void XexPrinter::printAll(const Xex& xex, FILE* stream) const
{
	char str[128];
	
	// print all the short info plus some more
	printShort(xex, stream);
	
	// library versions
	if( xex.numLibraryVersions() )
	{
		const char* approved_strs[4] = {
			"Unapproved", "Conditionally Approved", "Approved", "Expired"
		};
		fprintf(stream, "\nStatic Libraries\n");
		for(s32 lib_num=0; lib_num<xex.numLibraryVersions(); lib_num++)
		{
			XexVersionInfo version;
			char name[12];
			xex.getLibraryVersion(lib_num, name, version);
			snprintf(str, sizeof(str), "%3d) %-14s v%d.%d.%d.%d", lib_num, name,
				version.major, version.minor, version.build, version.qfe);
			fprintf(stream, "  %-32s  (%s", str,
				approved_strs[version.approvedlibrary]);
			if( version.tool )		fprintf(stream, ", Tool Version");
			if( version.xexVersion )fprintf(stream, ", Executable Version");
			if( version.debugBuild )fprintf(stream, ", Debug Build");
			fprintf(stream, ")\n");
		}
	}
	
	// import libraries
	if( xex.numImportLibraries() )
	{
		fprintf(stream, "\nImport Libraries\n");
		for(s32 lib_num=0; lib_num<xex.numImportLibraries(); lib_num++)
		{
			char name[64];
			XexVersion32 version;
			XexVersion32 min_version;
			DataBlock addresses;
			u32 module_number;
			u8  module_index;
			xex.getImportLibrary(lib_num, name, version, min_version, addresses, module_number, module_index);
			snprintf(str, sizeof(str), "%3d) %-14s v%d.%d.%d.%d", lib_num, name,
				version.major, version.minor, version.build, version.qfe);
			fprintf(stream, "  %-32s  (min v%d.%d.%d.%d)\n", str,
				min_version.major, min_version.minor, min_version.build, min_version.qfe);
			
			// print out all adddresses and values there
/*			DataBlock basefile;
			xex.getBasefile(basefile);
			u32 load_addr = xex.getLoadAddress();
			s32 num_imports = addresses.size() / 4;
			for(s32 import_num=0; import_num<num_imports; import_num++)
			{
				u32 import_addr = addresses.get32(import_num * 4);
				fprintf(stream, "    %08X  %08X  %08X\n", import_addr,
					basefile.get32be(import_addr - load_addr + 0),
					basefile.get32be(import_addr - load_addr + 4));
			}
*/		}
	}
	
	// resources
	if( xex.numResources() )
	{
		fprintf(stream, "\n");
		fprintf(stream, "Resources\n");
		for(s32 res_num=0; res_num<xex.numResources(); res_num++)
		{
			u32 addr;
			s32 size;
			char name[12];
			xex.getResource(res_num, addr, size, name);
			fprintf(stream, "  %3d) %08X - %08X : %s\n", res_num, addr, addr + size, name);
		}
	}
	
	// sections
	if( xex.numSections() )
	{
		fprintf(stream, "\n");
		fprintf(stream, "Sections\n");
		u32 sec_addr = xex.getLoadAddress();
		for(s32 sec_num=0; sec_num<xex.numSections(); sec_num++)
		{
			s32 size;
			u8  type;
			xex.getSection(sec_num, size, type);
			
			const char type_strs[4][32] = {
				"Unknown",
				"Code",
				"Data",
				"Header/Resource"
			};
			const char* type_str_ptr = type_strs[0];
			if(type == SECTIONINFO_CODE)
				type_str_ptr = type_strs[1];
			else if(type == SECTIONINFO_DATA)
				type_str_ptr = type_strs[2];
			else if(type == SECTIONINFO_READONLY)
				type_str_ptr = type_strs[3];
			
			fprintf(stream, "  %3d) %08X - %08X : %s\n", sec_num, sec_addr, sec_addr + size, type_str_ptr);
			sec_addr += size;
		}
	}
}

void XexPrinter::printGameRatings(const GameRatings& ratings, FILE* stream) const
{
	char rating_str[32];
	fprintf(stream, "\nGame Ratings\n");
	
	// esrb
	switch(ratings.esrb)
	{
	case 0x00:	strcpy(rating_str, "ESRB_EC");	break;
	case 0x02:	strcpy(rating_str, "ESRB_E");	break;
	case 0x04:	strcpy(rating_str, "ESRB_E10+");break;
	case 0x06:	strcpy(rating_str, "ESRB_T");	break;
	case 0x08:	strcpy(rating_str, "ESRB_M");	break;
	case 0xFF:	strcpy(rating_str, "ESRB_RP");	break;
	default:	strcpy(rating_str, "UNRATED");	break;
	}
	fprintf(stream, "  ESRB:      %-16s  %02X\n", rating_str, ratings.esrb);
	
	// pegi
	switch(ratings.pegi)
	{
	case 0x00:	strcpy(rating_str, "PEGI_3");	break;
	case 0x04:	strcpy(rating_str, "PEGI_7");	break;
	case 0x09:	strcpy(rating_str, "PEGI_12");	break;
	case 0x0D:	strcpy(rating_str, "PEGI_16");	break;
	case 0x0E:	strcpy(rating_str, "PEGI_18");	break;
	default:	strcpy(rating_str, "UNRATED");	break;
	}
	fprintf(stream, "  PEGI:      %-16s  %02X\n", rating_str, ratings.pegi);

	// pegi-fi
	switch(ratings.pegi_fi)
	{
	case 0x00:	strcpy(rating_str, "PEGI_3");	break;
	case 0x04:	strcpy(rating_str, "PEGI_7");	break;
	case 0x08:	strcpy(rating_str, "PEGI_11");	break;
	case 0x0C:	strcpy(rating_str, "PEGI_15");	break;
	case 0x0E:	strcpy(rating_str, "PEGI_18");	break;
	default:	strcpy(rating_str, "UNRATED");	break;
	}
	fprintf(stream, "  PEGI-FI:   %-16s  %02X\n", rating_str, ratings.pegi_fi);

	// pegi-pt
	switch(ratings.pegi_pt)
	{
	case 0x01:	strcpy(rating_str, "PEGI_4");	break;
	case 0x03:	strcpy(rating_str, "PEGI_6");	break;
	case 0x09:	strcpy(rating_str, "PEGI_12");	break;
	case 0x0D:	strcpy(rating_str, "PEGI_16");	break;
	case 0x0E:	strcpy(rating_str, "PEGI_18");	break;
	default:	strcpy(rating_str, "UNRATED");	break;
	}
	fprintf(stream, "  PEGI-PT:   %-16s  %02X\n", rating_str, ratings.pegi_pt);

	// pegi-bbfc
	switch(ratings.pegi_bbfc)
	{
	case 0x00:	strcpy(rating_str, "PEGIBBFC_3");	break;
	case 0x01:	strcpy(rating_str, "PEGIBBFC_4");	break;
	case 0x04:	strcpy(rating_str, "PEGIBBFC_7");	break;
	case 0x05:	strcpy(rating_str, "PEGIBBFC_8");	break;
	case 0x09:	strcpy(rating_str, "PEGIBBFC_12");	break;
	case 0x0C:	strcpy(rating_str, "PEGIBBFC_15");	break;
	case 0x0D:	strcpy(rating_str, "PEGIBBFC_16");	break;
	case 0x0E:	strcpy(rating_str, "PEGIBBFC_18");	break;
	default:	strcpy(rating_str, "UNRATED");		break;
	}
	fprintf(stream, "  PEGI-BBFC: %-16s  %02X\n", rating_str, ratings.pegi_bbfc);

	// cero
	switch(ratings.cero)
	{
	/*
	case 0x00:	strcpy(rating_str, "CERO_A");	break;
	case 0x02:	strcpy(rating_str, "CERO_B");	break;
	case 0x04:	strcpy(rating_str, "CERO_C");	break;
	case 0x06:	strcpy(rating_str, "CERO_D");	break;
	case 0x08:	strcpy(rating_str, "CERO_Z");	break;
	*/
	case 0x00:	strcpy(rating_str, "CERO_CHILDREN");break;
	case 0x02:	strcpy(rating_str, "CERO_12");	break;
	case 0x04:	strcpy(rating_str, "CERO_15");	break;
	case 0x06:	strcpy(rating_str, "CERO_18");	break;
	case 0x08:	strcpy(rating_str, "CERO_Z");	break;
	default:	strcpy(rating_str, "UNRATED");	break;
	}
	fprintf(stream, "  CERO:      %-16s  %02X\n", rating_str, ratings.cero);

	// usk
	switch(ratings.usk)
	{
	case 0x00:	strcpy(rating_str, "USK_ALL_AGES");	break;
	case 0x02:	strcpy(rating_str, "USK_6");		break;
	case 0x04:	strcpy(rating_str, "USK_12");		break;
	case 0x06:	strcpy(rating_str, "USK_16");		break;
	case 0x08:	strcpy(rating_str, "USK_NO_YOUTH");	break;
	default:	strcpy(rating_str, "UNRATED");		break;
	}
	fprintf(stream, "  USK:       %-16s  %02X\n", rating_str, ratings.usk);
	
	// oflc au
	switch(ratings.oflc_au)
	{
	case 0x00:	strcpy(rating_str, "OFLC_AU_G");	break;
	case 0x02:	strcpy(rating_str, "OFLC_AU_G8");	break;
	case 0x03:	strcpy(rating_str, "OFLC_AU_PG");	break;
	case 0x04:	strcpy(rating_str, "OFLC_AU_M15");	break;
	case 0x05:	strcpy(rating_str, "OFLC_AU_M");	break;
	case 0x06:	strcpy(rating_str, "OFLC_AU_MA15");	break;
	default:	strcpy(rating_str, "UNRATED");		break;
	}
	fprintf(stream, "  OFLCAU:    %-16s  %02X\n", rating_str, ratings.oflc_au);
	
	// oflc nz
	switch(ratings.oflc_nz)
	{
	case 0x00:	strcpy(rating_str, "OFLC_NZ_ALL");	break;
	case 0x02:	strcpy(rating_str, "OFLC_NZ_PG");	break;
	case 0x04:	strcpy(rating_str, "OFLC_NZ_M15");	break;
	case 0x06:	strcpy(rating_str, "OFLC_NZ_MA15");	break;
	case 0x10:	strcpy(rating_str, "OFLC_NZ_M");	break;
	case 0x20:	strcpy(rating_str, "OFLC_NZ_R16");	break;
	case 0x30:	strcpy(rating_str, "OFLC_NZ_R18");	break;
	case 0x40:	strcpy(rating_str, "OFLC_NZ_R");	break;
	default:	strcpy(rating_str, "UNRATED");		break;
	}
	fprintf(stream, "  OFLCNZ:    %-16s  %02X\n", rating_str, ratings.oflc_nz);
	
	// kmrb
	switch(ratings.kmrb)
	{
	case 0xFF:
	default:	strcpy(rating_str, "UNRATED");		break;
	}
	fprintf(stream, "  KMRB:      %-16s  %02X\n", rating_str, ratings.kmrb);
	
	// brasil
	switch(ratings.brasil)
	{
	case 0xFF:
	case 0x10:	strcpy(rating_str, "BRAZIL_L");		break;
	case 0x20:	strcpy(rating_str, "BRAZIL_10");	break;
	case 0x30:	strcpy(rating_str, "BRAZIL_12");	break;
	case 0x40:	strcpy(rating_str, "BRAZIL_14");	break;
	case 0x50:	strcpy(rating_str, "BRAZIL_16");	break;
	case 0x60:	strcpy(rating_str, "BRAZIL_18");	break;
	default:	strcpy(rating_str, "UNRATED");		break;
	}
	fprintf(stream, "  BRASIL:    %-16s  %02X\n", rating_str, ratings.brasil);
	
	// fpb
	switch(ratings.fpb)
	{
	case 0x00:	strcpy(rating_str, "FPB_A");	break;
	case 0x02:	strcpy(rating_str, "FPB_PG");	break;
	case 0x06:	strcpy(rating_str, "FPB_PG");	break;
	case 0x07:	strcpy(rating_str, "FPB_10");	break;
	case 0x08:	strcpy(rating_str, "FPB_PG_CURRENT");	break;
	case 0x0A:	strcpy(rating_str, "FPB_13");	break;
	case 0x0D:	strcpy(rating_str, "FPB_16");	break;
	case 0x0E:	strcpy(rating_str, "FPB_18");	break;
	default:	strcpy(rating_str, "UNRATED");	break;
	}
	fprintf(stream, "  FPB:       %-16s  %02X\n", rating_str, ratings.fpb);
	
	// taiwan
	switch(ratings.taiwan)
	{
	case 0x01:	strcpy(rating_str, "TAIWAN_GENERAL");	break;
	case 0x02:	strcpy(rating_str, "TAIWAN_PROTECTED");	break;
	case 0x03:	strcpy(rating_str, "TAIWAN_PG");		break;
	case 0x04:	strcpy(rating_str, "TAIWAN_RESTRICTED");break;
	default:	strcpy(rating_str, "UNRATED");			break;
	}
	fprintf(stream, "  Taiwan:    %-16s  %02X\n", rating_str, ratings.fpb);
	
	// singapore
	switch(ratings.singapore)
	{
	case 0x01:	strcpy(rating_str, "SINGAPORE_GENERAL");	break;
	case 0x02:	strcpy(rating_str, "SINGAPORE_AGE_ADVISORY");break;
	case 0x03:	strcpy(rating_str, "SINGAPORE_MATURE_18");	break;
	default:	strcpy(rating_str, "UNRATED");	break;
	}
	fprintf(stream, "  Singapore: %-16s  %02X\n", rating_str, ratings.fpb);
}

