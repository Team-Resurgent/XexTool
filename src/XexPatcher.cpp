// 
// Xex Patcher
// 

#include "XexPatcher.h"
#include "Xex.h"
#include "XeCrypt.h"
#include "XexData.h"
#include "Endian.h"
#include "XexHeader.h"
#include <assert.h>
#include "Ldic.h"

typedef struct {
	u32 deltaSrc;	// offset to start unpacking data from
	u32 deltaDest;	// offset to start unpacking data to
	u16 decompSize;	// size this block will unpack to
	u16 compSize;	// size of compressed data in this block
} DeltaBlock;


XexPatcher::XexPatcher()
{
}

XexPatcher::~XexPatcher()
{
}


// checks if the patch xex is the correct one for the source xex
bool XexPatcher::isCorrectPatch(const Xex& sourceXex, const Xex& patchXex) const
{
	// check the the patch descriptor matches the signature hash
	DataBlock patch_desc, orig_headers;
	sourceXex.getOriginalHeaders(orig_headers);
	s32 sec_offset = orig_headers.get32be( offsetof(XexImageHeader, securityInfoOffset) );
	s32 sig_offset = sec_offset + offsetof(XexSecurityInfo, imageInfo) + offsetof(XexHvImageInfo, signature);
	u8 source_sig[0x100];
	orig_headers.get(source_sig, sig_offset, 0x100);
	if( !patchXex.getDeltaPatchDescriptor(patch_desc) )
		return false;
	u8 patch_hash[20];
	patch_desc.get(patch_hash, offsetof(DeltaPatchDescriptor, sourceHash), 20);
	u8 source_hash[20];
	XeShaContext sha_ctx;
	XeCryptShaInit(&sha_ctx);
	XeCryptShaUpdate(&sha_ctx, source_sig, 0x100);
	XeCryptShaFinal(&sha_ctx, source_hash, 20);
	if( memcmp(source_hash, patch_hash, 20) != 0 )
		return false;
	
	return true;
}


