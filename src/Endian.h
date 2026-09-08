// 
// endian related funcs
// 

#ifndef _ENDIAN_FUNCS_H_
#define _ENDIAN_FUNCS_H_

#include "types.h"

#if defined(_LANGUAGE_C_PLUS_PLUS)||defined(__cplusplus)||defined(c_plusplus)
extern "C" {
#endif



// uncomment one of the following if endian of the system this is being
// compiled for is known. otherwise leave both commented out.
//#define _BE_SYSTEM_		// uncomment this if running on a big endian system
//#define _LE_SYSTEM_		// uncomment this if running on a little endian system

#ifdef SN_TARGET_PS3
#define _BE_SYSTEM_
#endif

#ifdef _XBOX
  #define _BE_SYSTEM_
#elif defined WIN32 || _WIN32 || WIN64 || _WIN64
  #define _LE_SYSTEM_
#endif // WIN32

#ifdef _BE_SYSTEM_
#ifdef _LE_SYSTEM_
#error Only define either _BE_SYSTEM_ or _LE_SYSTEM_ in Endian.h
#endif // _LE_SYSTEM_
#endif //_BE_SYSTEM_

#ifndef _BE_SYSTEM_
#ifndef _LE_SYSTEM_
#error You must define either _BE_SYSTEM_ or _LE_SYSTEM_ in Endian.h
#endif // _LE_SYSTEM_
#endif //_BE_SYSTEM_


// functions to load and store values from ram independent of endian of system
static inline u16 GET16BE(const void* a)
{
#ifdef _BE_SYSTEM_
	return *(u16*)a;
#else
	u8* addr = (u8*)a;
	u16 val = (addr[0]<< 8) | (addr[1]<< 0);
	return val;
#endif
}
static inline u16 GET16LE(const void* a)
{
#ifdef _LE_SYSTEM_
	return *(u16*)a;
#else
	u8* addr = (u8*)a;
	u16 val = (addr[0]<< 0) | (addr[1]<< 8);
	return val;
#endif
}

static inline u32 GET24BE(const void* a)
{
	u8* addr = (u8*)a;
	u32 val = (addr[0]<<16) | (addr[1]<< 8) | (addr[2]<< 0);
	return val;
}
static inline u32 GET24LE(const void* a)
{
	u8* addr = (u8*)a;
	u32 val = (addr[2]<<16) | (addr[1]<< 8) | (addr[0]<< 0);
	return val;
}
static inline u32 GET24(const void* a)
{
	u8* addr = (u8*)a;
#ifdef _LE_SYSTEM_
	u32 val  = (addr[2]<<16) | (addr[1]<<8) | (addr[0]<<0);
#else
	u32 val  = (addr[0]<<16) | (addr[1]<<8) | (addr[2]<<0);
#endif
	return val;
}

static inline u32 GET32BE(const void* a)
{
#ifdef _BE_SYSTEM_
	return *(u32*)a;
#else
	u8* addr = (u8*)a;
	u32 val = (addr[0]<<24) | (addr[1]<<16)
			| (addr[2]<< 8) | (addr[3]<< 0);
	return val;
#endif
}
static inline u32 GET32LE(const void* a)
{
#ifdef _LE_SYSTEM_
	return *(u32*)a;
#else
	u8* addr = (u8*)a;
	u32 val = (addr[0]<< 0) | (addr[1]<< 8)
			| (addr[2]<<16) | (addr[3]<<24);
	return val;
#endif
}

static inline u64 GET64BE(const void* a)
{
#ifdef _BE_SYSTEM_
	return *(u64*)a;
#else
	u8* addr= (u8*)a;
	u64 val = ((u64)addr[0]<<56) | ((u64)addr[1]<<48)
			| ((u64)addr[2]<<40) | ((u64)addr[3]<<32)
			| ((u64)addr[4]<<24) | ((u64)addr[5]<<16)
			| ((u64)addr[6]<< 8) | ((u64)addr[7]<< 0);
	return val;
#endif
}
static inline u64 GET64LE(const void* a)
{
#ifdef _LE_SYSTEM_
	return *(u64*)a;
#else
	u8* addr = (u8*)a;
	u64 val = ((u64)addr[0]<< 0) | ((u64)addr[1]<< 8)
			| ((u64)addr[2]<<16) | ((u64)addr[3]<<24)
			| ((u64)addr[4]<<32) | ((u64)addr[5]<<40)
			| ((u64)addr[6]<<48) | ((u64)addr[7]<<56);
	return val;
#endif
}


static inline void SET16BE(void* a, u16 v)
{
#ifdef _BE_SYSTEM_
	*(u16*)a = v;
#else
	u16* addr = (u16*)a;
	u8*  val  = (u8*)&v;
	*addr = (val[0]<< 8) | (val[1]<< 0);
#endif
}
static inline void SET16LE(void* a, u16 v)
{
#ifdef _LE_SYSTEM_
	*(u16*)a = v;
#else
	u16* addr = (u16*)a;
	u8*  val  = (u8*)&v;
	*addr = (val[0]<< 0) | (val[1]<< 8);
#endif
}

static inline void SET24BE(const void* a, u32 v)
{
	u8* addr = (u8*)a;
	u8* val  = (u8*)&v;
#ifdef _BE_SYSTEM_
	addr[0] = val[1];
	addr[1] = val[2];
	addr[2] = val[3];
#else
	addr[0] = val[2];
	addr[1] = val[1];
	addr[2] = val[0];
#endif
}
static inline void SET24LE(const void* a, u32 v)
{
	u8* addr = (u8*)a;
	u8* val  = (u8*)&v;
#ifdef _BE_SYSTEM_
	addr[0] = val[3];
	addr[1] = val[2];
	addr[2] = val[1];
#else
	addr[0] = val[0];
	addr[1] = val[1];
	addr[2] = val[2];
#endif
}
static inline void SET24(const void* a, u32 v)
{
	u8* addr = (u8*)a;
	u8* val  = (u8*)&v;
#ifdef _BE_SYSTEM_
	addr[0] = val[1];
	addr[1] = val[2];
	addr[2] = val[3];
#else
	addr[0] = val[0];
	addr[1] = val[1];
	addr[2] = val[2];
#endif
}

static inline void SET32BE(void* a, u32 v)
{
#ifdef _BE_SYSTEM_
	*(u32*)a = v;
#else
	u32* addr = (u32*)a;
	u8*  val  = (u8*)&v;
	*addr	= (val[0]<<24) | (val[1]<<16)
			| (val[2]<< 8) | (val[3]<< 0);
#endif
}
static inline void SET32LE(void* a, u32 v)
{
#ifdef _LE_SYSTEM_
	*(u32*)a = v;
#else
	u32* addr = (u32*)a;
	u8*  val  = (u8*)&v;
	*addr	= (val[0]<< 0) | (val[1]<< 8)
			| (val[2]<<16) | (val[3]<<24);
#endif
}

static inline void SET64BE(void* a, u64 v)
{
#ifdef _BE_SYSTEM_
	*(u64*)a = v;
#else
	u64* addr = (u64*)a;
	u8*  val  = (u8*)&v;
	*addr	= ((u64)val[0]<<56) | ((u64)val[1]<<48)
			| ((u64)val[2]<<40) | ((u64)val[3]<<32)
			| ((u64)val[4]<<24) | ((u64)val[5]<<16)
			| ((u64)val[6]<< 8) | ((u64)val[7]<< 0);
#endif
}
static inline void SET64LE(void* a, u64 v)
{
#ifdef _LE_SYSTEM_
	*(u64*)a = v;
#else
	u64* addr = (u64*)a;
	u8*  val  = (u8*)&v;
	*addr	= ((u64)val[0]<< 0) | ((u64)val[1]<< 8)
			| ((u64)val[2]<<16) | ((u64)val[3]<<24)
			| ((u64)val[4]<<32) | ((u64)val[5]<<40)
			| ((u64)val[6]<<48) | ((u64)val[7]<<56);
#endif
}


#if defined(_LANGUAGE_C_PLUS_PLUS)||defined(__cplusplus)||defined(c_plusplus)
}
#endif
#endif // _ENDIAN_FUNCS_H_

