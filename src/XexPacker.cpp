// 
// packs exe-data in xex files
// 

#include "XexPacker.h"
#include "XeCryptCompat.h"
#include <string.h>
#include <stdio.h>
#include <vector>
#include "DataBlock.h"
#include "Endian.h"
#include "XexImageEntryTypes.h"
#include "XexLzx.h"
#include "lzx/lzx.h"

/*
// decrypt basefile in place
bool XexPacker::decryptDelta(DataBlock& basefile, const XexKey& xex_key, const XexKey& delta_key, const XexKey& patched_key,)
{
	// prepare decryption by setting decryption key
	XECRYPT_AES_STATE aes_ctx;
	XexKey crypt_key;
	XeCryptAesKey(&aes_ctx, patched_key.data);
//	XeCryptAesEcb(ctx, delta_key.data, xex_key.data, FALSE);
//	XeCryptAesEcb(ctx, patch_headers.secInfo.encKey, patch_headers.data, FALSE);
	XeCryptAesEcb(ctx, delta_key.data, xex_key.data, FALSE);
	XeCryptAesEcb(ctx, patch_headers.secInfo.encKey, patch_headers.data, FALSE);
	
	XeCryptAesKey(&aes_ctx, crypt_key.data);
	
	// get data to decrypt
	s32 crypt_size = basefile.size();
	u8* crypt_buff = new u8[crypt_size];
	basefile.get(crypt_buff, 0, crypt_size);
	
	// decrypt data
	u8 ivec[16];
	memset(ivec, 0, 16);
	XeCryptAesCbc(&aes_ctx, crypt_buff, crypt_size, crypt_buff, ivec, FALSE);
	
	// set decrypted data
	basefile.set(crypt_buff, 0, crypt_size);
	delete[] crypt_buff;

	return true;
}
*/

// decrypt basefile in place
bool XexPacker::decrypt(DataBlock& basefile, const XexKey& decKey)
{
	// prepare decryption by setting decryption key
	XECRYPT_AES_STATE aes_ctx;
	XeCryptAesKey(&aes_ctx, decKey.data);
	u8 ivec[16];
	memset(ivec, 0, 16);
	
	// get data to decrypt
	s32 crypt_size = basefile.size();
	u8* crypt_buff = new u8[crypt_size];
	basefile.get(crypt_buff, 0, crypt_size);
	
	// decrypt data
	XeCryptAesCbc(&aes_ctx, crypt_buff, crypt_size, crypt_buff, ivec, FALSE);
	
	// set decrypted data
	basefile.set(crypt_buff, 0, crypt_size);
	delete[] crypt_buff;

	return true;
}

// encrypt basefile in place
bool XexPacker::encrypt(DataBlock& basefile, const XexKey& encKey)
{
	// prepare encryption by setting encryption key
	XECRYPT_AES_STATE aes_ctx;
	XeCryptAesKey(&aes_ctx, encKey.data);
	u8 ivec[16];
	memset(ivec, 0, 16);
	
	// get data to encrypt
	s32 crypt_size = basefile.size();
	u8* crypt_buff = new u8[crypt_size];
	basefile.get(crypt_buff, 0, crypt_size);
	
	// encrypt data
	XeCryptAesCbc(&aes_ctx, crypt_buff, crypt_size, crypt_buff, ivec, TRUE);
	
	// set encrypted data
	basefile.set(crypt_buff, 0, crypt_size);
	delete[] crypt_buff;

	return true;
}


bool XexPacker::unpackBinary(DataBlock& basefileOut, const DataBlock& basefileIn,
				bool isEncrypted, const XexKey& encKey,
				const DataBlock& unpackInfo, s32 imageSize)
{
	// decrypt first
	const DataBlock* basefile_in_ptr = &basefileIn;
	DataBlock basefile_in_decrypted;
	if(isEncrypted)
	{
		basefile_in_decrypted = basefileIn;
		basefile_in_ptr = &basefile_in_decrypted;
		if( !decrypt(basefile_in_decrypted, encKey) )
			return false;
	}
	
	basefileOut.clear();
	basefileOut = *basefile_in_ptr;
	return true;
}

