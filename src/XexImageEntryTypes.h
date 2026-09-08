// 
// XexImageEntryTypes.h  -  xorloser 2006
// 
// This file contains defines and structs to help map out
// the format of the data for each different image directory
// entry type.
// 
// The data for each entry is only present in a location
// seperate to the entry's key if the key ends with a number
// other than 0x00 or 0x01. The IS_IMAGEENTRY_DATA_OFFSET()
// macro can be used to check for this condition.
// 
// if a key is unknown, it will be ignored by the xbox360, it does
// not create any errors.
// 
// NOTE: all values are stored in big endian format since the
// xbox360 is big endian.
// 

#ifndef _XEX_IMAGE_ENTRY_TYPES_H_
#define _XEX_IMAGE_ENTRY_TYPES_H_

#include "types.h"
#include "XexDefines.h"

// macro for checking if XexImageEntry.value is an offset
#define IS_IMAGEENTRY_DATA_OFFSET(entry)	((((entry).key & 0xFF) != 0x00) && (((entry).key & 0xFF) != 0x01))

// macro for checking if XexImageEntrys data is of variable size
#define IS_IMAGEENTRY_DATA_VARIABLE_SIZE(entry) (((entry).key & 0xFF) == 0xFF)


// these are all the known keys for image entries
#define IMAGEKEY_RESOURCE_SECTION		0x000002FF
#define IMAGEKEY_BASEFILE_FORMAT		0x000003FF
#define IMAGEKEY_BASE_REFERENCE			0x00000405
#define IMAGEKEY_DELTA_PATCH_DESCRIPTOR	0x000005FF
#define IMAGEKEY_RESTRICT_KV_PRIVS		0x00004004		// keyvault privileges that must exist
#define IMAGEKEY_RESTRICT_DATES			0x00004104		// date start and stop to allow booting
#define IMAGEKEY_RESTRICT_CONSOLE_IDS	0x000042FF		// console id to allow execution on
#define IMAGEKEY_DISC_PROFILE_ID		0x00004304		// XGD3 related somehow
#define IMAGEKEY_BOUND_PATHNAME			0x000080FF
#define IMAGEKEY_BOUND_DEVICE_ID		0x00008105		// bind to a particular device
#define IMAGEKEY_ORIGINAL_BASE_ADDRESS	0x00010001
#define IMAGEKEY_ENTRY_POINT			0x00010100
#define IMAGEKEY_IMAGE_BASE_ADDRESS		0x00010201
#define IMAGEKEY_IMPORT_LIBRARIES		0x000103FF		// for importing from dlls
#define IMAGEKEY_IMAGE_CHECKSUM			0x00018002
#define IMAGEKEY_IMAGE_CALLCAP			0x00018102
#define IMAGEKEY_IMAGE_FASTCAP			0x00018200		// a single 32bit value (usually 0x00000001)
#define IMAGEKEY_ORIGINAL_PE_NAME		0x000183FF
#define IMAGEKEY_LIBRARY_VERSIONS		0x000200FF		// for statically linked libraries
#define IMAGEKEY_TLS_VALUES				0x00020104
#define IMAGEKEY_STACK_SIZE				0x00020200
#define IMAGEKEY_FILESYSTEM_CACHE_SIZE	0x00020301
#define IMAGEKEY_HEAP_SIZE				0x00020401
#define IMAGEKEY_PAGE_HEAP				0x00028002		// page heap size and flags
#define IMAGEKEY_SYSTEM_FLAGS			0x00030000
#define IMAGEKEY_xxxxxxxxxxx			0x00030001		// ?
#define IMAGEKEY_SYSTEM_FLAGS2			0x00030100
#define IMAGEKEY_EXECUTION_ID			0x00040006
#define IMAGEKEY_TITLE_WORKSPACE_SIZE	0x00040201
#define IMAGEKEY_GAME_RATINGS			0x00040310
#define IMAGEKEY_LAN_KEY				0x00040404
#define IMAGEKEY_LOGO_DATA				0x000405FF
#define IMAGEKEY_MULTIDISC_MEDIA_IDS	0x000406FF
#define IMAGEKEY_ALT_TITLE_IDS			0x000407FF		// alternate title ids
#define IMAGEKEY_EXTRA_DEBUG_MEMORY		0x00040801		// MB of additional memory (for running on debugs)
#define IMAGEKEY_EXPORTS_BY_NAME		0x00E10402


