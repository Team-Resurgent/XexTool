// 
// ldic compression/decompression library
// 

#include "ldic.h"
#include "decoder/ldi.h"
#include "encoder/lci.h"

#ifdef _XBOX
#include <xtl.h>
#else
#include <windows.h>
#endif

// error values
#define     LDIC_ERROR_NO_ERROR				0
#define     LDIC_ERROR_NOT_ENOUGH_MEMORY	1
#define     LDIC_ERROR_BAD_PARAMETERS		2
#define     LDIC_ERROR_BUFFER_OVERFLOW		3
#define     LDIC_ERROR_FAILED				4
#define     LDIC_ERROR_CONFIGURATION		5

int LdicLastError = 0;
int LdicGetLastError()
{
	return LdicLastError;
}


MI_MEMORY DIAMONDAPI LdicAlloc( ULONG cb )
{
    return LocalAlloc( LPTR, cb );
}

void DIAMONDAPI LdicFree( MI_MEMORY pv )
{
    LocalFree( pv );
}


// create decompression context
// maxDataBlockSize gets filled with max uncompressed data block size allowed
// 
// args:	max uncompressed data block size expected
//			window size for decompression
//			gets max compressed buffer size
//			gets ldic context if it's successfully created
// returns:	true if completed successfully
bool LdicCreateDecompression(LdicContext& ctx, int& maxUncompDataSize, int windowSize, int& maxCompDataSize)
{
	LZXDECOMPRESS config;
	config.WindowSize = windowSize;
	config.fCPUtype = LDI_CPU_80386;
	
	LdicLastError =  LDICreateDecompression((UINT*)&maxUncompDataSize, &config, LdicAlloc, LdicFree,
		(UINT*)&maxCompDataSize, &ctx, NULL, NULL, NULL, NULL, NULL);
	return LdicLastError == LDIC_ERROR_NO_ERROR;
}


// decompresses a block of data
// destSize gets updated with the size of the data decompressed
// 
// args:	decompression context
//			source buffer (compressed data)
//			size of source data to decompress
//			destination buffer (for decompressed data)
//			size of destination buffer for decompressed data
// returns:	true if successful
bool LdicDecompress(const LdicContext& ctx, const void* compData, int compDataSize, void* decompData, int& decompDataSize)
{
	LdicLastError = LDIDecompress(ctx, (void*)compData, compDataSize, decompData, (unsigned int*)&decompDataSize);
	return LdicLastError == LDIC_ERROR_NO_ERROR;
}

// resets ldic decompression context
bool LdicResetDecompression(const LdicContext& ctx)
{
	LdicLastError = LDIResetDecompression(ctx);
	return LdicLastError == LDIC_ERROR_NO_ERROR;
}

// destroys ldic decompression context
bool LdicDestroyDecompression(const LdicContext& ctx)
{
	LdicLastError = LDIDestroyDecompression(ctx);
	return LdicLastError == LDIC_ERROR_NO_ERROR;
}

// set window data
bool LdicSetWindowData(const LdicContext& ctx, const void* data, int dataSize)
{
	LdicLastError = LDISetWindowData(ctx, (BYTE*)data, dataSize);
	return LdicLastError == LDIC_ERROR_NO_ERROR;
}


/*
unsigned short swap16(unsigned short val)
{
	return ((val&0xFF)<< 8) | (val>>8);
}
unsigned char comp_info_buff[100 * 1024];
int LdicCompressedDataSet;
unsigned char* LdicCompressedDataPtr;

int LdicCompressionCallback(void* pfol, unsigned char* compData, long compDataSize, long uncompDataSize)
{
	LdicCompressedDataSet = 0;
//	int info_size = *(int*)pfol;
//	unsigned char* new_mem = (unsigned char*)LdicAlloc(compDataSize + 2);
//	*(long*)&comp_info_buff[info_size*8] = compDataSize + 2;
//	*(long*)&comp_info_buff[info_size*8 + 4] = (long)new_mem;
	
//	*(unsigned short*)new_mem = swap16((unsigned short)compDataSize);
//	memcpy(&new_mem[2], compData, compDataSize);

	*(unsigned short*)LdicCompressedDataPtr = swap16((unsigned short)compDataSize);
	memcpy(&LdicCompressedDataPtr[2], compData, compDataSize);
	
//	info_size++;
//	*(int*)pfol = info_size;
	return 0;
}
*/



// Create and reset an LCI compression context
// 
// args:	max uncompressed data block size expected
//			size of compression window
//			gets required compressed buffer size
//			gets ldic context if it's successfully created
// returns:	true if successful
bool LdicCreateCompression(LdicContext& ctx, int& maxUncompDataSize, int windowSize, int& minCompDataSize, LdicCompressionCallback callbackPtr, void* callbackParam)
{
	LZXCONFIGURATION config;
	config.WindowSize = windowSize;
	config.SecondPartitionSize = windowSize;
	
//	LdicCompressedDataSet = 0;
	
	LdicLastError =  LCICreateCompression((UINT*)&maxUncompDataSize, &config, LdicAlloc, LdicFree,
		(UINT*)&minCompDataSize, &ctx, callbackPtr, callbackParam);
//	LdicFlushCompressorOutput(ctx);
	return LdicLastError == LDIC_ERROR_NO_ERROR;
}

// Compress a block of data
bool LdicCompress(const LdicContext& ctx, const void* uncompData, int uncompDataSize, void* compData, int& compDataSize)
{
	int result_size = uncompDataSize;
/*	if( !LdicCompressedDataSet )
	{
		LdicCompressedDataPtr = (unsigned char*)compData;
		LdicCompressedDataSet = 1;
	}
*/	LdicLastError = LCICompress(ctx, (void*)uncompData, uncompDataSize, NULL, compDataSize, (unsigned long*)&result_size);
//	compDataSize = result_size;
	return LdicLastError == LDIC_ERROR_NO_ERROR;
}

// resets ldic compression context
bool LdicResetCompression(const LdicContext& ctx)
{
	LdicLastError = LCIResetCompression(ctx);
	return LdicLastError == LDIC_ERROR_NO_ERROR;
}

// destroys ldic compression context
bool LdicDestroyCompression(const LdicContext& ctx)
{
	LdicLastError = LCIDestroyCompression(ctx);
	return LdicLastError == LDIC_ERROR_NO_ERROR;
}

// flushes and leftover ldic data from compressor
bool LdicFlushCompressorOutput(const LdicContext& ctx)
{
	LdicLastError = LCIFlushCompressorOutput(ctx);
	return LdicLastError == LDIC_ERROR_NO_ERROR;
}

// sets the file translation size
bool LdicSetTranslationSize(const LdicContext& ctx, int size)
{
	LdicLastError = LCISetTranslationSize(ctx, size);
	return LdicLastError == LDIC_ERROR_NO_ERROR;
}


/*
bool LdicCompressionCallback(void * pfol, u8* compressedData, s32 compressedSize, s32 uncompressedSize)
{
    u32 bytes_written;
    LdicBlock block;
	
    //
    // Write block header
    //

    block.compressedSize = (u16)compressedSize;
    block.uncompressedSize = (u16)uncompressedSize;
	
    CompressedSize += sizeof(block) + compressedSize;
    UncompressedSize += uncompressedSize;
	
    if ( !WriteFile(OutputFileHandle, &Block, sizeof(Block), &BytesWritten, NULL) ) {
        return -1;
    }

    //
    // Write compressed data
    //

    if ( !WriteFile(OutputFileHandle, compressed_data, compressed_size, &BytesWritten, NULL) ) {
        return -1;
    }

    return 0;
}
*/
