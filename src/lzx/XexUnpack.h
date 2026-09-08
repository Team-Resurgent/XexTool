#pragma once

#include <cstdint>

#ifdef _WIN32
#define TR_EXPORT extern "C" __declspec(dllexport)
#else
#define TR_EXPORT extern "C" __attribute__((visibility("default")))
#endif

//
// Decompress a raw LZX stream held entirely in memory.
//
// XEX basefiles are one continuous LZX stream, so the whole stream is passed in
// a single call; decompressing piecewise would reset the window between calls.
//
// error is set non-zero on failure.
//
TR_EXPORT void LZXUnpack(uint8_t* inputData, uint32_t inputDataSize,
                         uint8_t* outputData, uint32_t outputDataSize,
                         uint32_t windowSize, uint32_t& error);

// Decompress one LZX block over a window pre-seeded with reference data.
// skipHeader suppresses the per-stream "intel filesize" header read, for
// producers that emit it once rather than per block.
TR_EXPORT void LZXUnpackDelta(uint8_t* inputData, uint32_t inputDataSize,
                              uint8_t* outputData, uint32_t outputDataSize,
                              uint32_t windowSize,
                              uint8_t* referenceData, uint32_t referenceDataSize,
                              int skipHeader, uint32_t& error);
