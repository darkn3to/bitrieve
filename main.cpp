#include <iostream>
#include <fstream>
#include <ext2fs/ext2fs.h>
#include <iomanip>
#include <vector>
#include <filesystem>

using namespace std;

namespace fs = std::filesystem;

struct journal_superblock {

};

void log(ext2_filsys &fs) {
    ofstream outfile("superblock.txt");
    outfile << "Superblock Information:\n";
    outfile << "s_inodes_count: " << fs->super->s_inodes_count << "\n";
    outfile << "s_blocks_count: " << fs->super->s_blocks_count << "\n";
    outfile << "s_r_blocks_count: " << fs->super->s_r_blocks_count << "\n";
    outfile << "s_free_blocks_count: " << fs->super->s_free_blocks_count << "\n";
    outfile << "s_free_inodes_count: " << fs->super->s_free_inodes_count << "\n";
    outfile << "s_first_data_block: " << fs->super->s_first_data_block << "\n";
    outfile << "s_log_block_size: " << fs->super->s_log_block_size << "\n";
    outfile << "s_blocks_per_group: " << fs->super->s_blocks_per_group << "\n";
    outfile << "s_inodes_per_group: " << fs->super->s_inodes_per_group << "\n";
    outfile << "s_mtime: " << fs->super->s_mtime << "\n";
    outfile << "s_wtime: " << fs->super->s_wtime << "\n";
    outfile << "s_mnt_count: " << fs->super->s_mnt_count << "\n";
    outfile << "s_max_mnt_count: " << fs->super->s_max_mnt_count << "\n";
    outfile << "s_magic: " << hex << fs->super->s_magic << "\n";
    outfile << "s_state: " << fs->super->s_state << "\n";
    outfile << "s_errors: " << fs->super->s_errors << "\n";
    //outfile << "s_minor_rev_level: " << fs->super->s_minor_rev_level << "\n";
    outfile << "s_lastcheck: " << fs->super->s_lastcheck << "\n";
    outfile << "s_checkinterval: " << fs->super->s_checkinterval << "\n";
    outfile << "s_creator_os: " << fs->super->s_creator_os << "\n";
    outfile << "s_rev_level: " << fs->super->s_rev_level << "\n";
    outfile << "s_first_ino: " << fs->super->s_first_ino << "\n";
    outfile << "s_inode_size: " << fs->super->s_inode_size << "\n";
    outfile << "s_block_group_nr: " << fs->super->s_block_group_nr << "\n";
    //outfile << "s_feature_compat: " << fs->super->s_feature_compat << "\n";
    //outfile << "s_feature_incompat: " << fs->super->s_feature_incompat << "\n";
    //outfile << "s_feature_ro_compat: " << fs->super->s_feature_ro_compat << "\n";
    outfile << "s_uuid: ";
    for (int i = 0; i < 16; ++i) outfile << hex << setw(2) << setfill('0') << (int)fs->super->s_uuid[i];
    outfile << "\n";
    outfile << "s_volume_name: " << fs->super->s_volume_name << "\n";
    outfile << "s_last_mounted: " << fs->super->s_last_mounted << "\n";
    outfile << "s_algorithm_usage_bitmap: " << fs->super->s_algorithm_usage_bitmap << "\n";
    outfile << "s_prealloc_blocks: " << fs->super->s_prealloc_blocks << "\n";
    outfile << "s_prealloc_dir_blocks: " << fs->super->s_prealloc_dir_blocks << "\n";
}

void read_block_data(ext2_filsys fs, blk_t block, int block_size, void *priv_data) {
    char *buf = (char *)malloc(block_size);
    ofstream *out = static_cast<ofstream *>(priv_data);
    if (!buf) {
        *out << "Memory allocation failed\n";
        return;
    }

    errcode_t err = io_channel_read_blk(fs->io, block, 1, buf);
    
    if (err) {
        *out << "Error reading block " << block << ": " << err << "\n";
        free(buf);
        return;
    }
    else {
        int offset = 0;
        while (offset + sizeof(struct ext2_dir_entry) <= block_size) {  
            struct ext2_dir_entry *dir = (struct ext2_dir_entry *)(buf + offset);
        
            if (dir->rec_len < 8 || dir->rec_len + offset > block_size) {
                cout << "Invalid dir entry at offset " << offset << ", rec_len: " << dir->rec_len << endl;
                break;
            }
        
            uint8_t name_len = dir->name_len;
            if (name_len > EXT2_NAME_LEN) {
                cout << "[Possibly deleted] entry with invalid name_len: " << (int)name_len << "\n";
                break;
            }
        
            if (dir->inode == 0) {
                if (name_len > 0) {
                    char name[EXT2_NAME_LEN + 1] = {0};
                    memcpy(name, dir->name, name_len);
                    name[name_len] = '\0';
                    cout << "[Possibly deleted] name: " << name << " (rec_len: " << dir->rec_len << ")\n";
                } else {
                    cout << "[Possibly deleted] entry with no valid name\n";
                }
            } else {
                char name[EXT2_NAME_LEN + 1] = {0};
                memcpy(name, dir->name, name_len);
                name[name_len] = '\0';
                cout << "Directory Entry: " << name
                     << " (Inode: " << dir->inode
                     << ", Rec_len: " << dir->rec_len << ")\n";
            }
        
            offset += dir->rec_len;
        }
        
    }

    free(buf);
}


