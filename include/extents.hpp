#pragma once
#include <ext2fs/ext2fs.h>
#include <cstring>
#include <unistd.h>
#include <fcntl.h>
#include "globals.hpp"

inline bool inode_uses_extents(const ext2_inode &inode)
{
    return inode.i_flags & EXT4_EXTENTS_FL;
}

inline bool read_extent_header(const blk_t *i_block, ext3_extent_header &hdr)
{
    memcpy(&hdr, i_block, sizeof(ext3_extent_header));
    return hdr.eh_magic == 0xF30A;
}

void read_extents(const blk_t *extent_start, __le16 entries, unsigned int blocksize)
{
    unsigned short i = 0;

    ext3_extent ext;
    for (; i < entries; i++)
    {
        memcpy(&ext, reinterpret_cast<const uint8_t *>(extent_start) + i * sizeof(ext3_extent), sizeof(ext3_extent));

        uint64_t phys = ((uint64_t)ext.ee_start_hi << 32) | ext.ee_start;
        // uint64_t len = ext.ee_len & 0x7FFF;

        chunks.push_back({.addr = phys * blocksize,
                          .len = ext.ee_len * blocksize});
    }
}

void read_extent_idx(uint64_t block_nr, const string &device, const unsigned int blocksize, ext2_ino_t inode_num)
{
    int fd = open(device.c_str(), O_RDONLY);
    char buffer[blocksize];
    size_t bytes_read = pread(fd, buffer, blocksize, block_nr * blocksize);
    if (bytes_read != blocksize)
    {
        cout << "[ " << inode_num << " ] " << "Error reading block: " << block_nr << ". Bytes read: " << bytes_read << endl;
        close(fd);
        return;
    }

    ext3_extent_header *hdr = reinterpret_cast<ext3_extent_header *>(buffer);

    if (hdr->eh_depth == 0)
    {
        read_extents(reinterpret_cast<blk_t *>(buffer + sizeof(ext3_extent_header)), hdr->eh_entries, blocksize);
    }
    else
    {
        ext3_extent_idx idx;
        for (unsigned short j = 0; j < hdr->eh_entries; j++)
        {
            memcpy(&idx, buffer + sizeof(ext3_extent_header) + j * sizeof(ext3_extent_idx), sizeof(ext3_extent_idx));
            uint64_t new_block_nr = ((uint64_t)idx.ei_leaf_hi << 32) | idx.ei_leaf;

            read_extent_idx(new_block_nr, device, blocksize, inode_num);
        }
    }

    close(fd);
}