bool XexPacker::unpackRaw(DataBlock& basefileOut, const DataBlock& basefileIn,
				bool isEncrypted, const XexKey& encKey,
				const DataBlock& unpackInfo, s32 imageSize)
{
	// decrypt first
	const DataBlock* basefile_in_ptr = &basefileIn;
	DataBlock basefile_in_decrypted;
	if(isEncrypted)
	{
		basefile_in_decrypted = basefileIn;
		basefile_in_ptr = &basefile_in_decrypted;
		if( !decrypt(basefile_in_decrypted, encKey) )
			return false;
	}
	
/*
// dump to file
s32 data_size = basefile_in_ptr->size();
u8* data = new u8[data_size];
basefile_in_ptr->get(data, 0, data_size);
FILE* fd = fopen("dump.data", "w+b");
fwrite(data, data_size, 1, fd);
delete[] data;
fclose(fd);
*/

	// unpack decrypted basefile
	basefileOut.clear();
	s32 output_offset = 0;
	s32 input_offset = 0;
	s32 curr_image_size = 0;
	RawBaseFileBlock* unpack_info = new RawBaseFileBlock[(unpackInfo.size()-8) / sizeof(RawBaseFileBlock)];
	unpackInfo.get(unpack_info, 8, unpackInfo.size()-8);
	RawBaseFileBlock* unpack_info_ptr = unpack_info;
	
	while(output_offset < imageSize)
	{
		// handle the copying out of data for this block
		s32 unpack_data_size = unpack_info_ptr->dataSize;
		if(unpack_data_size)
		{
			// copy data out
			basefileOut.set(*basefile_in_ptr, input_offset, output_offset, unpack_data_size);
			
			input_offset += unpack_data_size;
			output_offset += unpack_data_size;
			curr_image_size += unpack_data_size;
		}
		
		// handle the zeroing for this block
		s32 unpack_zero_size = unpack_info_ptr->zeroSize;
		if(unpack_zero_size)
		{
			basefileOut.fill(0, output_offset, unpack_zero_size);
			
			output_offset += unpack_zero_size;
			curr_image_size += unpack_zero_size;
		}
//printf("unpack raw:  %d  |  %d\n", unpack_data_size, unpack_zero_size);
		if(unpack_zero_size == 0)
			break;
		
		unpack_info_ptr++;
	}
	
	// handle the zeroing for the last block
	s32 unpack_zero_size = imageSize - curr_image_size;
	if(unpack_zero_size)
	{
//printf("z - unpack raw:  %d  |  %d\n", 0, unpack_zero_size);
		basefileOut.fill(0, output_offset, unpack_zero_size);
	}
	
	delete[] unpack_info;
	return true;
}