int print_blocks(ext2_filsys fs, blk_t *blocknr, e2_blkcnt_t blockcnt, blk_t ref_blk, int ref_offset, void *priv_data) {
    ofstream *out = static_cast<ofstream *>(priv_data);
    if (*blocknr) {
        *out << *blocknr << " ";
        /*int block_size = EXT2_BLOCK_SIZE(fs->super);
        read_block_data(fs, *blocknr, block_size, priv_data);  */
    }
    return 0;
}

int print_inode(ext2_filsys fs, ext2_ino_t ino, ofstream &blocklog) {
    int retval = 0;
    errcode_t errcode;
    struct ext2_inode *inode;

    // Allocate memory for the inode structure
    inode = (struct ext2_inode *)operator new(EXT2_INODE_SIZE(fs->super));
    if (!inode) {
        cerr << "Memory allocation failed for inode structure." << endl;
        return ENOMEM;
    }

    // Read the inode data
    errcode = ext2fs_read_inode_full(fs, ino, inode, EXT2_INODE_SIZE(fs->super));
    if (errcode) {
        cerr << "Error reading inode " << ino << ": " << (errcode) << endl;
        retval = errcode;
        goto finally;
    }

    // Log inode information
    blocklog << "Contents of inode " << ino << ":\n";
    blocklog << "Mode: " << inode->i_mode << "\n";
    blocklog << "Size: " << inode->i_size << "\n";
    blocklog << "Blocks: " << inode->i_blocks << "\n";
    blocklog << "Links: " << inode->i_links_count << "\n";

    // Check if the inode is allocated or unallocated
    if (ext2fs_test_inode_bitmap(fs->inode_map, ino)) {
        blocklog << "Inode is Allocated\n";
    } else {
        blocklog << "Inode is Unallocated\n";
    }

    if (inode->i_dtime != 0) {
        blocklog << "Inode is Deleted file\n";
    } else {
        blocklog << "Inode is Active file\n";
    }

    // Check if the inode is a directory and if it is deleted
    if ((inode->i_mode & LINUX_S_IFMT) == LINUX_S_IFDIR) {
        if (inode->i_dtime != 0) {
            blocklog << "Inode " << ino << " is a deleted directory.\n";
        } else {
            blocklog << "Inode " << ino << " is an active directory.\n";
        }
    }

finally:
    // Free the allocated memory
    delete inode;
    return retval;
}

void create_snapshot(ext2_filsys fs) {
    string path;
    bool valid_path = false;
    do {
        cout << "Please provide a target path: ";
        cin >> path;

        if (fs::exists(path)) {
            cout << "Path exists. " << endl;
            valid_path = true;
        }
        else {
            cout << "Path doesn't exist. " << endl;
        }
    } while(!valid_path);

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
    return;

    ofstream snapshot("snapshot.txt");

    errcode_t errcode;
    errcode = ext2fs_read_inode_bitmap(fs);
    if (errcode) {
        cout << "Error reading inode bitmap. Error code: " << errcode << endl;
    }
    for (ext2_ino_t it = 11; it <= fs->super->s_inodes_count; it++) {  
        struct ext2_inode inode;
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
        //print_inode(fs, it, snapshot);
       
    }
}

void recover_files(ext2_filsys fs, const char *snapshot_file, const char *file_name) {
    ifstream snapshot(snapshot_file);
    if (!snapshot.is_open()) {
        cout << "Snapshot file: " << snapshot_file  << " does not exist." << endl;
        return;
    }

    
}

int main(int argc, char **argv) {
    ext2_filsys fs;
    const char *device_partition = "/dev/sda9";

    errcode_t errcode;

    errcode = ext2fs_open(device_partition, 0, 0, 0, unix_io_manager, &fs);

    create_snapshot(fs);
    //recover_files(fs, "snapshot.txt");

    return 0;
}