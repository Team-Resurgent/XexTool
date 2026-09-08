// 
// a block of data
// can insert and extract data from anywhere in the block
// 

#include "DataBlock.h"
#include <string.h>

#define min(a, b) (((a) < (b))?(a):(b))


DataBlock::DataBlock() :
	m_data(NULL),
	m_dataSize(0)
{
	// sets up 0 sized array so that theres always an array defined
	m_data = new u8[m_dataSize];
}

DataBlock::DataBlock(const DataBlock& block)
{
	m_dataSize = block.size();
	m_data = new u8[m_dataSize];
	block.get(m_data, 0, m_dataSize);
}

DataBlock& DataBlock::operator=(const DataBlock& block)
{
	if(this != &block)
	{
		delete[] m_data;
		m_data = NULL;
		
		m_dataSize = block.size();
		m_data = new u8[m_dataSize];
		block.get(m_data, 0, m_dataSize);
	}
	return *this;
}

DataBlock::~DataBlock()
{
	delete[] m_data;
	m_data = NULL;
}


s32  DataBlock::size() const
{
	return m_dataSize;
}

void DataBlock::clear()
{
	m_dataSize = 0;
	delete[] m_data;
	m_data = new u8[m_dataSize];
}

void DataBlock::truncate(s32 size)
{
	m_dataSize = size;
}

void DataBlock::fill(u8 data, s32 offset, s32 size)
{
	u8* fill_buff = new u8[size];
	memset(fill_buff, data, size);
	set(fill_buff, offset, size);
	delete[] fill_buff;
}

// read data in from file
s32  DataBlock::read( FILE* fd, s32 offset, s32 size)
{
	if(fd == NULL) return false;
	u8* data_buff = new u8[size];
	s32 size_read = (s32)fread(data_buff, 1, size, fd);
	set(data_buff, offset, size_read);
	delete[] data_buff;
	return size_read;
}

// write data out to file
s32  DataBlock::write(FILE* fd, s32 offset, s32 size)
{
	if(fd == NULL) return false;
	u8* data_buff = new u8[size];
	get(data_buff, offset, size);
	s32 size_written = (s32)fwrite(data_buff, 1, size, fd);
	delete[] data_buff;
	return size_written;
}

void DataBlock::set(const DataBlock& in, s32 inOffset, s32 thisOffset, s32 size)
{
	u8* data_buff = new u8[size];
	in.get(data_buff, inOffset, size);
	set(data_buff, thisOffset, size);
	delete[] data_buff;
}

void DataBlock::get(DataBlock& out, s32 outOffset, s32 thisOffset, s32 size) const
{
	u8* data_buff = new u8[size];
	get(data_buff, thisOffset, size);
	out.set(data_buff, outOffset, size);
	delete[] data_buff;
}

void DataBlock::set(const void* data, s32 offset, s32 size)
{
	if(offset + size > m_dataSize)
	{
		// need to allocate bigger buffer
		u8* new_buffer = new u8[offset+size];
		memcpy(new_buffer, m_data, min(m_dataSize, offset));
		if(offset-m_dataSize > 0)
			memset(&new_buffer[m_dataSize], 0, offset-m_dataSize);
		memcpy(&new_buffer[offset], data, size);
		
		delete[] m_data;
		m_data = new_buffer;
		m_dataSize = offset + size;
	}
	else
	{
		// can just insert into current buffer
		memcpy(&m_data[offset], data, size);
	}
}

void DataBlock::get(void* data, s32 offset, s32 size) const
{
	u8* data_ptr = (u8*)data;
	if(offset + size > m_dataSize)
	{
		// can only get data within buffer
		s32 getSize = m_dataSize-offset;
		if(getSize < 0)
			getSize = 0;
		memcpy(data_ptr, &m_data[offset], getSize);
		memset(&data_ptr[getSize], 0, size-getSize);
	}
	else
	{
		// can just retreive from the current buffer
		memcpy(data, &m_data[offset], size);
	}
}

void DataBlock::set8( u8  data, s32 offset)
{
	set(&data, offset, 1);
}
void DataBlock::set16(u16 data, s32 offset)
{
	set(&data, offset, 2);
}
void DataBlock::set32(u32 data, s32 offset)
{
	set(&data, offset, 4);
}
void DataBlock::set64(u64 data, s32 offset)
{
	set(&data, offset, 8);
}