bool XexPacker::unpackCompressed(DataBlock& basefileOut, const DataBlock& basefileIn,
				bool isEncrypted, const XexKey& encKey,
				const DataBlock& unpackInfo, s32 imageSize)
{
	// decrypt first
	const DataBlock* basefile_in_ptr = &basefileIn;
	DataBlock basefile_in_decrypted;
	if(isEncrypted)
	{
		basefile_in_decrypted = basefileIn;
		basefile_in_ptr = &basefile_in_decrypted;
		if( !decrypt(basefile_in_decrypted, encKey) )
			return false;
	}
	
/*
// dump to file
s32 data_size = basefile_in_ptr->size();
u8* data = new u8[data_size];
basefile_in_ptr->get(data, 0, data_size);
FILE* fd = fopen("dump.data", "w+b");
fwrite(data, data_size, 1, fd);
delete[] data;
fclose(fd);
*/	
	// unpack decrypted basefile
	bool success = true;
	u8* block_data = NULL;
	u8* uncomp_data = NULL;

	basefileOut.clear();
	s32 window_size = unpackInfo.get32(8);
	s32 max_uncomp_size = 0x8000;

	// One decoder for the whole basefile: the chunks below are pieces of a
	// single LZX stream and the window carries across them.
	XexLzxDecoder* decoder = XexLzxCreate((u32)window_size);
	if( decoder == NULL )
	{
		fprintf(stderr, "XexLzxCreate failed for window 0x%X\n", (unsigned)window_size);
		return false;
	}
	
	// unpacks blocks at a time, each block has multiple smaller blocks to decompress too
//s32 accumulate;
	s32 input_offset = 0;
	s32 output_offset = 0;
	s32 block_size = 0;
	CompBaseFileBlock unpack_info;
	while( output_offset < imageSize )
	{
		// get input data to decompress
		if(output_offset == 0)
		{
			unpack_info.dataSize = unpackInfo.get32(12);
			unpackInfo.get(&unpack_info.hash, 16, sizeof(XexHash));
			input_offset += block_size;
		}
		else
		{
			unpack_info.dataSize = basefile_in_ptr->get32be(input_offset);
			basefile_in_ptr->get(unpack_info.hash.data, input_offset+4, sizeof(XexHash));
			input_offset += block_size;
		}
		block_size = unpack_info.dataSize;
		block_data = new u8[block_size];
		basefile_in_ptr->get(block_data, input_offset, block_size);
//printf("%08X - %08X\n", input_offset, output_offset);
		
		// generate hash over data to ensure its valid before trying to decompress it
		XexHash hash;
		XeCryptSha(block_data, block_size, 0,0, 0,0, hash.data, sizeof(XexHash));
		if(memcmp(hash.data, unpack_info.hash.data, sizeof(XexHash)) != 0)
		{
//printf("bad hash!\n");
			success = false;
			goto finish_up;
		}
		
		// unpack smaller blocks inside current block
		uncomp_data = new u8[max_uncomp_size];
		u8* comp_ptr  = block_data + sizeof(CompBaseFileBlock);
		u8* uncomp_ptr = uncomp_data;
//accumulate = 0;
		while(1)
		{
			// now decompress data
			s32 comp_size = GET16BE(comp_ptr);
//if( GET16BE(comp_ptr) )
//	printf("    %4X - %4X\n", GET16BE(comp_ptr), GET16BE(comp_ptr+2));
//if(comp_size == 0xFDFD)
//{
//	printf("b00p\n");
//}
			comp_ptr += 2;
			if(comp_size == 0)
			{
//printf("  compsize == 0\n");
				break;
			}
			//s32 special_size = GET16BE(comp_ptr);
//printf("  %04X - %04X\n", comp_size, accumulate+=comp_size);
			s32 uncomp_size = 0x8000;
			if(uncomp_size > imageSize-output_offset)
				uncomp_size = imageSize-output_offset;

			if( !XexLzxDecodeChunk(decoder, comp_ptr, comp_size,
			                       uncomp_ptr, uncomp_size) )
			{
				success = false;
				goto finish_up;
			}
			basefileOut.set(uncomp_ptr, output_offset, uncomp_size);
			output_offset += uncomp_size;
			comp_ptr += comp_size;
		}
//printf("%4X\n", block_size);

		delete[] uncomp_data;
		uncomp_data = NULL;
		delete[] block_data;
		block_data = NULL;
	}
	
finish_up:
	XexLzxDestroy(decoder);
	if(block_data) delete[] block_data;
	if(uncomp_data)delete[] uncomp_data;
	return success;
}