// create an xex from the source xex and patch xex
// the output xex must not be the same as the input xex or patch
bool XexPatcher::patch(Xex& outputXex, const Xex& sourceXex, const Xex& patchXex)
{
//	FILE* fd;
	
	m_pTargetXex = &outputXex;
	
	if(	&sourceXex == &patchXex ||
		&sourceXex == m_pTargetXex ||
		!patchXex.isPatchModule() ||
		sourceXex.isPatchModule() )
		return false;
	
	if( !isCorrectPatch(sourceXex, patchXex) )
		return false;
	
	// create output headers
	DataBlock xex_headers_target, xex_headers_source, patch_info;
	sourceXex.getOriginalHeaders(xex_headers_source);
	patchXex.getDeltaPatchDescriptor(patch_info);
	if( !unpackDeltaHeaders(xex_headers_target, xex_headers_source, patch_info) )
	{
		// error getting headers
		return false;
	}
//fd = fopen("hdrs.bin", "wb");
//xex_headers_target.write(fd, 0, xex_headers_target.size());
//fclose(fd);

	XexHeader xex_headers;
	s32 target_image_size = 0;
	DataBlock basefile_format;
	if( !xex_headers.readHeaders(*m_pTargetXex, xex_headers_target, target_image_size, basefile_format) )
		return false;
	
	// get keys to decrypt with
	XexKey xex_target_key, xex_source_key, patch_info_key, xex_patch_key;
	m_pTargetXex->getImageKey(xex_target_key);
	patch_info.get(patch_info_key.data, 0x20, sizeof(XexKey));
	sourceXex.getImageKey(xex_source_key);
	patchXex.getImageKey(xex_patch_key);
	
//	s32 target_security_offset = xex_headers_target.get32be(0x10);
//	s32 target_image_size = xex_headers_target.get32be(target_security_offset + 4);
//	xex_headers_target.get(xex_target_key.data, target_security_offset +
//		offsetof(XexSecurityInfo, imageInfo) + offsetof(XexHvImageInfo, imageKey),
//		sizeof(XexKey));
//	patch_info.get(patch_info_key.data, 0x20, sizeof(XexKey));
//	sourceXex.getImageKey(xex_source_key);
	
	// get key from patch xex file
	// get and decrypt the target xex image key
//	patchXex.getImageKey(xex_patch_key);
/*	XeAesContext aes_ctx;
	if( patchXex.isManufacturingUtility() ||
		patchXex.isManufacturingSupportTool() )
	{
		if(patchXex.isRetail())
			XeCryptAesKey(&aes_ctx, XexData::XEX_MFG_RETAIL_KEY);
		else
			XeCryptAesKey(&aes_ctx, XexData::XEX_MFG_DEBUG_KEY);
	}
	else
	{
		if(patchXex.isRetail())
			XeCryptAesKey(&aes_ctx, XexData::XEX_RETAIL_KEY);
		else
			XeCryptAesKey(&aes_ctx, XexData::XEX_DEBUG_KEY);
	}
	XeCryptAesEcb(&aes_ctx, xex_target_key.data, xex_target_key.data, XE_CRYPT_DEC);
*/	
	// decrypt xex patch key using decrypted target key
	XeAesContext aes_ctx;
	XeCryptAesKey(&aes_ctx, xex_target_key.data);
	XeCryptAesEcb(&aes_ctx, xex_patch_key.data, xex_patch_key.data, XE_CRYPT_DEC);
	
	// get basefile data to create target basefile
	DataBlock basefile_source, basefile_patch, basefile_target;
	sourceXex.getBasefile(basefile_source);
	patchXex.getPatchData(basefile_patch);
	// get basefile info from patch xex
	DataBlock basefile_patch_info;
	patchXex.getPatchInfo(basefile_patch_info);
/*
FILE* fd;
fd = fopen("basefile_source.bin", "wb");
basefile_source.write(fd, 0, basefile_source.size());
fclose(fd);

fd = fopen("basefile_patch-enc.bin", "wb");
basefile_patch.write(fd, 0, basefile_patch.size());
fclose(fd);

fd = fopen("base_patch_info.bin", "wb");
basefile_patch_info.write(fd, 0, basefile_patch_info.size());
fclose(fd);
*/
	// create target basefile
	if( !unpackDeltaBasefile(basefile_target, target_image_size, basefile_source, basefile_patch, basefile_patch_info, xex_patch_key.data) )
	{
		// error creating basefile
		return false;
	}
	m_pTargetXex->setBasefile(basefile_target);

/*
	// success with patching
	// now write to a temp file and read it in with the xex reader
	// so that we can return it in an object
	const char temp_filename[] = "temp.bin";
	FILE* fd = fopen(temp_filename, "w+b");
	if(fd == NULL)
	{
		// error creating file
		return false;
	}

	// dump headers
	u8* data = new u8[xex_headers_target.size()];
	xex_headers_target.get(data, 0, xex_headers_target.size());
	fwrite(data, 1, xex_headers_target.size(), fd);
	delete[] data;
	// dump basefile
	data = new u8[basefile_target.size()];
	basefile_target.get(data, 0, basefile_target.size());
	fwrite(data, 1, basefile_target.size(), fd);
	delete[] data;
	fclose(fd);
	
	// now load in the xex from the file
	XexReader reader(m_pTargetXex);
	if( !reader.read(temp_filename) )
	{
		// error reading in created xex file
		remove(temp_filename);
		return false;
	}
	
	// success - so delete the temp xex file
	remove(temp_filename);
*/
	return true;
}


// unpack all delta-blocks within the given data buffers
// 
// args:	ldic context
//			output for decompressed data
//			size of output buffer
//			input data to decompress
//			size of input data to decompress
// returns:	true if successful
bool XexPatcher::XexpDeltaDecompress(const LdicContext& ctx, DataBlock& output,
									 const u8* inputBuff, s32 inputSize)
{
	s32 outputSize = output.size();
	while(inputSize != 0)
	{
		// get delta block and check is values
		if(inputSize < sizeof(DeltaBlock))
			return false;
		DeltaBlock* delta_block = (DeltaBlock*)inputBuff;
		inputBuff += sizeof(DeltaBlock);
		inputSize -= sizeof(DeltaBlock);
		s32 delta_src = GET32BE( &delta_block->deltaSrc );
		s32 delta_dest = GET32BE( &delta_block->deltaDest );
		s32 decomp_size = GET16BE( &delta_block->decompSize );
		s32 comp_size = GET16BE( &delta_block->compSize );
		// success
		if(decomp_size == 0)
			break;
		if(	delta_src > outputSize || 
			decomp_size > outputSize-delta_src ||
			delta_dest >= outputSize ||
			decomp_size > outputSize-delta_dest )
			return false;
		
		// depending on comp_size either set zeros, move memory, or decompress
		if(comp_size == 0)
		{
			// set zeros
			output.fill(0, delta_dest, decomp_size);
//printf("memset(0x%06x, 0, 0x%x);\n", delta_dest, decomp_size);
		}
		else if(comp_size == 1)
		{
			// move memory
			if(delta_src != delta_dest)
			{
				u8* data = new u8[decomp_size];
				output.get(data, delta_src, decomp_size);
				output.set(data, delta_dest, decomp_size);
				delete[] data;
//				memmove(outputBuff+delta_dest, outputBuff+delta_src, decomp_size);
//printf("memcpy(0x%06x, 0x%06x, 0x%x);\n", delta_dest, delta_src, decomp_size);
			}
			else
			{
//printf("WTF happened!\n");
			}
		}
		else
		{
//printf("decomp(0x%06x, 0x%06x, 0x%x, 0x%x);\n", delta_dest, delta_src, decomp_size, comp_size);
			u8* data = new u8[decomp_size];
			output.get(data, delta_src, decomp_size);
			
			// decompress
			if(	inputSize < comp_size ||
				!LdicSetWindowData(ctx, data, decomp_size) ||
				!LdicDecompress(ctx, inputBuff, comp_size, data, decomp_size) ||
				!LdicResetDecompression(ctx) )
			{
				delete[] data;
				return false;
			}
			output.set(data, delta_dest, decomp_size);
			delete[] data;
			
			inputBuff += comp_size;
			inputSize -= comp_size;
		}
	}
	
	return true;
}


