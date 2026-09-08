// 
// ldic compression/decompression library
// 

#ifndef _LDIC_H_
#define _LDIC_H_
#if defined(_LANGUAGE_C_PLUS_PLUS)||defined(__cplusplus)||defined(c_plusplus)
extern "C" {
#endif

// ldic blocks used in compressed data
typedef void*				LdicContext;

// ldic blocks used in compressed data
typedef struct LdicBlock {
    unsigned short compressedSize;
    unsigned short uncompressedSize;
} LdicBlock;

typedef int (*LdicCompressionCallback)(void *param, unsigned char* compData, long compDataSize, long uncompDataSize);


// 
// COMPRESSION ROUTINES
// 

// Create and reset an LCI compression context
// 
// args:	max uncompressed data block size expected
//			gets required compressed buffer size
//			gets ldic context if it's successfully created
// returns:	true if successful
bool LdicCreateCompression(LdicContext& ctx, int& maxUncompDataSize, int windowSize, int& minCompDataSize, LdicCompressionCallback funcPtr, void* funcData);

// Compress a block of data
bool LdicCompress(const LdicContext& ctx, const void* src, int srcSize, void* dest, int& destSize);

// Reset compression context
bool LdicResetCompression(const LdicContext& ctx);

// Destroy LCI compression context
bool LdicDestroyCompression(const LdicContext& ctx);

// flushes and leftover ldic data from compressor
bool LdicFlushCompressorOutput(const LdicContext& ctx);

// Set file translation size
bool LdicSetTranslationSize(const LdicContext& ctx, int size);



// 
// DECOMPRESSION ROUTINES
// 

// create decompression context
// maxDataBlockSize gets filled with max uncompressed data block size allowed
// 
// args:	max uncompressed data block size expected
//			window size for decompression
//			gets max compressed buffer size
//			gets ldic context if it's successfully created
// returns:	true if completed successfully
bool LdicCreateDecompression(LdicContext& ctx, int& maxUncompDataSize, int windowSize, int& maxCompDataSize);

// decompresses a block of data
// destSize gets updated with the size of the data decompressed
// 
// args:	decompression context
//			source buffer (compressed data)
//			size of source data to decompress
//			destination buffer (for decompressed data)
//			size of destination buffer for decompressed data
// returns:	true if successful
bool LdicDecompress(const LdicContext& ctx, const void* compData, int compDataSize, void* decompData, int& decompDataSize);

// Reset LDI decompression context
bool LdicResetDecompression(const LdicContext& ctx);

// Destroy LDI Decompression context
bool LdicDestroyDecompression(const LdicContext& ctx);

// set window data
bool LdicSetWindowData(const LdicContext& ctx, const void* data, int dataSize);



// use this to get info on the error that occurred
// it will be one of the following LDIC_ERROR_??? flags
int  LdicGetLastError();

// error values
#define LDIC_ERROR_NO_ERROR				0
#define LDIC_ERROR_NOT_ENOUGH_MEMORY	1
#define LDIC_ERROR_BAD_PARAMETERS		2
#define LDIC_ERROR_BUFFER_OVERFLOW		3
#define LDIC_ERROR_FAILED				4
#define LDIC_ERROR_CONFIGURATION		5


#if defined(_LANGUAGE_C_PLUS_PLUS)||defined(__cplusplus)||defined(c_plusplus)
}
#endif
#endif //_LDIC_H_