/*
bool XexPacker::unpackDeltaCompressed(DataBlock& basefileOut, const DataBlock& basefileIn,
				bool isEncrypted, const XexKey& xex_key, const XexKey& patch_key, const XexKey& patch_hdr_key,
				const DataBlock& unpackInfo, s32 imageSize)
{
	// decrypt first
	const DataBlock* basefile_in_ptr = &basefileIn;
	DataBlock basefile_in_decrypted;
	if(isEncrypted)
	{
		basefile_in_decrypted = basefileIn;
		basefile_in_ptr = &basefile_in_decrypted;
		if( !decryptDelta(basefile_in_decrypted, xex_key, patch_key, patch_hdr_key) )
			return false;
	}
	
	// unpack decrypted basefile
	bool success = true;
	u8* block_data = NULL;
	u8* uncomp_data = new u8[imageSize];
	
	basefileOut.clear();
	s32 window_size = unpackInfo.get32(8);
	s32 source_size = window_size;
	CompBaseFileBlock unpack_info;
	unpack_info.dataSize = unpackInfo.get32(12);
	unpackInfo.get(unpack_info.hash, 16, sizeof(XexHash));
	
	s32 max_uncomp_size = 0x8000;
	
	// unpacks blocks at a time, each block has multiple smaller blocks to decompress too
	s32 input_offset = 0;
	s32 output_offset = 0;
	while( unpack_info.dataSize )
	{
		// get data to decompress
		s32 block_size = unpack_info.dataSize;
		block_data = new u8[block_size];
		basefile_in_ptr->get(block_data, input_offset, block_size);
		input_offset += block_size;
		
		// generate hash over data to ensure its valid before trying to decompress it
		XexHash hash;
		XeCryptSha(block_data, block_size, 0,0, 0,0, hash, sizeof(XexHash));
		if(memcmp(hash, unpack_info.hash, sizeof(XexHash)) != 0)
		{
			success = false;
			goto finish_up;
		}
		
		if( !XexpDeltaDecompress(ctx, uncomp_data, imageSize, block_data+24, block_size) )
		{
			success = false;
			goto finish_up;
		}
		
		// get update unpack_info for next block from header of this block
		unpack_info.dataSize = GET32BE(block_data + 0);
		memcpy(unpack_info.hash, block_data + 4, sizeof(XexHash));
		delete[] block_data;
		block_data = NULL;
	}
	basefileOut.set(uncomp_data, 0, imageSize);
	
finish_up:
	if(block_data) delete[] block_data;
	if(uncomp_data)delete[] uncomp_data;
	return success;
}
*/

bool XexPacker::packBinary(DataBlock& basefileOut, const DataBlock& basefileIn,
				bool isEncrypted, const XexKey& encKey,
				DataBlock& unpackInfo, s32 imageSize)
{
	basefileOut.clear();
	basefileOut = basefileIn;
	
	// now encrypt
	if( isEncrypted )
	{
		if( !encrypt(basefileOut, encKey) )
			return false;
	}
	
	// update unpack info
	unpackInfo.clear();
	s32 size = 0x10;
	unpackInfo.set32( size, 0 );
	u16 type = (isEncrypted) ? 1 : 0;
	unpackInfo.set16( type, 4 );
	type = 1;
	unpackInfo.set16( type, 6 );
	size = basefileOut.size();
	unpackInfo.set32( size, 8 );
	unpackInfo.set32( 0, 12 );
	return true;
}