bool XexPatcher::unpackDeltaHeaders(DataBlock& headersTarget, const DataBlock& headersSource, const DataBlock& patchInfo)
{
	LdicContext ldic_ctx;
	s32 window_size = 0x8000;
	s32 source_size = 0x8000;
	s32 max_uncomp_size = 0;
	if( !LdicCreateDecompression(ldic_ctx, source_size, window_size, max_uncomp_size) )
		return false;
	
	// create output headers buffer
	DeltaPatchDescriptor* patch_desc = (DeltaPatchDescriptor*)new u8[patchInfo.size()];
	patchInfo.get(patch_desc, 0, patchInfo.size());
	s32 dest_size = (patch_desc->targetHeaderSize + 0xFFF) & 0xFFFFF000;
	u8* dest_hdrs = new u8[dest_size];
	memset(dest_hdrs, 0, dest_size);
	headersSource.get(dest_hdrs+patch_desc->deltaHeaderTargetOffset,
		patch_desc->deltaHeaderSourceOffset,
		patch_desc->deltaHeaderSourceSize);
	headersTarget.clear();
	headersTarget.set(dest_hdrs, 0, dest_size);
	delete[] dest_hdrs;
	
	// unpack all delta-blocks within the given data buffers
//printf("\n\npatching headers\n\n");
	bool result = XexpDeltaDecompress(ldic_ctx, headersTarget,
		patch_desc->patchData, patch_desc->infoSize-offsetof(DeltaPatchDescriptor, patchData));
	
	LdicDestroyDecompression(ldic_ctx);
	delete[] patch_desc;
	return result;
}





// FIX FIX FIX FIX FIX FIX FIX FIX FIX FIX FIX FIX FIX 
// 
// this needs to be fixed to support a target file smaller than the source file!
// 
// FIX FIX FIX FIX FIX FIX FIX FIX FIX FIX FIX FIX FIX 

