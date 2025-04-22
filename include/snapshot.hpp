#pragma once
#include <iostream>
#include <filesystem>
#include <cstdlib>
#include <string>
#include "extents.hpp"

using namespace std;

namespace fs2 = filesystem;

inline void create_snapshot(ext2_filsys fs, string path, unsigned int depth, const string &device)
{
    ofstream snapshot("snapshot.bin", ios::binary);

    string command = "find \"" + path + "\" -maxdepth " + to_string(depth) + " -type f -exec stat --format='%i %n' {} \\; | sort -n > inode_names.txt";
    system(command.c_str());

    ifstream inode_names("inode_names.txt");

    string line;
    ext2_inode inode;

    while (getline(inode_names, line))
    {
        size_t space = line.find(' ');
        if (space == string::npos)
            continue;

        string inode_str = line.substr(0, space);
        string path = line.substr(space + 1);
        ext2_ino_t inode_num = static_cast<ext2_ino_t>(stoul(inode_str));

        ext2fs_read_inode(fs, inode_num, &inode);
        ext3_extent_header hdr;

        if (inode_uses_extents(inode))
        {
            read_extent_header(inode.i_block, hdr);

            if (hdr.eh_depth == 0)
            {
                read_extents(&inode.i_block[3], hdr.eh_entries, fs->blocksize);
            }
            else
            {
                // read_extent_idx(&inode.i_block[3], hdr.eh_entries, fs->blocksize, path, inode_num);
                ext3_extent_idx idx;
                for (unsigned short j = 0; j < hdr.eh_entries; j++)
                {
                    memcpy(&idx, reinterpret_cast<const uint8_t *>(&inode.i_block[3]) + j * sizeof(ext3_extent_idx), sizeof(ext3_extent_idx));

                    uint64_t block_nr = ((uint64_t)idx.ei_leaf_hi << 32) | idx.ei_leaf;

                    read_extent_idx(block_nr, device, fs->blocksize, inode_num);
                }
            }
            snapshot.write(reinterpret_cast<const char *>(&inode_num), sizeof(ext2_ino_t));

            uint32_t path_len = path.size();
            snapshot.write(reinterpret_cast<const char *>(&path_len), sizeof(path_len));
            snapshot.write(path.c_str(), path_len);

            uint32_t size = inode.i_size;
            snapshot.write(reinterpret_cast<const char *>(&size), sizeof(size));

            uint32_t extent_count = chunks.size();
            snapshot.write(reinterpret_cast<const char *>(&extent_count), sizeof(extent_count));

            for (const auto &it : chunks)
            {
                snapshot.write(reinterpret_cast<const char *>(&it.addr), sizeof(uint64_t));
                snapshot.write(reinterpret_cast<const char *>(&it.len), sizeof(uint64_t));
            }
            chunks.clear();
        }
        else
        {
            cout << "Inode " << inode_num << " does not use extents." << endl;
        }
    }

    snapshot.close();
    inode_names.close();

    return;
}