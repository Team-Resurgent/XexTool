// 
// XexDefines.h  -  xorloser 2006
// 
// This contains defines and structures for the xex file format
// the basic format of an xex file is as follows:
// 
//   |----------------------------|
//   |       XexImageHeader       |
//   |----------------------------|
//   |      XexImageEntry  0      |
//   |      XexImageEntry  1      |
//   |      XexImageEntry ...     |
//   |      XexImageEntry  n      |
//   |----------------------------|
//   |   Optional Zero Padding    |
//   |----------------------------|
//   |      XexSecurityInfo       |
//   |----------------------------|
//   |    XexHvSectionInfo  0     |
//   |    XexHvSectionInfo  1     |
//   |    XexHvSectionInfo ...    |
//   |    XexHvSectionInfo  n     |
//   |----------------------------|
//   |   XexImageEntry Data  0    |
//   |   XexImageEntry Data  1    |
//   |   XexImageEntry Data ...   |
//   |   XexImageEntry Data  n    |
//   |----------------------------|
//   |       Basefile Data        |
//   |----------------------------|
// 

#ifndef _XEX_DEFINES_H_
#define _XEX_DEFINES_H_

#include "types.h"
#include "Endian.h"
//#include "KernelDefines.h"

//typedef u8 XexHash[20];
//typedef u8 MediaId[16];
//typedef u8 XexKey[16];
typedef struct { u8 data[20]; } XexHash;
typedef struct { u8 data[16]; } MediaId;
typedef struct { u8 data[16]; } XexKey;


// initial header found at the start of an xex file
// (0x18 bytes)
typedef struct XexImageHeader
{
	/*000*/u8  magic[4];				// "XEX2"
	/*004*/u32 moduleFlags;				// ModuleFlag bit field
	/*008*/s32 sizeOfHeaders;			// size of headers
	/*00C*/s32 sizeOfDiscardableHeaders;// the xbox360 xex loader checks that this is 0
	/*010*/s32 securityInfoOffset;		// offset of security info from start of file
	/*014*/s32 imageEntryCount;			// number of XexImageEntrys
} XexImageHeader;

// an xex image directory entry. this contains an key and associated value.
// the associated value is either data or an offset to where the data is.
// these start directly after the XexImageHeader. ie offset 0x18 in the xex file
// XexImageHeader.headerDirectoryEntryCount specifies how many of these there are.
// (0x8 bytes)
typedef struct XexImageEntry
{
    /*00*/u32 key;					// key for entry type
    /*04*/u32 value;				// value specific to the entry type
} XexImageEntry;

// (0x174 bytes)
typedef struct XexHvImageInfo
{
	/*000*/u8  signature[256];		// hash over security info encrypted with MS private key
	/*100*/s32 infoSize;			// size of data over which 'signature' hash is calculated
	/*104*/u32 imageFlags;			// image flags bit field
	/*108*/u32 loadAddress;			// load address
	/*10C*/XexHash imageHash;		// hash over the first basefile section
	/*120*/s32 importTableCount;	// this is the number of import library sections
	/*124*/XexHash importHash;		// hash over the first import library info block
	/*138*/MediaId mediaId;			// XGD2 media id (0 if xex is not for XGD2 media)
	/*148*/XexKey imageKey;			// aes key used for decryption of basefile data
	/*158*/u32 exportTableAddress;	// address of export table in basefile data
	/*15C*/XexHash headerHash;		// hash over image entries and data, section info and xex header
	/*170*/u32 gameRegion;			// game region bit field
} XexHvImageInfo;

// these are often around offset 0x100 in the xex file
// they come after the XexImageEntrys in the file
// XexImageHeader.securityInfoOffset is the offset of this
// from the start of the xex file
// (0x184 bytes)
typedef struct XexSecurityInfo
{
	/*000*/s32 size;				// size of security info and all section info
	/*004*/s32 imageSize;			// size of full basefile image
	/*008*/XexHvImageInfo imageInfo;
	/*17C*/u32 allowedMediaTypes;	// allowed media bit field
	/*180*/s32 sectionCount;		// number of XexHvSectionInfos
} XexSecurityInfo;

// these are flags and hashes for sections of the exe file
// the flash and hash are for XexHvSectionInfo.size bytes of data
// NOTE: watch out for bitfields on machines with different endians!
// (0x14 bytes)
typedef struct XexHvSectionInfo
{
	union {
		struct {
#ifdef _BE_SYSTEM_
	/*00*/u32 size:28;				// size of this section in "number of pages"
	/*00*/u32 info:4;				// info value for the section
#else
	/*00*/u32 info:4;				// info value for the section
	/*00*/u32 size:28;				// size of this section in "number of pages"
#endif
		};
		u32 dword;
	};
	/*04*/XexHash hash;				// sha1 hash over the section
} XexHvSectionInfo;