u8   DataBlock::get8( s32 offset) const
{
	u8 data;
	get(&data, offset, 1);
	return data;
}
u16  DataBlock::get16(s32 offset) const
{
	u16 data;
	get(&data, offset, 2);
	return data;
}
u32  DataBlock::get32(s32 offset) const
{
	u32 data;
	get(&data, offset, 4);
	return data;
}
u64  DataBlock::get64(s32 offset) const
{
	u64 data;
	get(&data, offset, 8);
	return data;
}


// get and set in big endian
void DataBlock::set16be(u16 data, s32 offset)
{
	set8(data>> 8, offset+0);	set8(data>> 0, offset+1);
}
void DataBlock::set32be(u32 data, s32 offset)
{
	set8(data>>24, offset+0);	set8(data>>16, offset+1);
	set8(data>> 8, offset+2);	set8(data>> 0, offset+3);
}
void DataBlock::set64be(u64 data, s32 offset)
{
	set8((u8)(data>>56), offset+0);	set8((u8)(data>>48), offset+1);
	set8((u8)(data>>40), offset+2);	set8((u8)(data>>32), offset+3);
	set8((u8)(data>>24), offset+4);	set8((u8)(data>>16), offset+5);
	set8((u8)(data>> 8), offset+6);	set8((u8)(data>> 0), offset+7);
}
u16  DataBlock::get16be(s32 offset) const
{
	return	(get8(offset+0)<< 8) | (get8(offset+1)<< 0);
}
u32  DataBlock::get32be(s32 offset) const
{
	return	(get8(offset+0)<<24) | (get8(offset+1)<<16) |
			(get8(offset+2)<< 8) | (get8(offset+3)<< 0);
}
u64  DataBlock::get64be(s32 offset) const
{
	return	((u64)get8(offset+0)<<56) | ((u64)get8(offset+1)<<48) |
			((u64)get8(offset+2)<<40) | ((u64)get8(offset+3)<<32) |
			((u64)get8(offset+4)<<24) | ((u64)get8(offset+5)<<16) |
			((u64)get8(offset+6)<< 8) | ((u64)get8(offset+7)<< 0);
}

// get and set in little endian
void DataBlock::set16le(u16 data, s32 offset)
{
	set8(data>> 0, offset+0);	set8(data>> 8, offset+1);
}
void DataBlock::set32le(u32 data, s32 offset)
{
	set8(data>> 0, offset+0);	set8(data>> 8, offset+1);
	set8(data>>16, offset+2);	set8(data>>24, offset+3);
}
void DataBlock::set64le(u64 data, s32 offset)
{
	set8((u8)(data>> 0), offset+0);	set8((u8)(data>> 8), offset+1);
	set8((u8)(data>>16), offset+2);	set8((u8)(data>>24), offset+3);
	set8((u8)(data>>32), offset+4);	set8((u8)(data>>40), offset+5);
	set8((u8)(data>>48), offset+6);	set8((u8)(data>>56), offset+7);
}
u16  DataBlock::get16le(s32 offset) const
{
	return	(get8(offset+0)<< 0) | (get8(offset+1)<< 8);
}
u32  DataBlock::get32le(s32 offset) const
{
	return	(get8(offset+0)<< 0) | (get8(offset+1)<< 8) |
			(get8(offset+2)<<16) | (get8(offset+3)<<24);
}
u64  DataBlock::get64le(s32 offset) const
{
	return	((u64)get8(offset+0)<< 0) | ((u64)get8(offset+1)<< 8) |
			((u64)get8(offset+2)<<16) | ((u64)get8(offset+3)<<24) |
			((u64)get8(offset+4)<<32) | ((u64)get8(offset+5)<<40) |
			((u64)get8(offset+6)<<48) | ((u64)get8(offset+7)<<56);
}


// get a pointer to the const data inside the data block
// be careful when using this!
const void* DataBlock::cdata() const
{
	return m_data;
}


/*
#include <winsock2.h>
#define ntohll_macro(x) (((u64)(::ntohl((u32)((x << 32) >> 32))) << 32) | (u32)::ntohl(((u32)(x >> 32))))
#define htonll_macro(x) ntohll_macro(x)


void DataBlock::ntohs(s32 offset)
{
	set16( ::ntohs(get16(offset)), offset);
}
void DataBlock::ntohl(s32 offset)
{
	set32( ::ntohl(get32(offset)), offset);
}
void DataBlock::ntohll(s32 offset)
{
	set64( ntohll_macro(get64(offset)), offset);
}

void DataBlock::htons(s32 offset)
{
	set16( ::htons(get16(offset)), offset);
}
void DataBlock::htonl(s32 offset)
{
	set32( ::htonl(get32(offset)), offset);
}
void DataBlock::htonll(s32 offset)
{
	set64( htonll_macro(get64(offset)), offset);
}
*/

