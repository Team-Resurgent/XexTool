// 
// a block of data that is basically like an array of variable size
// you can insert and extract data from anywhere in the block
// 
// the block will also change its size as required
// 

#ifndef _DATA_BLOCK_H_
#define _DATA_BLOCK_H_

#include <stdio.h>
#include "types.h"

class DataBlock
{
public:
	DataBlock();
	DataBlock(const DataBlock& block);
	DataBlock& operator=(const DataBlock& block);
	~DataBlock();
	
	s32  size() const;
	void clear();
	void truncate(s32 size);
	void fill(u8 data, s32 offset, s32 size);	// works like memset kinda
	s32  read( FILE* fd, s32 offset, s32 size);	// read data in from file
	s32  write(FILE* fd, s32 offset, s32 size);	// write data out to file
	
	// get and set variable sized data
	void set(const void* data, s32 offset, s32 size);
	void get(void* data, s32 offset, s32 size) const;
	void set(const DataBlock& in, s32 inOffset, s32 thisOffset, s32 size);
	void get(DataBlock& out, s32 outOffset, s32 thisOffset, s32 size) const;
	
	// get and set in system endian
	void set8( u8  data, s32 offset);
	void set16(u16 data, s32 offset);
	void set32(u32 data, s32 offset);
	void set64(u64 data, s32 offset);
	u8   get8( s32 offset) const;
	u16  get16(s32 offset) const;
	u32  get32(s32 offset) const;
	u64  get64(s32 offset) const;
	
	// get and set in big endian
	void set16be(u16 data, s32 offset);
	void set32be(u32 data, s32 offset);
	void set64be(u64 data, s32 offset);
	u16  get16be(s32 offset) const;
	u32  get32be(s32 offset) const;
	u64  get64be(s32 offset) const;
	
	// get and set in little endian
	void set16le(u16 data, s32 offset);
	void set32le(u32 data, s32 offset);
	void set64le(u64 data, s32 offset);
	u16  get16le(s32 offset) const;
	u32  get32le(s32 offset) const;
	u64  get64le(s32 offset) const;
	
	void byteswap(s32 offset, s32 size);
	
	// get a pointer to the const data inside the data block
	// be careful when using this!
	const void* cdata() const;
/*
	void ntohs(s32 offset);
	void ntohl(s32 offset);
	void ntohll(s32 offset);
	
	void htons(s32 offset);
	void htonl(s32 offset);
	void htonll(s32 offset);
*/	
private:
	u8* m_data;
	s32 m_dataSize;
};

#endif // _DATA_BLOCK_H_