// this is pointed to by XexHvImageInfo.exportTableAddress
// it is located inside the basefile data
typedef struct XexHvExportTable
{
	/*000*/u32 magic[3];			// "H\0\0\0", "\0HVE", "H\0\0\0"
	/*00C*/u32 moduleNumber[2];
	/*014*/u32 version[3];
	/*020*/u32 imageBaseAddress;	// needs to be shifted 16bits left
	/*024*/u32 count;				// number of exports
	/*028*/u32 base;				// base for ordinals
	/*02C*/u32 funcAddrs[1];
} XexHvExportTable;

// 32bit version info
// NOTE: watch out for bitfields on machines with different endians!
typedef union XexVersion32 {
	struct {
#ifdef _BE_SYSTEM_
	u32 major:4;					// most significant version info
	u32 minor:4;
	u32 build:16;
	u32 qfe:8;						// least significant version info
#else
	u32 qfe:8;						// least significant version info
	u32 build:16;
	u32 minor:4;
	u32 major:4;					// most significant version info
#endif
	};
	u32 dword;						// version as a single 32bit value
} XexVersion32;

// 64bit version info
// NOTE: watch out for bitfields on machines with different endians!
typedef struct XexVersionInfo {
	u16 major;
	u16 minor;
	u16 build;
	union {
		struct {
#ifdef _BE_SYSTEM_
		u16 qfe:8;					// 0x00FF : qfe version
		u16 unused1:2;				// 0x0300 : 0
		u16 tool:1;					// 0x0400 : 0
		u16 unused2:1;				// 0x0800 : 0
		u16 xexVersion:1;			// 0x1000 : set if lib is "executable version"
		u16 approvedlibrary:2;		// 0x6000 : 0=unapproved, 1=conditionally approved, 2=approved 3=expired?
		u16 debugBuild:1;			// 0x8000 : set if lib is "debug build"
#else
		u16 qfe:8;					// 0x00FF : qfe version
		// imagexex.exe tests show that for the following 8 bits:
		// flags & 0x30 == 0x70 : possibly approved
		// flags & 0x50 == 0x70 : approved
		// flags & 0x70 == 0x70 : expired
		// else                 : unapproved
		// It seems iumagexex requires the xexVersion bit to be set
		// before it will correctly handle the "approvedlibrary" flags.
		// 
		//   0x00=unapproved
		//   0x10=unapproved
		//   0x20=unapproved
		//   0x30=possibly approved
		//   0x40=unapproved
		//   0x50=approved
		//   0x60=unapproved
		//   0x70=expired
		//   0x80=unapproved
		//   0x90=unapproved
		//   0xA0=unapproved
		//   0xB0=possibly approved
		//   0xC0=unapproved
		//   0xD0=approved
		//   0xE0=unapproved
		//   0xF0=expired
		u16 unused1:2;				// 0x0300 : 0
		u16 tool:1;					// 0x0400 : 1=tool version
		u16 unused2:1;				// 0x0800 : 0
		u16 xexVersion:1;			// 0x1000 : set if lib is "executable version"
		u16 approvedlibrary:2;		// 0x6000 : 0=unapproved, 1=conditionally approved, 2=approved, 3=expired?
		u16 debugBuild:1;			// 0x8000 : set if lib is "debug build"
#endif
		};
		u16 word;
	};
} XexVersionInfo;




// section info values
#define SECTIONINFO_UNUSED0				0x00000000	// gives "game can't start" error if used
#define SECTIONINFO_CODE				0x00000001	// encrypted, code memory
#define SECTIONINFO_DATA				0x00000002	// unencrypted read/write data memory
#define SECTIONINFO_READONLY			0x00000003	// used for basefile headers and resources
#define SECTIONINFO_UNUSED4				0x00000004	// gives "game can't start" error if used

// module bitflags for XexImageHeader.moduleFlags
#define MODULEFLAG_TITLE_MODULE			0x00000001
#define MODULEFLAG_EXPORTS_TO_TITLE		0x00000002
#define MODULEFLAG_SYSTEM_DEBUGGER		0x00000004
#define MODULEFLAG_DLL_MODULE			0x00000008
#define MODULEFLAG_PATCH_MODULE			0x00000010
#define MODULEFLAG_PATCH_FULL			0x00000020
#define MODULEFLAG_PATCH_DELTA			0x00000040
#define MODULEFLAG_USER_MODE			0x00000080
#define MODULEFLAG_UNKNOWN				0xFFFFFF00