// IMAGEKEY_RESOURCE_SECTION		0x000002FF
// resources are included inside basefile

typedef struct ResourceEntry {
	char name[8];				// name of resource, has no null terminator
	u32  addr;					// address of resource
	s32  size;					// size of resource
} ResourceEntry;

typedef struct ResourceDirectory {
	s32  infoSize;				// size of full info block in bytes, includes this variable
	ResourceEntry entry[1];		// there is one or more of these depending on "infoSize"
} ResourceDirectory;


// IMAGEKEY_BASEFILE_FORMAT			0x000003FF
// basefile info depends on how the basefile is stored

// generic header on all basefile info blocks
typedef struct BaseFileInfoHeader {
	s32 infoSize;				// size of full info block in bytes, includes this variable
	u16 encType;				// 0=not-encrypted, 1=encrypted
	u16 compType;				// 1=not-compressed, 2=compressed, 3=delta-compressed
} BaseFileInfoHeader;

// info block for raw basefiles
typedef struct RawBaseFileBlock {
	s32 dataSize;				// size of basefile data block
	s32 zeroSize;				// size of zeroed data block
} RawBaseFileBlock;
typedef struct RawBaseFileInfo {
	s32 infoSize;				// size of full info block in bytes, includes this variable
	u16 encType;				// 0=not-encrypted, 1=encrypted
	u16 compType;				// 1=raw, 2=compressed, 3=delta-compressed
	RawBaseFileBlock block[1];	// there is one or more of these depending on infoSize
} RawBaseFileInfo;

// info block for compressed basefiles
// (both compressed and delta compressed are the same)
typedef struct CompBaseFileBlock {
    s32 dataSize;				// size of compressed data block
    XexHash hash;				// sha1 hash over compressed, unencrypted data block
} CompBaseFileBlock;
typedef struct CompBaseFileInfo {
	s32 infoSize;				// size of full info block in bytes, includes this variable
	u16 encType;				// 0=not-encrypted, 1=encrypted
	u16 compType;				// 1=raw, 2=compressed, 3=delta-compressed
	u32 compressionWindow;		// 0x8000
	CompBaseFileBlock block;	// there is only ever one of these in the info block
								// there is also one present at the start of each block in the compressed basefile
} CompBaseFileInfo;


// IMAGEKEY_BASE_REFERENCE			0x00000405
typedef struct BaseReference {
	u8 ref[20];
} BaseReference;

// IMAGEKEY_DELTA_PATCH_DESCRIPTOR	0x000005FF
typedef struct DeltaPatchDescriptor {
	s32 infoSize;				// size of full info block in bytes, includes this variable
	XexVersion32 targetVersion;
	XexVersion32 sourceVersion;
	u8  sourceHash[20];			// this is the hash over the signature of the source xex file
/*20*/XexKey imageKeySource;
/*30*/s32 targetHeaderSize;
	s32 deltaHeaderSourceOffset;
	s32 deltaHeaderSourceSize;
	s32 deltaHeaderTargetOffset;
/*40*/s32 deltaImageSourceOffset;
	s32 deltaImageSourceSize;
	s32 deltaImageTargetOffset;
	u8  patchData[1];			// this is the start of the first delta block
} DeltaPatchDescriptor;


// IMAGEKEY_RESTRICT_KV_PRIVS		0x00004004
// keyvault priv values required to allow xex to boot
typedef struct RestrictKVPrivs {
	u64 mask;					// mask to apply to value
	u64	val;					// required value after mask has been applied
} RestrictKVPrivs;

