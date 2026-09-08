#!/usr/bin/env python3
"""Extract files from an Xbox 360 STFS package (CON / LIVE / PIRS).

Title updates ship as STFS packages with the .xexp patch inside, so getting at
one for testing means unpacking the container first.

Block numbering skips the hash tables interleaved through the data, which is
what BlockToOffset below accounts for; the layout follows xenia's
StfsContainerDevice.

Usage:
    python stfs_extract.py <package> [-o OUTDIR] [--list]
"""
import argparse
import os
import struct
import sys

BLOCK_SIZE = 0x1000
END_OF_CHAIN = 0xFFFFFF
BLOCKS_PER_HASH_LEVEL = (170, 28900, 4913000)

HEADER_SIZE_OFF = 0x340   # BE u32
VOLUME_DESC_OFF = 0x379   # StfsVolumeDescriptor, 0x24 bytes
ENTRY_SIZE = 0x40


def round_up(v, n):
    return (v + n - 1) // n * n


class Stfs:
    def __init__(self, data):
        self.d = data
        magic = data[:4]
        if magic not in (b"CON ", b"LIVE", b"PIRS"):
            raise ValueError("not an STFS package (magic %r)" % magic)
        self.magic = magic.decode("ascii").strip()

        self.header_size, = struct.unpack_from(">I", data, HEADER_SIZE_OFF)
        self.data_offset = round_up(self.header_size, BLOCK_SIZE)

        v = VOLUME_DESC_OFF
        if data[v] != 0x24:
            raise ValueError("volume descriptor length is 0x%02X, expected 0x24"
                             % data[v])
        flags = data[v + 2]
        self.read_only = bool(flags & 1)
        # A read-only package keeps one backing block per hash table; a
        # writable one keeps two, for resiliency.
        self.blocks_per_hash_table = 1 if self.read_only else 2
        self.file_table_block_count, = struct.unpack_from("<H", data, v + 3)
        self.file_table_block_number = (data[v + 5] | (data[v + 6] << 8) |
                                        (data[v + 7] << 16))
        self.total_blocks, = struct.unpack_from(">I", data, v + 0x1C)

    def block_to_offset(self, block_index):
        block = block_index
        for level_base in BLOCKS_PER_HASH_LEVEL:
            block += ((block_index + level_base) // level_base) * \
                     self.blocks_per_hash_table
            if block_index < level_base:
                break
        return self.data_offset + (block << 12)

    def read_block(self, block_index):
        off = self.block_to_offset(block_index)
        return self.d[off:off + BLOCK_SIZE]

    def hash_entry(self, block_index):
        """The hash table record for a block carries its next-block pointer."""
        level_base = BLOCKS_PER_HASH_LEVEL[0]
        # the level-0 hash table immediately precedes each run of 170 blocks
        table_block = (block_index // level_base) * \
            (level_base + self.blocks_per_hash_table)
        if block_index >= level_base:
            table_block += ((block_index // BLOCKS_PER_HASH_LEVEL[1]) + 1) * \
                self.blocks_per_hash_table
        off = self.data_offset + (table_block << 12) + \
            (block_index % level_base) * 0x18
        rec = self.d[off:off + 0x18]
        if len(rec) < 0x18:
            return END_OF_CHAIN
        return rec[0x15] << 16 | rec[0x16] << 8 | rec[0x17]

    def entries(self):
        out = []
        block = self.file_table_block_number
        for _ in range(self.file_table_block_count):
            buf = self.read_block(block)
            for i in range(BLOCK_SIZE // ENTRY_SIZE):
                e = buf[i * ENTRY_SIZE:(i + 1) * ENTRY_SIZE]
                if not e or e[0] == 0:
                    continue
                flags = e[0x28]
                name_len = flags & 0x3F
                if name_len == 0:
                    continue
                name = e[:name_len].decode("utf-8", "replace")
                start = e[0x2F] | (e[0x30] << 8) | (e[0x31] << 16)
                dir_index, = struct.unpack_from(">h", e, 0x32)
                length, = struct.unpack_from(">I", e, 0x34)
                out.append(dict(name=name, start=start, length=length,
                                directory=bool(flags & 0x80),
                                contiguous=bool(flags & 0x40),
                                dir_index=dir_index))
            block = self.hash_entry(block)
            if block == END_OF_CHAIN:
                break
        return out

    def read_file(self, entry):
        remaining = entry["length"]
        block = entry["start"]
        parts = []
        while remaining > 0 and block != END_OF_CHAIN:
            buf = self.read_block(block)
            take = min(remaining, BLOCK_SIZE)
            parts.append(buf[:take])
            remaining -= take
            block = block + 1 if entry["contiguous"] else self.hash_entry(block)
        return b"".join(parts)


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("package")
    ap.add_argument("-o", "--outdir", default=".")
    ap.add_argument("--list", action="store_true", help="list contents only")
    args = ap.parse_args()

    stfs = Stfs(open(args.package, "rb").read())
    print("%s package, header 0x%X, data at 0x%X, %d blocks, %s" %
          (stfs.magic, stfs.header_size, stfs.data_offset, stfs.total_blocks,
           "read-only" if stfs.read_only else "writable"))

    entries = stfs.entries()
    if not entries:
        sys.exit("no directory entries found")

    for e in entries:
        kind = "dir " if e["directory"] else "file"
        print("  %s %-40s %10d bytes  block %d" %
              (kind, e["name"], e["length"], e["start"]))

    if args.list:
        return 0

    os.makedirs(args.outdir, exist_ok=True)
    written = 0
    for e in entries:
        if e["directory"]:
            continue
        data = stfs.read_file(e)
        if len(data) != e["length"]:
            print("  ! %s: got %d bytes, expected %d" %
                  (e["name"], len(data), e["length"]))
        path = os.path.join(args.outdir, e["name"])
        open(path, "wb").write(data)
        written += 1
    print("extracted %d file(s) to %s" % (written, os.path.abspath(args.outdir)))
    return 0


if __name__ == "__main__":
    sys.exit(main())