// image bitflags for XexHvImageInfo.imageFlags
#define IMAGEFLAG_MANUFACTURING_UTILITY	0x00000002	// util
#define IMAGEFLAG_MANUFACTURING_TOOL	0x00000004	// support
#define IMAGEFLAG_XGD2					0x00000008	// set this to allow xgd2 meda only
#define IMAGEFLAG_CARDEA_KEY			0x00000100	// WMDRM-ND
#define IMAGEFLAG_XEIKA_KEY				0x00000200	// AP25 related (is it really???)
#define IMAGEFLAG_TITLE_USERMODE		0x00000400	// is this what the xbox1 emu uses for dynamic recompilation?
#define IMAGEFLAG_SYSTEM_USERMODE		0x00000800
#define IMAGEFLAG_ORANGE0				0x00001000
#define IMAGEFLAG_ORANGE1				0x00002000
#define IMAGEFLAG_ORANGE2				0x00004000
//#define IMAGEFLAG_TESTKIT_RESTRICTED	0x00008000	// testkit restricted
#define IMAGEFLAG_SIGNED_KV_RESTRICTED	0x00008000	// signed keyvault restricted
#define IMAGEFLAG_IPTV_SIGNUP_APP		0x00010000	// iptv signup application
#define IMAGEFLAG_IPTV_TITLE_APP		0x00020000	// iptv title application
#define IMAGEFLAG_NCCP_KEYS				0x00040000	// ??
#define IMAGEFLAG_ACTIVATION_REQ		0x08000000	// online activation required
#define IMAGEFLAG_4K_PAGES				0x10000000	// 4kb page flag, otherwise pages are 64kb
#define IMAGEFLAG_NO_GAME_REGION		0x20000000
#define IMAGEFLAG_REVOCATION_CHECK_OPT	0x40000000	// revocation check optional
#define IMAGEFLAG_REVOCATION_CHECK_REQ	0x80000000	// revocation check required
#define IMAGEFLAG_UNKNOWN				0x07F800F1

// game regions for XexHvImageInfo.gameRegion
#define REGION_NORTH_AMERICA			0x000000FF
#define REGION_JAPAN					0x00000100
#define REGION_CHINA					0x00000200
#define REGION_REST_OF_ASIA				0x0000FC00
#define REGION_AUST_NZ					0x00010000
#define REGION_REST_OF_EUROPE			0x00FE0000
#define REGION_REST_OF_WORLD			0xFF000000
#define REGION_UNKNOWN					0x00000000
#define REGION_ALL						0xFFFFFFFF

// media types for XexSecurityInfo.allowedMediaTypes
#define MEDIATYPE_HARD_DISK				0x00000001
#define MEDIATYPE_DVDX2					0x00000002	// original xbox1 disc
#define MEDIATYPE_DVD_CD				0x00000004	// set this along with IMAGEFLAG_XGD2 for xgd2 media. clear this but set IMAGEFLAG_XGD2 for xgd2 updated media
#define MEDIATYPE_DVD5					0x00000008
#define MEDIATYPE_DVD9					0x00000010
#define MEDIATYPE_SYS_FLASH				0x00000020	// from system flash filesystem
#define MEDIATYPE_MEM_UNIT				0x00000080	// memory unit (external memory card)
#define MEDIATYPE_MASS_STORAGE			0x00000100	// usb mass storage device
#define MEDIATYPE_SMB					0x00000200	// networked smb share
#define MEDIATYPE_RAM					0x00000400
#define MEDIATYPE_RAM_DRIVE				0x00000800
#define MEDIATYPE_SECURE_VIRT_OD		0x00001000	// secure virtual optical device (svod)
#define MEDIATYPE_WIRELESS_N_STORAGE	0x00002000	// wirelessN storage device "\\Device\\Nomnil" (wirelessn)
#define MEDIATYPE_SYS_EXT_PARTITION		0x00004000	// system extended partition
#define MEDIATYPE_SYS_AUX_PARTITION		0x00008000	// system auxiliary partition
#define MEDIATYPE_INSECURE_PKG			0x01000000	// "CONS" package
#define MEDIATYPE_SAVEGAME_PKG			0x02000000	// "CONS" package
#define MEDIATYPE_LOCALSIGN_PKG			0x04000000	// "CONS" package
#define MEDIATYPE_LIVESIGN_PKG			0x08000000	// "LIVE" package
#define MEDIATYPE_XBOX_PKG				0x10000000	// "PIRS" package
#define MEDIATYPE_UNKNOWN				0xE0FF0040
#define MEDIATYPE_ALL					0xFFFFFFFF
//#define MEDIATYPE_CONSOLE_PKG			0x02000000	// "CONS" package
//#define MEDIATYPE_LOCAL_PKG			0x04000000	// "CONS" package
//#define MEDIATYPE_LIVE_PKG			0x08000000	// "LIVE" package
//#define MEDIATYPE_PIRS_PKG			0x10000000	// "PIRS" package


