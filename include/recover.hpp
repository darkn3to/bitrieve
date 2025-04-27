#pragma once
#include <iostream>
#include <stdbool.h>
#include "globals.hpp"
#include <filesystem>

#define BUFFER_SIZE 65536 // 64 KB buffer for re-writing files.

namespace fs2 = filesystem;

struct cRecord {
    unsigned int inode_num;
    string file_path;
    uint64_t size;
    uint32_t extent_count;
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

        verify.read(reinterpret_cast<char*>(&record.size), sizeof(record.size));
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


void get_overwritten_blocks(ext2_filsys &fs) {
    for (unsigned int i = 0; i < record.extent_count; i++) {
        uint64_t start = record.addr[i]/fs->blocksize;
        uint64_t lenstart = record.len[i]/fs->blocksize;
        for (uint64_t j = 0; j < lenstart; j++) {
            blk_t blk = start + j;
            int used = ext2fs_test_block_bitmap(fs->block_map, blk);
            if (used < 0) {
                cerr << "Bitmap test error on block " << blk
                << ": " << used << "\n";
                continue;
            }
            if (used) {
                cout << YELLOW << "Block is overwritten: " << blk << "\n" << RESET;
            }
        }
    } 
}

void print_fs_vitals(ext2_filsys fs) {
    cout << GREEN << "Filesystem Vitals:\n" << RESET;
    cout << "Block Size: " << fs->blocksize << " bytes\n";
    cout << "Total Blocks: " << fs->super->s_blocks_count << "\n";
    cout << "Free Blocks: " << fs->super->s_free_blocks_count << "\n";
    cout << "Total Inodes: " << fs->super->s_inodes_count << "\n";
    cout << "Free Inodes: " << fs->super->s_free_inodes_count << "\n";
    cout << "First Data Block: " << fs->super->s_first_data_block << "\n";
    cout << "Volume Name: " << fs->super->s_volume_name << "\n";
}

void write_file(ext2_filsys &fs, const string &output_path, const string &filename, const string &device) {
    const string recovered_dir = output_path + "/recovered";
    if (!fs2::exists(recovered_dir)) {
        fs2::create_directories(recovered_dir);
    }
    const string full_file_path = recovered_dir + "/" + filename;

    cout << endl << GREEN << "Writing file to: " 
         << full_file_path << "\n" << RESET;

    char buffer[BUFFER_SIZE];

    ofstream out(full_file_path, ios::binary);
    uint64_t bytes_written = 0, curr_extent_len = 0;
    unsigned int i = 0;
    int fd = open(device.c_str(), O_RDONLY);
    while ((i < record.extent_count) && (bytes_written < record.size)) {
        curr_extent_len = record.len[i];
        size_t pos_in_extent = 0;
    
        while ((curr_extent_len > 0) && (bytes_written < record.size)) {
            size_t to_read = std::min(static_cast<size_t>(BUFFER_SIZE),
                          std::min(static_cast<size_t>(curr_extent_len),
                                   static_cast<size_t>(record.size - bytes_written)));

    
            ssize_t bytes_read = pread(fd, buffer, to_read, record.addr[i] + pos_in_extent);
            if (bytes_read <= 0) {
                perror("pread");
                break;  
            }
    
            out.write(buffer, bytes_read);
            bytes_written += bytes_read;
            pos_in_extent += bytes_read;
            curr_extent_len -= bytes_read;
        }
    
        ++i;
    }
    close(fd);
    out.close();
    delete[] record.addr;
    delete[] record.len;
}

const string extract_filename(const string &path) {
    size_t last_slash = path.find_last_of("/\\");
    if (last_slash == string::npos) {
        return path; 
    }
    return path.substr(last_slash + 1);
}

void recover(const string& path, ext2_filsys &fs, const string & output_path, const string &device) {
    if (check_file_path(path)) {
        ext2_inode inode;
        ext2fs_read_inode(fs, record.inode_num, &inode);
        if (inode.i_dtime == 0) {
            cout << GREEN << "File is not deleted. No recovery needed. Check Rubbish Bin for file if still not found.\n" << RESET;
        }
        else {
            //recovery logic
            errcode_t err = ext2fs_read_block_bitmap(fs);
            if (err) {
                cerr << RED << "Failed to read block bitmap: " << err << "\n" << RESET;
                return;
            }


            //print_fs_vitals(fs);
            get_overwritten_blocks(fs);
            write_file(fs, output_path, extract_filename(record.file_path), device);
        }
    }
    
}
