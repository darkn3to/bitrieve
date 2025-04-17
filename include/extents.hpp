#pragma once
#include <ext2fs/ext2fs.h>
#include <fstream>
#include <cstring>

inline bool inode_uses_extents(const ext2_inode& inode) {
    return inode.i_flags & EXT4_EXTENTS_FL;
}

inline bool read_extent_header(const ext2_inode& inode, ext3_extent_header& hdr) {
    if (!inode_uses_extents(inode)) return false;
    std::memcpy(&hdr, &inode.i_block, sizeof(ext3_extent_header));
    return hdr.eh_magic == 0xF30A;
}