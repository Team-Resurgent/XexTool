//
// LZX decompression for XEX basefiles and patches, over libLZX.
//
#pragma once

#include <cstdint>

struct XexLzxDecoder;

// A XEX basefile is one continuous LZX stream cut into length-prefixed chunks,
// so the decoder is created once and fed each chunk in turn; the window carries
// across them.
XexLzxDecoder* XexLzxCreate(uint32_t windowSize);
void XexLzxDestroy(XexLzxDecoder* decoder);

// Seed the window with the data a delta was compressed against. Call before
// decoding, on a decoder used for a single block.
bool XexLzxSeedWindow(XexLzxDecoder* decoder, const uint8_t* data, uint32_t size);

// Decode one chunk. outputSize is what the chunk expands to.
bool XexLzxDecodeChunk(XexLzxDecoder* decoder,
                       const uint8_t* input, uint32_t inputSize,
                       uint8_t* output, uint32_t outputSize);