bool XexPacker::packRaw(DataBlock& basefileOut, const DataBlock& basefileIn,
				bool isEncrypted, const XexKey& encKey,
				DataBlock& unpackInfo, s32 imageSize)
{
	basefileOut.clear();
	unpackInfo.clear();
	unpackInfo.set32(0, 0);
	unpackInfo.set16(0, 4);
	unpackInfo.set16(0, 6);
	
	s32 page_size = 0x8000;
	
	s32 data_size = basefileIn.size();
	u8* data = new u8[data_size];
	basefileIn.get(data, 0, data_size);
	u8* data_end = data + data_size;
	
	// do raw packing of data
	// this just packs blocks of non-zero data and blocks of zero data.
	s32 data_block_size = 0;
	s32 zero_block_size = 0;
	s32 output_offset = 0;
	s32 data_offset = 0;
	while( data_offset < data_size )
	{
		// try to find zero block
		// (a data block must exist before the zero block though)
		if(	data_block_size != 0 &&
			data_offset % page_size == 0)
		{
			for(u8* data_ptr = data + data_offset; data_ptr < data_end; data_ptr++)
			{
				if(*data_ptr != 0)
					break;
				zero_block_size++;
			}
		}
		zero_block_size &= -page_size;
		
		// check if a zero_block was found that is big enough to bother using
		// or if the current data block uses up the last of the data
		if(	zero_block_size >= page_size ||
			data_offset + zero_block_size == data_size)
		{
//printf("pack:  %d : %d    %X : %X\n", data_block_size, zero_block_size, data_block_size, zero_block_size);
			// force zero_block to be a max of page_size bytes
//			zero_block_size = page_size;
			
			s32 unpack_offset = unpackInfo.size();
			unpackInfo.set32(data_block_size, unpack_offset + 0);
			if(data_offset + zero_block_size != data_size)
				unpackInfo.set32(zero_block_size, unpack_offset + 4);
			else
				unpackInfo.set32(0, unpack_offset + 4);
			
			// zero_block was found, so first write out data_block, then zero_block
			basefileOut.set(data+data_offset-data_block_size, output_offset, data_block_size);
			output_offset += data_block_size;
			data_block_size = 0;
			
			data_offset += zero_block_size;
			zero_block_size = 0;
		}
		else
		{
			// zero block isnt big enough, so keep adding to data_block
			s32 increment_block_size = data_offset % page_size;
			if(increment_block_size == 0)
				increment_block_size = page_size;
			if(increment_block_size > data_size - data_offset)
				increment_block_size = data_size - data_offset;
			data_block_size += increment_block_size;
			data_offset += increment_block_size;
		}
	}

	// if zero block data is left over - it should be automatically added to the end of the image
	if(data_block_size || zero_block_size)
	{
//		printf("last pack:  %d : %d    %X : %X\n", data_block_size, zero_block_size, data_block_size, zero_block_size);
//		printf("data_block_size2 = %X\n", data_block_size);
//		printf("zero_block_size2 = %X\n", zero_block_size);
		s32 unpack_offset = unpackInfo.size();
		unpackInfo.set32(data_block_size, unpack_offset + 0);
		unpackInfo.set32(0, unpack_offset + 4);
		// add last data block
		basefileOut.set(data+data_offset-data_block_size, output_offset, data_block_size);
	}
	
	delete[] data;
	
	// now encrypt
	if( isEncrypted )
	{
		if( !encrypt(basefileOut, encKey) )
			return false;
	}
	
	unpackInfo.set32( unpackInfo.size(), 0 );
	return true;
}


int XexPacker::s_compressionCallback(void* param, unsigned char* compData, long compDataSize, long uncompDataSize)
{
	XexPacker* packer = (XexPacker*)param;
	return packer->compressionCallback(param, compData, compDataSize, uncompDataSize);
}

int XexPacker::compressionCallback(void* param, unsigned char* compData, long compDataSize, long uncompDataSize)
{
//	printf("Writing compressed data: was=%04X is=%04X\n", uncompDataSize, compDataSize);
	
	u16 compSize = (u16)compDataSize;
	compSize = GET16BE(&compSize);
	
	s32 offset = m_callbackData->size();
	m_callbackData->set16(compSize, offset);
	m_callbackData->set(compData, offset+2, compDataSize);
	
//	XexHash hash;
//	XeCryptSha(compData, compDataSize, 0,0, 0,0, hash, sizeof(XexHash));
//	m_callbackData->set32(compSize, offset+0);
//	m_callbackData->set(hash, offset+4, sizeof(XexHash));
//	m_callbackData->set16(compSize, offset+4+sizeof(XexHash));
//	m_callbackData->set(compData, offset+4+sizeof(XexHash)+2, compDataSize);
	
	return 0;
}

