#pragma once
#include <ext2fs/ext2fs.h>
#include <fstream>
#include <cstring>

inline bool inode_uses_extents(const ext2_inode& inode) {
    return inode.i_flags & EXT4_EXTENTS_FL;
}

inline bool read_extent_header(const blk_t *i_block, ext3_extent_header& hdr) {
    std::memcpy(&hdr, i_block, sizeof(ext3_extent_header));
    return hdr.eh_magic == 0xF30A;
}

void read_extents(const blk_t *extent_start, __le16 entries, std::ofstream& out) {
    unsigned short i = 0;

    ext3_extent ext;
    for (; i < entries; i++) {
        std::memcpy(&ext, reinterpret_cast<const uint8_t*>(extent_start) + i * sizeof(ext3_extent), sizeof(ext3_extent));
        //out << "lblock: " << ext.ee_block << " len: " << ext.ee_len << " Physical start: " << ext.ee_start << std::endl;
        
    }
}

void read_extent_idx() {

}