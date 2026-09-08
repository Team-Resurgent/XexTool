#include "XexLzx.h"

#include <cstdlib>

#include "lzx/lzx.h"

struct XexLzxDecoder
{
	LZX_DECODER_CONTEXT* context;
};

XexLzxDecoder* XexLzxCreate(uint32_t windowSize)
{
	LZX_DECODER_CONTEXT* context = lzx_create_decompression_window(windowSize);
	if (context == NULL)
		return NULL;

	XexLzxDecoder* decoder = new XexLzxDecoder;
	decoder->context = context;
	return decoder;
}

void XexLzxDestroy(XexLzxDecoder* decoder)
{
	if (decoder == NULL)
		return;
	lzx_destroy_decompression(decoder->context);
	delete decoder;
}

bool XexLzxSeedWindow(XexLzxDecoder* decoder, const uint8_t* data, uint32_t size)
{
	if (decoder == NULL)
		return false;
	return lzx_set_window_data(decoder->context, data, size) == 0;
}

bool XexLzxDecodeChunk(XexLzxDecoder* decoder,
                       const uint8_t* input, uint32_t inputSize,
                       uint8_t* output, uint32_t outputSize)
{
	if (decoder == NULL)
		return false;

	// bytes_decompressed is in/out: it states the expected size going in
	uint32_t produced = outputSize;
	if (lzx_decompress_block(decoder->context, input, inputSize,
	                         output, &produced) != 0)
		return false;
	return produced == outputSize;
}