bool XexPatcher::unpackDeltaBasefile(DataBlock& basefileTarget, s32 targetImageSize,
				const DataBlock& basefileSource, const DataBlock& basefilePatch,
				const DataBlock& basefilePatchInfo, const u8* decKey)
{
	// decrypt patch basefile data
	const DataBlock* basefile_patch_ptr = &basefilePatch;
	DataBlock basefile_patch_decrypted;
	if( basefilePatchInfo.get16(4) == 1 )
	{
		u8 ivec[16] = {0};
		u8* data = new u8[basefilePatch.size()];
		basefilePatch.get(data, 0, basefilePatch.size());
		XeAesContext aes_ctx;
		XeCryptAesKey(&aes_ctx, decKey);
		XeCryptAesCbc(&aes_ctx, data, basefilePatch.size(), data, ivec, XE_CRYPT_DEC);
		basefile_patch_decrypted.set(data, 0, basefilePatch.size());
//FILE* fd;
//fd = fopen("basefile_patch-dec.bin", "wb");
//basefile_patch_decrypted.write(fd, 0, basefile_patch_decrypted.size());
//fclose(fd);
		delete[] data;
		basefile_patch_ptr = &basefile_patch_decrypted;
	}
	
//FILE* fd;
//fd = fopen("base_patch.bin", "wb");
//basefile_patch_decrypted.write(fd, 0, basefile_patch_decrypted.size());
//fclose(fd);
//int cnt=1;

	bool success = true;
	LdicContext ctx = NULL;
	u8* block_data = NULL;
//	u8* uncomp_data = NULL;
	
	basefileTarget.clear();
//	basefileTarget = basefileSource;
//	if(targetImageSize > basefileSource.size())
//		basefileTarget.fill(0, basefileSource.size(), targetImageSize-basefileSource.size());
	int offset = basefileSource.size() - targetImageSize;
	if(offset < 0)
		offset = 0;
	basefileTarget.set(basefileSource, offset, 0, basefileSource.size()-offset);
	if(targetImageSize > basefileTarget.size())
		basefileTarget.fill(0, basefileTarget.size(), targetImageSize-basefileTarget.size());
	
	s32 window_size = basefilePatchInfo.get32(8);
	s32 source_size = window_size;
	CompBaseFileBlock unpack_info;
	unpack_info.dataSize = basefilePatchInfo.get32(12);
	basefilePatchInfo.get(&unpack_info.hash, 16, sizeof(XexHash));
	
	s32 max_uncomp_size = 0;
	if( !LdicCreateDecompression(ctx, source_size, window_size, max_uncomp_size) )
	{
		success = false;
		goto finish_up;
	}
	
//printf("\n\n\n\npatching basefile\n\n");
	// unpacks blocks at a time, each block has multiple smaller blocks to decompress too
//s32 patch_cnt = 0;
	s32 input_offset = 0;
//	s32 output_offset = 0;
	while( unpack_info.dataSize )
	{
		// get patch basefile data to decompress
		s32 block_size = unpack_info.dataSize;
		block_data = new u8[block_size];
		basefile_patch_ptr->get(block_data, input_offset, block_size);
		input_offset += block_size;
		
/*		// new preload the destination buffer with source basefile data
		uncomp_data = new u8[max_uncomp_size];
		s32 preload_size = max_uncomp_size;
		if(preload_size > basefileSource.size()-output_offset)
			preload_size = basefileSource.size()-output_offset;
		basefileSource.get(uncomp_data, output_offset, preload_size);
		if(max_uncomp_size > preload_size)
			memset(uncomp_data, 0, max_uncomp_size-preload_size);
*/		
//printf("%2d) %8X / %8X  (%8X)\n", patch_cnt, input_offset-block_size, basefile_patch_ptr->size(), block_size);
//patch_cnt++;
		// generate hash over data to ensure its valid before trying to decompress it
		XexHash hash;
		XeCryptSha(block_data, block_size, 0,0, 0,0, hash.data, sizeof(XexHash));
		if(memcmp(hash.data, unpack_info.hash.data, sizeof(XexHash)) != 0)
		{
			printf("Error patch file is corrupted for block 0x%X - 0x%X (0x%X)!\n", input_offset-block_size, input_offset, block_size);
			success = false;
			goto finish_up;
		}
		
		// unpack all "delta blocks" inside this "comp block"
		u8* comp_ptr  = block_data + sizeof(CompBaseFileBlock);
		s32 comp_size = block_size - sizeof(CompBaseFileBlock);
//		u8* uncomp_ptr = uncomp_data;
//		s32 uncomp_size = max_uncomp_size;
//printf("\n\npatching basefile %d\n\n", cnt);
		if( !XexpDeltaDecompress(ctx, basefileTarget, comp_ptr, comp_size) )
		{
			printf("Error doing delta decompress.\n");
			success = false;
			goto finish_up;
		}
		
//char tmp_filename[256];
//sprintf(tmp_filename, "patched_output%d.bin", cnt);
//fd = fopen(tmp_filename, "wb");
//basefileTarget.write(fd, 0, basefileTarget.size());
//fclose(fd);
//cnt++;

/*		// copy decompressed data to output
		basefileTarget.set(uncomp_ptr, output_offset, uncomp_size);
		output_offset += uncomp_size;
		comp_ptr += comp_size;
		delete[] uncomp_data;
		uncomp_data = NULL;
*/		
		// get unpack_info for next block from header of this block
		unpack_info.dataSize = GET32BE(block_data + 0);
		memcpy(unpack_info.hash.data, block_data + 4, sizeof(XexHash));
		delete[] block_data;
		block_data = NULL;
	}
	
	// if the target is bigger than the source then the result is bigger.
	// this needs to be fixed when i work out how to handle such things.
	if(basefileTarget.size() > targetImageSize)
	{
		basefileTarget.truncate(targetImageSize);
		success = false;
		printf("Patching error due to target file being smaller than the source file\n");
	}
	
finish_up:
	//basefileTarget.truncate(targetImageSize);
	if(block_data) delete[] block_data;
//	if(uncomp_data)delete[] uncomp_data;
	if(ctx) LdicDestroyDecompression(ctx);
	return success;
}
