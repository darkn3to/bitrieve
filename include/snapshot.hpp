#pragma once
#include <iostream>
#include <fstream>
#include <filesystem>
#include <ext2fs/ext2fs.h>
#include <cstdlib>
#include <string>
#include <vector>
#include "extents.hpp"

namespace fs = std::filesystem;

inline int print_blocks(ext2_filsys fs, blk_t *blocknr, e2_blkcnt_t blockcnt, blk_t ref_blk, int ref_offset, void *priv_data) {
    std::ofstream *out = static_cast<std::ofstream *>(priv_data);
    if (*blocknr) {
        *out << *blocknr << " ";
    }
    return 0;
}

inline void create_snapshot(ext2_filsys fs) {
    std::string path;
    bool valid_path = false;
    do {
        std::cout << "Please provide a target path: ";
        std::cin >> path;
        if (fs::exists(path)) {
            std::cout << "Path exists.\n";
            valid_path = true;
        } else {
            std::cout << "Path doesn't exist.\n";
        }
    } while (!valid_path);

    unsigned depth;
    do {
        std::cout << "Please provide max-depth of directory traversal: ";
        std::cin >> depth;
        if (depth < 1 || depth > 10) {
            std::cout << "Depth is invalid. Please enter a value between 1 - 10." << std::endl;
        }
    } while (depth < 1 || depth > 10);

    std::string command = "find \"" + path + "\" -maxdepth " + std::to_string(depth) + " -type f -exec stat --format='%i %n' {} \\; | sort -n > inode_names.txt";
    system(command.c_str());

    std::ifstream inode_names("inode_names.txt");
    std::string line;
    ext2_inode inode;

    while (std::getline(inode_names, line)) {
        size_t space = line.find(' ');
        if (space != std::string::npos) {
            ext2fs_read_inode(fs, std::stoi(line.substr(0, space)), &inode);
            ext3_extent_header hdr;

            if (read_extent_header(inode, hdr)) {
                // extent-based logic
            } else {
                std::cout << "Inode " << line.substr(0, space) << " does not use extents." << std::endl;
            }
        }
    }

    std::ofstream snapshot("snapshot.txt");
    errcode_t errcode = ext2fs_read_inode_bitmap(fs);
    if (errcode) {
        std::cout << "Error reading inode bitmap. Error code: " << errcode << std::endl;
    }

    for (ext2_ino_t it = 11; it <= fs->super->s_inodes_count; it++) {
        ext2_inode inode;
        errcode = ext2fs_read_inode(fs, it, &inode);
        if ((inode.i_mode & LINUX_S_IFMT) == LINUX_S_IFREG) {
            if (ext2fs_test_inode_bitmap(fs->inode_map, it)) {
                snapshot << "inode: " << it << "\n";
                snapshot << "size: " << inode.i_size << "\n";
                snapshot << "blocks: ";
                errcode = ext2fs_block_iterate2(fs, it, BLOCK_FLAG_DATA_ONLY, nullptr,
                                               print_blocks, &snapshot);
                if (errcode) {
                    std::cout << "Error iterating blocks for inode " << it << ": " << errcode << std::endl;
                }
                snapshot << "\n\n";
            }
        }
    }
}