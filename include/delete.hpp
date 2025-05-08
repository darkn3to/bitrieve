#pragma once
#include <cstdlib>
#include <iostream>
#include <filesystem>
#include <fstream>
#include <fcntl.h>
#include <unistd.h>
#include <sys/random.h>
#include "extents.hpp"

using namespace std;

namespace delete_namespace
{
    struct cRecord
    {
        uint32_t size, extent_count;
        uint64_t *addr, *len;
    };
}

namespace delete_namespace
{
    cRecord record;
}

namespace fs2 = filesystem;

ext2_ino_t fetch_file(const string &path, ext2_filsys &fs, const string &device)
{
    string command = "stat --format='%i' \"" + path + "\"";

    FILE *pipe = popen(command.c_str(), "r");
    if (!pipe)
    {
        cout << "Failed to run command.\n";
        return false;
    }

    char buffer[32];
    if (fgets(buffer, sizeof(buffer), pipe) != nullptr)
    {
        pclose(pipe);
        ext2_ino_t inode_num = static_cast<ext2_ino_t>(stoul(buffer));
        ext2_inode inode;
        ext2fs_read_inode(fs, inode_num, &inode);

        if (inode.i_dtime != 0)
        {
            inode.i_dtime = time(nullptr);
            ext2fs_write_inode(fs, inode_num, &inode);
            cout << GREEN << "File is already deleted.\n"
                 << RESET;
        }
        else
        {
            ext3_extent_header hdr;

            if (inode_uses_extents(inode))
            {
                read_extent_header(inode.i_block, hdr);

                if (hdr.eh_depth == 0)
                {
                    read_extents(&inode.i_block[3], hdr.eh_entries, fs->blocksize);
                    for (const auto &chunk : chunks)
                    {
                        for (uint64_t i = 0; i < chunk.len; ++i)
                        {
                            ext2fs_unmark_block_bitmap2(fs->block_map, chunk.addr + i);
                        }
                    }
                }
                else
                {
                    ext3_extent_idx idx;
                    for (unsigned short j = 0; j < hdr.eh_entries; j++)
                    {
                        memcpy(&idx, reinterpret_cast<const uint8_t *>(&inode.i_block[3]) + j * sizeof(ext3_extent_idx), sizeof(ext3_extent_idx));
                        uint64_t block_nr = ((uint64_t)idx.ei_leaf_hi << 32) | idx.ei_leaf;
                        read_extent_idx(block_nr, device, fs->blocksize, inode_num);
                        ext2fs_unmark_block_bitmap2(fs->block_map, block_nr);
                    }
                }

                delete_namespace::record.size = ((uint64_t)inode.i_size_high << 32) | inode.i_size;
                delete_namespace::record.extent_count = chunks.size();
                delete_namespace::record.addr = new uint64_t[delete_namespace::record.extent_count];
                delete_namespace::record.len = new uint64_t[delete_namespace::record.extent_count];
                for (unsigned int i = 0; i < delete_namespace::record.extent_count; ++i)
                {
                    delete_namespace::record.addr[i] = chunks[i].addr;
                    delete_namespace::record.len[i] = chunks[i].len;
                }


                chunks.clear();
                return inode_num;
            }
            else
            {
                cout << YELLOW << "Inode " << inode_num << " does not use extents.\n"
                     << RESET;
            }
        }
    }
    else
    {
        cout << RED << "Failed to get inode number.\n"
             << RESET;
        pclose(pipe);
    }

    return 0;
}

void overwrite_blocks(ext2_filsys &fs, const string &device, const string &path)
{
    char *buffer;
    uint32_t bytes_overwritten = 0, curr_extent_len = 0;
    unsigned int i = 0;

    ofstream out(path, ios::binary);

    int fd = open(device.c_str(), O_WRONLY);
    if (fd < 0)
    {
        perror("open");
        return;
    }

    while ((i < delete_namespace::record.extent_count) && (bytes_overwritten < delete_namespace::record.size))
    {
        curr_extent_len = delete_namespace::record.len[i];
        size_t pos_in_extent = 0;

        while ((curr_extent_len > 0) && (bytes_overwritten < delete_namespace::record.size))
        {
            size_t to_write = min(static_cast<size_t>(65536),
                                  min(static_cast<size_t>(curr_extent_len),
                                      static_cast<size_t>(delete_namespace::record.size - bytes_overwritten)));

            buffer = new char[to_write];
            getrandom(buffer, to_write, 0);

            //out.write(buffer, to_write);
            ssize_t bytes_written = pwrite(fd, buffer, to_write, delete_namespace::record.addr[i] + pos_in_extent);
            if (bytes_written <= 0)
            {
                perror("pwrite");
                break;  
            }


            bytes_overwritten += bytes_written;
            pos_in_extent += bytes_written;
            curr_extent_len -= bytes_written;

            delete[] buffer;


        }

        ++i;
    }
    fsync(fd);
    close(fd);

}

void clear_inode_metadata(ext2_filsys &fs, ext2_ino_t inode_num)
{
    ext2_inode inode;
    ext2fs_read_inode(fs, inode_num, &inode);

    inode.i_dtime = time(nullptr);  // Set deletion time
    inode.i_mode = 0;               // Clear file mode
    inode.i_uid = 0;                // Clear user ID
    inode.i_gid = 0;                // Clear group ID
    inode.i_links_count = 0;        // Set link count to 0
    inode.i_size = 0;               // Clear file size
    inode.i_size_high = 0;          // Clear high part of file size
    inode.i_blocks = 0;             // Clear block count
    memset(inode.i_block, 0, sizeof(inode.i_block)); // Clear block pointers

    ext2fs_write_inode(fs, inode_num, &inode);  // Write the cleared inode back to disk
}


void delete_file(const string &path, ext2_filsys &fs, const string &device)
{
    ext2_ino_t inode_num = fetch_file(path, fs, device);
    if (inode_num != 0)
    {
        ext2fs_flush(fs);
        // -------------------unmount-------------------------------
        string command = "umount " + device;
        system(command.c_str());
        

        // -------------------delete-------------------------------
        overwrite_blocks(fs, device, path);
        clear_inode_metadata(fs, inode_num);


        // -------------------remount-------------------------------
        fs2::path file_path(path);
        string directory_path = file_path.parent_path().string();
        command = "mkdir " + directory_path;
        //system(command.c_str());
        command = "mount " + device + " " + directory_path;
        system(("mount -o noload " + device + " \"" + directory_path + "\"").c_str());


        // overwrite_inode();
        //clear_inode_metadata(fs, inode_num);
        
        
        cout << GREEN << "File deleted successfully.\n"
             << RESET;
    }

    delete[] delete_namespace::record.addr;
    delete[] delete_namespace::record.len;
}
