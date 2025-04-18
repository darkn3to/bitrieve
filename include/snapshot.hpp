#pragma once
#include <iostream>
#include <fstream>
#include <filesystem>
#include <ext2fs/ext2fs.h>
#include <cstdlib>
#include <string>
#include <vector>
#include "extents.hpp"

using namespace std;

namespace fs = filesystem;

inline int print_blocks(ext2_filsys fs, blk_t *blocknr, e2_blkcnt_t blockcnt, blk_t ref_blk, int ref_offset, void *priv_data) {
    ofstream *out = static_cast<ofstream *>(priv_data);
    if (*blocknr) {
        *out << *blocknr << " ";
    }
    return 0;
}

inline void create_snapshot(ext2_filsys fs) {
    string path;
    bool valid_path = false;
    do {
        cout << "Please provide a target path: ";
        cin >> path;
        if (fs::exists(path)) {
            cout << "Path exists.\n";
            valid_path = true;
        } else {
            cout << "Path doesn't exist.\n";
        }
    } while (!valid_path);

    unsigned depth;
    do {
        cout << "Please provide max-depth of directory traversal: ";
        cin >> depth;
        if (depth < 1 || depth > 10) {
            cout << "Depth is invalid. Please enter a value between 1 - 10." << endl;
        }
    } while (depth < 1 || depth > 10);

    string command = "find \"" + path + "\" -maxdepth " + to_string(depth) + " -type f -exec stat --format='%i %n' {} \\; | sort -n > inode_names.txt";
    system(command.c_str());

    ifstream inode_names("inode_names.txt");
    string line;
    ext2_inode inode;

    ofstream test("test.txt");
    ofstream test2("test2.txt");

    while (getline(inode_names, line)) {
        size_t space = line.find(' ');
        if (space != string::npos) {
            ext2fs_read_inode(fs, stoi(line.substr(0, space)), &inode);
            ext3_extent_header hdr;

            if (inode_uses_extents(inode)) {  // handle it the ext4 way 
                read_extent_header(inode.i_block, hdr);
                /*test << "Inode " << line.substr(0, space) << " uses extents." << endl;
                test << "Extent depth: " << hdr.eh_depth << endl;
                test << "Extent entries: " << hdr.eh_entries << endl;
                test << "Extent max entries: " << hdr.eh_max << endl;
                test << "Extent generation: " << hdr.eh_generation << endl;*/
                

                if (hdr.eh_depth == 0) {
                    read_extents(&inode.i_block[3], hdr.eh_entries, test2);
                }
                else {
                    read_extent_idx();
                }
            } 
            else { // handle it the ext2/ext3 way. future work.
                cout << "Inode " << line.substr(0, space) << " does not use extents." << endl;
            }
        }
    }
    
    return;

    ofstream snapshot("snapshot.txt");
    errcode_t errcode = ext2fs_read_inode_bitmap(fs);
    if (errcode) {
        cout << "Error reading inode bitmap. Error code: " << errcode << endl;
    }


    /*for (ext2_ino_t it = 11; it <= fs->super->s_inodes_count; it++) {
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
                    cout << "Error iterating blocks for inode " << it << ": " << errcode << endl;
                }
                snapshot << "\n\n";
            }
        }
    }*/
}