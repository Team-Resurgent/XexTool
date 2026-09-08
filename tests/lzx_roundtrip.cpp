//
// Round trips data through the LZX encoder and decoder the way a xex basefile
// is packed and unpacked.
//
// The point of this test is that a build can compile cleanly and still code
// wrongly. char is unsigned on some targets, and the decoder's bit counter has
// to be able to go negative, so a plain char there decodes nothing on arm64
// while working perfectly on x64. Compression bugs are just as quiet -- a
// build-vs-build diff cannot see them because both sides agree -- so the only
// way to catch them is to decode what was compressed and compare. That kind of
// fault shows up only when real data fails to survive a round trip.
//
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "lzx/lzx.h"
#include "XexLzx.h"

namespace {

const uint32_t kWindowSize = 0x8000;
const uint32_t kChunkSize  = 0x8000;

// libLZX frames each compressed block as a little endian pair of
// { uint16 compressed size; uint16 uncompressed size; }
struct Chunk {
	std::vector<uint8_t> data;
	uint32_t             uncompressedSize;
};

// Literals only, so the trees stay shallow and mostly the literal path runs.
std::vector<uint8_t> makeRandom(size_t size)
{
	std::vector<uint8_t> data(size);
	uint32_t state = 0x12345678;
	for(size_t i=0; i<size; i++)
	{
		state ^= state << 13;
		state ^= state >> 17;
		state ^= state << 5;
		data[i] = (uint8_t)(state >> 24);
	}
	return data;
}

// Mixed literals and back references at varying distances, so matches are
// emitted and the match position slots and aligned offsets are exercised.
std::vector<uint8_t> makeMixed(size_t size)
{
	std::vector<uint8_t> data = makeRandom(size);
	for(size_t i=1024; i<size; i++)
	{
		size_t period = 251 + (i % 7) * 1021;
		if( i >= period && (i % 101) >= 30 )
			data[i] = data[i - period];
	}
	return data;
}

// One long run, which compresses to almost nothing and leans on the length
// tree and the largest match lengths.
std::vector<uint8_t> makeRuns(size_t size)
{
	std::vector<uint8_t> data(size, 0);
	for(size_t i=0; i<size; i+=4096)
		data[i] = (uint8_t)(i / 4096);
	return data;
}

// Mixed data sprinkled with 0xE8 bytes. LZX has an optional x86 CALL (0xE8)
// address translation that only makes sense for x86 code; a XEX basefile is
// PowerPC, where 0xE8 is ordinary data. If that translation is ever enabled
// again this is the case that catches it -- the bytes after each 0xE8 come
// back wrong.
std::vector<uint8_t> makeE8(size_t size)
{
	std::vector<uint8_t> data = makeMixed(size);
	for(size_t i=0; i+5<size; i += 37)
	{
		data[i] = 0xE8;
		data[i+1] = (uint8_t)(i);
		data[i+2] = (uint8_t)(i >> 8);
		data[i+3] = (uint8_t)(i >> 16);
		data[i+4] = (uint8_t)(i >> 24);
	}
	return data;
}

bool compress(const std::vector<uint8_t>& source, std::vector<Chunk>& chunksOut)
{
	std::vector<uint8_t> encoded(source.size() + (source.size() >> 2) + 0x10000);
	ENCODER_CONTEXT* enc = lzx_create_compression_window(encoded.data(), kWindowSize);
	if( enc == NULL )
		return false;

	// each call compresses exactly the bytes it is given into one block, so
	// the sizes recorded here are what the decoder must produce
	std::vector<uint32_t> blockSizes;
	uint32_t remaining = (uint32_t)source.size();
	const uint8_t* src = source.data();
	lzx_flush_compression(enc);
	while( (size_t)(src - source.data()) < source.size() )
	{
		uint32_t take = (kChunkSize < remaining) ? kChunkSize : remaining;
		blockSizes.push_back(take);
		if( lzx_compress_next_block(enc, &src, take, &remaining) != 0 )
		{
			lzx_destroy_compression(enc);
			return false;
		}
	}
	lzx_flush_compression(enc);
	uint32_t encodedSize = enc->output_buffer_size;
	lzx_destroy_compression(enc);

	uint32_t pos = 0;
	size_t   block = 0;
	while( pos + 4 <= encodedSize && block < blockSizes.size() )
	{
		uint32_t cs = (uint32_t)encoded[pos] | ((uint32_t)encoded[pos+1] << 8);
		pos += 4;
		if( cs == 0 || pos + cs > encodedSize )
			break;
		Chunk chunk;
		chunk.data.assign(encoded.begin() + pos, encoded.begin() + pos + cs);
		chunk.uncompressedSize = blockSizes[block];
		chunksOut.push_back(chunk);
		pos += cs;
		block++;
	}
	return chunksOut.size() == blockSizes.size();
}

bool decompress(const std::vector<Chunk>& chunks, std::vector<uint8_t>& out)
{
	XexLzxDecoder* dec = XexLzxCreate(kWindowSize);
	if( dec == NULL )
		return false;

	out.clear();
	for(size_t i=0; i<chunks.size(); i++)
	{
		std::vector<uint8_t> buffer(chunks[i].uncompressedSize);
		if( !XexLzxDecodeChunk(dec, chunks[i].data.data(), (uint32_t)chunks[i].data.size(),
							   buffer.data(), chunks[i].uncompressedSize) )
		{
			XexLzxDestroy(dec);
			return false;
		}
		out.insert(out.end(), buffer.begin(), buffer.end());
	}
	XexLzxDestroy(dec);
	return true;
}

bool runCase(const std::string& name, const std::vector<uint8_t>& source)
{
	std::vector<Chunk> chunks;
	if( !compress(source, chunks) )
	{
		printf("FAIL %-22s could not compress\n", name.c_str());
		return false;
	}

	size_t compressedSize = 0;
	for(size_t i=0; i<chunks.size(); i++)
		compressedSize += chunks[i].data.size();

	std::vector<uint8_t> decoded;
	if( !decompress(chunks, decoded) )
	{
		printf("FAIL %-22s could not decompress %u chunks\n",
			   name.c_str(), (unsigned)chunks.size());
		return false;
	}

	if( decoded.size() != source.size() )
	{
		printf("FAIL %-22s decoded %u bytes, expected %u\n", name.c_str(),
			   (unsigned)decoded.size(), (unsigned)source.size());
		return false;
	}

	for(size_t i=0; i<source.size(); i++)
	{
		if( decoded[i] != source[i] )
		{
			printf("FAIL %-22s first difference at offset %u, %02X not %02X\n",
				   name.c_str(), (unsigned)i, decoded[i], source[i]);
			return false;
		}
	}

	printf("ok   %-22s %u bytes in %u chunks, %u compressed\n", name.c_str(),
		   (unsigned)source.size(), (unsigned)chunks.size(), (unsigned)compressedSize);
	return true;
}

} // namespace

int main()
{
	bool passed = true;

	// a single block, then several, so the window carrying across blocks is
	// covered as well as the first one
	passed &= runCase("random single block", makeRandom(0x1000));
	passed &= runCase("random many blocks",  makeRandom(0x30000));
	passed &= runCase("mixed single block",  makeMixed(0x8000));
	passed &= runCase("mixed many blocks",   makeMixed(0x50000));
	passed &= runCase("runs many blocks",    makeRuns(0x40000));
	passed &= runCase("e8 call bytes",       makeE8(0x40000));

	printf("%s\n", passed ? "all lzx round trips passed" : "lzx round trips FAILED");
	return passed ? 0 : 1;
}