// system flags set in IMAGEKEY_SYSTEM_FLAGS  (0x00030000)
// system flags set in IMAGEKEY_SYSTEM_FLAGS2 (0x00030100)
#define SYSFLAG_NO_FORCE_REBOOT				0x00000001
#define SYSFLAG_FOREGROUND_TASKS			0x00000002
#define SYSFLAG_NO_ODD_MAPPING				0x00000004
#define SYSFLAG_HANDLE_MCE_INPUT			0x00000008
#define SYSFLAG_RESTRICT_HUD_FEATURES		0x00000010
#define SYSFLAG_HANDLE_GAMEPAD_DISCONNECT	0x00000020
#define SYSFLAG_INSECURE_SOCKETS			0x00000040
#define SYSFLAG_XBOX_1_XSP_INTEROP			0x00000080	// now called XEX_PRIVILEGE_RESERVED_7
#define SYSFLAG_SET_DASH_CONTEXT			0x00000100
#define SYSFLAG_USES_GAME_VOICE_CHANNEL		0x00000200
#define SYSFLAG_PAL50_INCOMPATIBLE			0x00000400
#define SYSFLAG_INSECURE_UTILITYDRIVE		0x00000800
#define SYSFLAG_HAS_XAM_HOOKS				0x00001000
#define SYSFLAG_PII							0x00002000
#define SYSFLAG_CROSSPLATFORM_SYSLINK		0x00004000
#define SYSFLAG_MULTIDISC_SWAP				0x00008000
#define SYSFLAG_MULTIDISC_INSECURE_MEDIA	0x00010000
#define SYSFLAG_AP25_MEDIA					0x00020000	// XEX_PRIVILEGE_AUTHENTICATION_EX_REQUIRED
#define SYSFLAG_NO_CONFIRM_EXIT				0x00040000
#define SYSFLAG_ALLOW_BKGRND_DOWNLOAD		0x00080000	// allow background downloading
#define SYSFLAG_CREATE_PERSIST_RAMDRIVE		0x00100000	// create persistable ramdrive
#define SYSFLAG_INHERIT_PERSIST_RAMDRIVE	0x00200000	// inherit persistable ramdrive
#define SYSFLAG_ALLOW_HUD_VIBRATION			0x00400000
#define SYSFLAG_BOTH_UTILITY_PARTITIONS		0x00800000	// allow access to both utility partitions
#define SYSFLAG_HANDLE_IPTV_INPUT			0x01000000
#define SYSFLAG_PREFER_BIGBUTTON_INPUT		0x02000000
#define SYSFLAG_ALLOW_XSAM_RESERVATION		0x04000000	// might be the flag for restricting keyvault privileges, now named XEX_PRIVILEGE_RESERVED_26
#define SYSFLAG_MULTIDISC_CROSS_TITLE		0x08000000
#define SYSFLAG_TITLE_INSTALL_INCOMPATIBLE	0x10000000
#define SYSFLAG_ALLOW_AVATAR_GET_METADATA	0x20000000	// ALLOW_AVATAR_GET_METADATA_BY_XUID
#define SYSFLAG_ALLOW_CONTROLLER_SWAPPING	0x40000000
#define SYSFLAG_DASH_EXTENSIBILITY_MODULE	0x80000000

#define SYSFLAG2_ALLOW_NETWORK_READ_CANCEL	0x00000001
#define SYSFLAG2_UNINTERRUPTABLE_READS		0x00000002
#define SYSFLAG2_REQUIRE_FULL_EXPERIENCE	0x00000004	// requires "Fall 2008 NXE" or later
#define SYSFLAG2_GAMEVOICE_REQUIRED_UI		0x00000008
#define SYSFLAG2_TITLE_SET_PRESENCE_STRING	0x00000010
#define SYSFLAG2_NATAL_TILTCONTROL			0x00000020
#define SYSFLAG2_REQUIRES_SKELETAL_TRACKING	0x00000040
#define SYSFLAG2_SUPPORTS_SKELETAL_TRACKING	0x00000080
#define SYSFLAG2_USE_LARGE_HDS_FILE_CACHE	0x00000100
#define SYSFLAG2_TITLE_SUPPORTS_DEEP_LINK	0x00000200

#define SYSFLAG_UNKNOWN						0x00000000
#define SYSFLAG2_UNKNOWN					0xFFFFFC00


#endif // _XEX_DEFINES_H_