// IMAGEKEY_RESTRICT_DATES			0x00004104
// dates between which the xex is allowed to boot
typedef struct RestrictDates {
	u64	start;					// date to start allowing execution of this xex
	u64	end;					// date to stop allowing execution of this xex
} RestrictDates;

// IMAGEKEY_RESTRICT_CONSOLE_IDS	0x000042FF
// table of console ids that the xex is allowed to boot on
typedef struct ConsoleId {
	u8 id[5];
} ConsoleId;

typedef struct RestrictConsoleIds {
	s32 infoSize;				// size of full info block in bytes, includes this variable
	ConsoleId	ids[1];			// one or more console ids (each is 5bytes)
} RestrictConsoleIds;


// IMAGEKEY_DISC_PROFILE_ID			0x00004304
typedef XexKey DiscProfileId;


// IMAGEKEY_BOUND_PATHNAME			0x000080FF
// the only path where the xex can be successfully launched from
typedef struct BoundPathname {
	s32 infoSize;				// size of full info block in bytes, includes this variable
	char pathname[1];			// size of pathname depends on infoSize. it is a null terminated string
								// and is padded to a multiple of 4 bytes
} BoundPathname;

// IMAGEKEY_BOUND_DEVICE_ID			0x00008105
// the ID of the only device the xex can be successfully launched from
typedef struct BoundDeviceId {
	u8 id[20];						// device id
} BoundDeviceId;



// IMAGEKEY_IMPORT_LIBRARIES		0x000103FF
// this is information on imported functions from loaded dlls
// the import libraries consist of a directory filled with
// one or more entries

typedef struct ImportLibraryEntry {
/*00*/s32  infoSize;			// size of full info block in bytes, includes this variable
/*04*/XexHash hash;				// hash over the next import info block
								// the hash for the first block is stored in XexHvImageInfo.importHash
/*18*/u32  moduleNumber;		// ?
/*1C*/XexVersion32 version;		// library version expected
/*20*/XexVersion32 minVersion;	// minimum library version required
/*24*/u8   reserved;			// not used
/*25*/u8   moduleIndex;			// index of this module entry in the directory
/*26*/s16  numAddresses;		// number of addresses that follow
/*28*/u32  addresses[1];		// these are the addresses of the imports for this library
								// there are one or more of these depending on "numAddresses"
} ImportLibraryEntry;


typedef struct ImportLibraryDirectory {
	s32  infoSize;				// size of full info block in bytes, includes this variable
	s32  nameTableSize;			// size of library name strings "names"
	s32  numNames;				// the number of name strings in "name"
	char names[1];				// the name of each imported library is listed in the order
								// they appear in this import info. each name string is
								// null terminated and padded to a multiple of 4 bytes.
	// after names comes the ImportLibraryEntrys
} ImportLibraryDirectory;


// IMAGEKEY_IMAGE_CHECKSUM			0x00018002
// checksum and creation time of base file
typedef struct CheckSumTime {
	u32  checksum;				// checksum over basefile
	u32  filetime;				// creation time of basefile in filetime
} CheckSumTime;


// IMAGEKEY_IMAGE_CALLCAP			0x00018102
typedef struct CallCap {
	u32 addr1;
	u32 addr2;
} CallCap;


// IMAGEKEY_ORIGINAL_PE_NAME		0x000183FF
// name of the original PE file the xex was made from
typedef struct OriginalPEName {
	s32  infoSize;				// size of full info block in bytes, includes this variable
	char name[1];				// null terminated string padded to a multiple of 4 bytes
} OriginalPEName;


// IMAGEKEY_LIBRARY_VERSIONS		0x000200FF
// this is information on the versions of statically linked libraries present in the basefile

typedef struct StaticLibraryEntry {
	char name[8];				// 8 character name of library, padded with zeros, but no null terminator required
	XexVersionInfo version;		// library version and approval status of that version
} StaticLibraryEntry;