bool XexPacker::packCompressed(DataBlock& basefileOut, const DataBlock& basefileIn,
				bool isEncrypted, const XexKey& encKey,
				DataBlock& unpackInfo, s32 imageSize)
{
	s32 window_size = 0x8000;
	s32 uncomp_size = 0x8000;

	// Compress the whole image, then reframe. libLZX prefixes each block with
	// { uint16 compressed; uint16 uncompressed; } little-endian, while a XEX
	// carries a single big-endian compressed length per block, which is what
	// the hashed-block splitting below expects.
	std::vector<u8> source((size_t)imageSize);
	basefileIn.get(source.data(), 0, imageSize);

	std::vector<u8> encoded((size_t)imageSize + (imageSize >> 2) + 0x10000);
	ENCODER_CONTEXT* enc = lzx_create_compression_window(encoded.data(), (u32)window_size);
	if( enc == NULL )
		return false;

	u32 remaining = (u32)imageSize;
	const u8* srcPtr = source.data();
	lzx_flush_compression(enc);
	while( (u32)(srcPtr - source.data()) < (u32)imageSize )
	{
		u32 take = ((u32)uncomp_size < remaining) ? (u32)uncomp_size : remaining;
		if( lzx_compress_next_block(enc, &srcPtr, take, &remaining) != 0 )
		{
			lzx_destroy_compression(enc);
			return false;
		}
	}
	lzx_flush_compression(enc);
	u32 encodedSize = enc->output_buffer_size;
	lzx_destroy_compression(enc);

	DataBlock comp_data;
	{
		u32 pos = 0;
		s32 out_offset = 0;
		while( pos + 4 <= encodedSize )
		{
			u32 cs = encoded[pos] | (encoded[pos+1] << 8);
			pos += 4;                       // skip both little-endian sizes
			if( cs == 0 || pos + cs > encodedSize )
				break;
			u16 beSize = (u16)cs;
			beSize = GET16BE(&beSize);
			comp_data.set16(beSize, out_offset);
			comp_data.set(&encoded[pos], out_offset + 2, cs);
			out_offset += 2 + (s32)cs;
			pos += cs;
		}
	}
	
	// now split compressed data into hashed blocks
	// (the hash will be calculated later)

	basefileOut.clear();
	s32 comp_offset = 0;
	s32 max_hashed_block_size = 0x10000;
	s32 curr_hashed_block_size = 0;
	s32 curr_hashed_block_offset = 0;
	s32 hashed_block_header_size = 4 + sizeof(XexHash);
	std::vector<s32> hash_block_sizes;
	while( comp_offset < comp_data.size() )
	{
		u16 block_size = comp_data.get16(comp_offset);
		block_size = GET16BE(&block_size) + 2;

		// '2' needs to be added so that there is a 16bit comp_size value of 0 at the end of the buffer
		if( curr_hashed_block_size + block_size + hashed_block_header_size + 2 >= max_hashed_block_size )
		{
			// add current hash block
			s32 output_offset = basefileOut.size();
			s32 aligned_hashed_block_size = (curr_hashed_block_size + hashed_block_header_size + 2 + (0x800-1)) & (-0x800);
			hash_block_sizes.push_back(aligned_hashed_block_size);
//printf("%4X\n", aligned_hashed_block_size);
//if(aligned_hashed_block_size == 0x10000)
//{
//	printf("b00p\n");
//}
			
			// zero current hash block header and insert current hash block data
			basefileOut.fill( 0, output_offset, hashed_block_header_size );
			basefileOut.set(comp_data, curr_hashed_block_offset, output_offset + hashed_block_header_size, curr_hashed_block_size);
			basefileOut.fill( 0, output_offset + hashed_block_header_size + curr_hashed_block_size, aligned_hashed_block_size - (curr_hashed_block_size + hashed_block_header_size));
			
			curr_hashed_block_offset += curr_hashed_block_size;
			curr_hashed_block_size = block_size;
		}
		else
		{
			// keep building hash block
			curr_hashed_block_size += block_size;
		}
//printf("    %4X - %4X\n", block_size-2, comp_data.get16be(comp_offset+2));
		
		// increment offset and check if last block
		comp_offset += block_size;
		if( comp_offset >= comp_data.size() )
		{
			// add current hash block
			s32 output_offset = basefileOut.size();
			s32 aligned_hashed_block_size = (curr_hashed_block_size + hashed_block_header_size+ 0x800-1) & (-0x800);
//printf("%04X - abs last\n", aligned_hashed_block_size);
			hash_block_sizes.push_back(aligned_hashed_block_size);
			
			// zero current hash block header and insert current hash block data
			basefileOut.fill( 0, output_offset, hashed_block_header_size );
			basefileOut.set(comp_data, curr_hashed_block_offset, output_offset + hashed_block_header_size, curr_hashed_block_size);
			basefileOut.fill( 0, output_offset + hashed_block_header_size + curr_hashed_block_size, aligned_hashed_block_size - (curr_hashed_block_size + hashed_block_header_size));
			
			curr_hashed_block_offset += curr_hashed_block_size;
			curr_hashed_block_size = block_size;
		}

		/*
		if( curr_hashed_block_size + block_size + hashed_block_header_size > max_hashed_block_size ||
			comp_offset + block_size >= comp_data.size() )
		{
			// this can cause "curr_hashed_block_size" to be > "max_hashed_block_size"!
			if(comp_offset + block_size >= comp_data.size())
				curr_hashed_block_size += block_size;
			s32 output_offset = basefileOut.size();
			s32 aligned_hashed_block_size = (curr_hashed_block_size + hashed_block_header_size+ 0x800-1) & (-0x800);
if(aligned_hashed_block_size == 0x10000)
{
	printf("b00p\n");
}
			hash_block_sizes.push_back(aligned_hashed_block_size);
			
			// zero current hash block header and insert current hash block data
			basefileOut.fill( 0, output_offset, hashed_block_header_size );
			basefileOut.set(comp_data, curr_hashed_block_offset, output_offset + hashed_block_header_size, curr_hashed_block_size);
			basefileOut.fill( 0, output_offset + hashed_block_header_size + curr_hashed_block_size, aligned_hashed_block_size - (curr_hashed_block_size + hashed_block_header_size));
			
			curr_hashed_block_offset += curr_hashed_block_size;
			curr_hashed_block_size = block_size;
		}
		else
		{
			curr_hashed_block_size += block_size;
		}
		comp_offset += block_size;
*/	}
	
	// all conversion into hashed blocks has been done
	// so insert hash block sizes and hashes
	s32 block_offset = basefileOut.size();
	for(s32 block_num=(s32)hash_block_sizes.size()-1; block_num>=0; block_num--)
	{
		s32 block_size = hash_block_sizes[block_num];
		block_offset -= block_size;
		
		// fill in last hash block header
		u8* temp_block_data = new u8[block_size];
		basefileOut.get(temp_block_data, block_offset, block_size);
		XexHash hash;
		XeCryptSha(temp_block_data, block_size, 0,0, 0,0, hash.data, sizeof(XexHash));
		delete[] temp_block_data;
		if(block_num != 0)
		{
			s32 out_offset = block_offset - hash_block_sizes[block_num-1];
			basefileOut.set32( GET32BE(&block_size), out_offset );
			basefileOut.set( hash.data, out_offset+4, sizeof(XexHash) );
		}
		else
		{
			// save hash and size in unpack_data
			unpackInfo.clear();
			s32 size = 0x24;
			unpackInfo.set32( size, 0 );
			u16 type = (isEncrypted) ? 1 : 0;
			unpackInfo.set16( type, 4 );
			type = 2;
			unpackInfo.set16( type, 6 );
			size = 0x8000;
			unpackInfo.set32( size, 8 );
			unpackInfo.set32( block_size, 12 );
			unpackInfo.set( hash.data, 16, sizeof(XexHash) );
		}
	}
	
	// now encrypt
	if( isEncrypted )
	{
		if( !encrypt(basefileOut, encKey) )
			return false;
	}
	return true;
}

/*
bool XexPacker::packDeltaCompressed(DataBlock& basefileOut, const DataBlock& basefileIn,
				bool isEncrypted, const XexKey& encKey,
				DataBlock& unpackInfo, s32 imageSize)
{
	basefileOut = basefileIn;
	
	// now encrypt
	if( isEncrypted )
	{
		if( !encrypt(basefileOut, encKey) )
			return false;
	}
	return true;
	return false;
}
*/



/*
bool XexPacker::unpackDeltaBasefile(DataBlock& headersOut, const DataBlock& headersIn, const DataBlock& patchInfo)
{
	return true;
}
*/

