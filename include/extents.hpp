#pragma once
#include <ext2fs/ext2fs.h>
#include <cstring>
#include <unistd.h>
#include <fcntl.h>
#include "globals.hpp"

inline bool inode_uses_extents(const ext2_inode& inode) {
    return inode.i_flags & EXT4_EXTENTS_FL;
}

inline bool read_extent_header(const blk_t *i_block, ext3_extent_header& hdr) {
    memcpy(&hdr, i_block, sizeof(ext3_extent_header));
    return hdr.eh_magic == 0xF30A;
}

void read_extents(const blk_t *extent_start, __le16 entries, unsigned int  blocksize) {
    unsigned short i = 0;

    ext3_extent ext;
    for (; i < entries; i++) {
        memcpy(&ext, reinterpret_cast<const uint8_t*>(extent_start) + i * sizeof(ext3_extent), sizeof(ext3_extent));

        uint64_t phys = ((uint64_t)ext.ee_start_hi << 32) | ext.ee_start;
        //uint64_t len = ext.ee_len & 0x7FFF;
        
        chunks.push_back({
            .lblock = ext.ee_block,
            .addr = phys * blocksize,
            .len = ext.ee_len * blocksize
        });

        /*test2 << "lblock: " << ext.ee_block << " len: " << ext.ee_len << " Physical start: " << ext.ee_start << " unmasked len: " << ext.ee_len << endl;
        test2 << endl;*/
    }

    /*for (auto it: chunks) {
        bf << " [" << it.addr << ", " << it.len << "]" << " ";
    }
    bf << endl;
    chunks.clear();*/
}

void read_extent_idx(const blk_t *extent_start, __le16 entries, unsigned int blocksize) {
    ext3_extent_idx idx;
    unsigned short i = 0;
    int fd = open("/dev/sda10", O_RDONLY);

    for (; i < entries; i++) {
        memcpy(&idx, reinterpret_cast<const uint8_t*>(extent_start) + i * sizeof(ext3_extent_idx), sizeof(ext3_extent_idx));
        uint64_t block_nr = ((uint64_t)idx.ei_leaf_hi << 32) | idx.ei_leaf;
        
        char buffer[blocksize];
        size_t bytes_read = pread(fd, buffer, blocksize, block_nr * blocksize);
        if (bytes_read != blocksize) {
            cerr << "Error reading block: " << block_nr << endl;
            close(fd);
            return;
        }
        ext3_extent_header *hdr = reinterpret_cast<ext3_extent_header*>(buffer);
        /*test3 << "Extent header magic: " << hdr->eh_magic << endl;
        test3 << "Extent header depth: " << hdr->eh_depth << endl;
        test3 << "Extent header entries: " << hdr->eh_entries << endl;
        test3 << "Extent header max: " << hdr->eh_max << endl;
        test3 << endl;*/
        if (hdr->eh_depth == 0) {
            read_extents(reinterpret_cast<uint32_t *>(buffer + sizeof(ext3_extent_header)), hdr->eh_entries, blocksize);
        }
        else {
            read_extent_idx(reinterpret_cast<uint32_t *>(buffer + sizeof(ext3_extent_header)), hdr->eh_entries, blocksize);
        } 
    }

    close(fd);
}