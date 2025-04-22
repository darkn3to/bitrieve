#pragma once
#include <iostream>
#include <stdbool.h>
#include "globals.hpp"

struct cRecord {
    unsigned int inode_num;
    string file_path;
    uint32_t size, extent_count;
    uint64_t *addr, *len;
};

cRecord record;

void show_record() {
    cout << "Record: " << record.inode_num << "\n";
    cout << "File Path: " << record.file_path << "\n";
    cout << "Size: " << record.size << "\n";
    cout << "Extent Count: " << record.extent_count << "\n";
    cout << "Address, Len: ";
    for (unsigned int i = 0; i < record.extent_count; ++i) {
        cout << "[ " << record.addr[i] << ", " << record.len[i] << " ] ";
    }
    cout << endl;
}

bool check_file_path(const string& path) {
    ifstream verify("snapshot.bin", ios::binary | ios::ate);
    if (!verify) {
        cout << RED << "Failed to open snapshot.bin for reading.\n" << RESET;
        return false;
    }

    if (verify.tellg() == 0) {
        cout << RED << "snapshot.bin is empty. Run `create` before `recover`.\n" << RESET;
        return false;
    }

    verify.seekg(0);
    cout << "Checking snapshot.bin for file path: " << path << endl << endl;

    uint32_t path_len;

    while (verify.peek() != EOF) {
        verify.read(reinterpret_cast<char*>(&record.inode_num), sizeof(unsigned int));
        verify.read(reinterpret_cast<char*>(&path_len), sizeof(path_len));

        record.file_path.resize(path_len);
        verify.read(&record.file_path[0], path_len);
        record.file_path.erase(record.file_path.find_last_not_of(" \n\r\t") + 1);

        verify.read(reinterpret_cast<char*>(&record.size), sizeof(uint32_t));
        verify.read(reinterpret_cast<char*>(&record.extent_count), sizeof(uint32_t));

        // Allocate space
        record.addr = new uint64_t[record.extent_count];
        record.len = new uint64_t[record.extent_count];

        for (unsigned int i = 0; i < record.extent_count; ++i) {
            verify.read(reinterpret_cast<char*>(&record.addr[i]), sizeof(uint64_t));
            verify.read(reinterpret_cast<char*>(&record.len[i]),  sizeof(uint64_t));
        }

        if (verify.fail()) {
            cout << YELLOW << "Read error or format mismatch at record for inode "
                 << record.inode_num << "\n";
            break;
        }

        if (path == record.file_path) {
            cout << GREEN << "File path found: " << record.file_path << "\n" << RESET;
            show_record();
            return true;
        }

        delete[] record.addr;
        delete[] record.len;
    }

    cout << YELLOW << "File path not found in snapshot: " << path << RESET << endl;
    verify.close();
    return false;
}


void get_used_blocks(ext2_filsys fs) {

}

void recover(const string& path, ext2_filsys fs) {
    if (check_file_path(path)) {
        ext2_inode inode;
        ext2fs_read_inode(fs, record.inode_num, &inode);
        if (inode.i_dtime == 0) {
            cout << GREEN << "File is not deleted. No recovery needed. Check Rubbish Bin for file if still not found.\n" << RESET;
        }
        else {
            //recovery logic
            //get_used_blocks(fs);
        }
    }
    
}