typedef struct StaticLibraryDirectory {
	s32  infoSize;				// size of full info block in bytes, includes this variable
	StaticLibraryEntry entry[1];// there are one or more of these depending on "infoSize"
} StaticLibraryDirectory;


// IMAGEKEY_TLS_VALUES				0x00020104
typedef struct TLSInfo {
	s32 numSlots;				// number of TLS slots
	u32 rawAddress;				// address of raw TLS data
	u32 rawSize;				// size of raw TLS data
	u32 dataSize;				// size of TLS data
} TLSInfo;


// IMAGEKEY_PAGE_HEAP				0x00028002
typedef struct PageHeap {
	u32	size;					// size of page heap
	u32	flags;					// flags for page heap
} PageHeap;


// IMAGEKEY_EXECUTION_ID			0x00040006
typedef struct ExecutionId {
	u32 mediaId;				// media id of xex (randomly assigned)
	XexVersion32 version;		// ???
	XexVersion32 baseVersion;	// ??? same as minimum version ?
	u32 titleId;				// title id of xex
	u8  platform;				// 2=xenon
	u8  execType;				// 1=game, 2=demo, 3=demoette
	u8  numDisc;				// number of disc in set that this xex is from
	u8  maxDiscs;				// number of discs in set
	u32 saveGameId;				// save game id
} ExecutionId;


// IMAGEKEY_GAME_RATINGS			0x00040310
typedef struct GameRatings {
	u8  esrb;
	u8  pegi;
	u8  pegi_fi;
	u8  pegi_pt;
	u8  pegi_bbfc;
	u8  cero;
	u8  usk;
	u8  oflc_au;
	u8  oflc_nz;
	u8  kmrb;
	u8  brasil;
	u8  fpb;
	u8  taiwan;
	u8  singapore;
	u8  reserved[50];			// reserved for future ratings specifications
} GameRatings;


// IMAGEKEY_LAN_KEY					0x00040404
typedef XexKey LANKey;


// IMAGEKEY_LOGO_DATA				0x000405FF
// 144 x 24 4bit logo
typedef struct LogoData {
	s32  infoSize;				// size of full info block in bytes, includes this variable
	s32  logoSize;				// size of logo data (does not incldue this variable)
	u8   logoData[1];			// the size of this depends on "logoSize"
} LogoData;


// IMAGEKEY_MULTIDISC_MEDIA_IDS		0x000406FF
typedef struct MultidiscMediaIds {
	s32  infoSize;				// size of full info block in bytes, includes this variable
	MediaId mediaId[1];			// contains one or more media ids depending on "infoSize"
} MultidiscMediaIds;


// IMAGEKEY_ALT_TITLE_IDS			0x000407FF
typedef struct AltTitleIds {
	s32  infoSize;				// size of full info block in bytes, includes this variable
	u32 titleId[1];				// contains one or more title ids depending on "infoSize"
} AltTitleIds;


// IMAGEKEY_EXPORTS_BY_NAME			0x00E10402
typedef struct ExportsByName {
	u32 exportTableOffset;		// offset in basefile of ExportByNameTable
	u32 exportTableSize;		// size of ExportByNameTable
} ExportsByName;

// actual export by name table - present in basefile
typedef struct ExportByNameTable {
/*00*/u32 characteristics;
	u32 timeDateStamp;
	u16 majorVersion;
	u16 minorVersion;
	u32 name;					// offset of export string table from start of exe file
/*10*/u32 base;					// base for export ordinals
	u32 numberOfFunctions;
	u32 numberOfNames;			// number of name strings in table
	u32 addressOfFunctions;		// offset of array of function addresses
/*20*/u32 addressOfNames;		// offset of array of name string offsets 
	u32 addressOfNameOrdinals;	// offset of 16bit array of ordinals
	u32 addressOfHVExport;
} ExportByNameTable;


#endif // _XEX_IMAGE_ENTRY_TYPES_H_

