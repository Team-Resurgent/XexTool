// 
// data type defs for easy cross-processor support
// 

#ifndef _COMMON_TYPES_
#define _COMMON_TYPES_


typedef unsigned char		u8;
typedef unsigned short		u16;
typedef unsigned int		u32;
typedef unsigned long long	u64;

typedef char				s8;	// matches xecryptTypes.h; distinct from signed char in C++
typedef signed short		s16;
typedef signed int			s32;
typedef signed long long	s64;


#if defined(_LANGUAGE_C_PLUS_PLUS)||defined(__cplusplus)||defined(c_plusplus)
#include <vector>
#include <string>
typedef std::vector<u8> xbytes_t;
typedef std::vector<u8>::iterator xbytes_iter_t;
typedef std::string xstr_t;
#endif


#ifndef NULL
#define NULL	0
#endif // NULL

typedef int                 BOOL;

#ifndef FALSE
#define FALSE               0
#endif

#ifndef TRUE
#define TRUE                1
#endif



// lets you align a numerical value to a multiple of something
#define ALIGN_VALUE(value, alignAmount)	(((value)+(alignAmount)-1) & (-(alignAmount)))


#if defined WIN32 || _WIN32 || WIN64 || _WIN64
	// windows specific stuff
	#define STRICMP		_stricmp
	#define FSEEK64		_fseeki64
	#define FTELL64		_ftelli64
	#define DIRSEPCHAR	'\\'
	#define MKDIR(dir)	_mkdir(dir)
	
	#include <direct.h>
	#include <io.h>
#else
	// "other systems" - yes there probably should be some system specific checks here...
	#define STRICMP		strcasecmp
	#define FSEEK64		fseeko
	#define FTELL64		ftello
	#define DIRSEPCHAR	'/'
	#define MKDIR(dir)	mkdir(dir, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH)
	
	#define _FILE_OFFSET_BITS 64
	#define __USE_FILE_OFFSET64
#endif


// this MUST be included after the "_FILE_OFFSET_BITS" define for
// 64bit fileio to work properly in linux etc
#include <stdio.h>


#endif // _COMMON_TYPES_


