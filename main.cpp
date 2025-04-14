#include <iostream>
#include <fstream>
#include <ext2fs/ext2fs.h>
#include <sys/statvfs.h>

using namespace std;

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
    outfile << "s_minor_rev_level: " << fs->super->s_minor_rev_level << "\n";
    outfile << "s_lastcheck: " << fs->super->s_lastcheck << "\n";
    outfile << "s_checkinterval: " << fs->super->s_checkinterval << "\n";
    outfile << "s_creator_os: " << fs->super->s_creator_os << "\n";
    outfile << "s_rev_level: " << fs->super->s_rev_level << "\n";
    outfile << "s_def_resuid: " << fs->super->s_def_resuid << "\n";
    outfile << "s_def_resgid: " << fs->super->s_def_resgid << "\n";
    outfile << "s_first_ino: " << fs->super->s_first_ino << "\n";
    outfile << "s_inode_size: " << fs->super->s_inode_size << "\n";
    outfile << "s_block_group_nr: " << fs->super->s_block_group_nr << "\n";
    outfile << "s_feature_compat: " << fs->super->s_feature_compat << "\n";
    outfile << "s_feature_incompat: " << fs->super->s_feature_incompat << "\n";
    outfile << "s_feature_ro_compat: " << fs->super->s_feature_ro_compat << "\n";
    outfile << "s_uuid: " << fs->super->s_uuid << "\n";
    outfile << "s_volume_name: " << fs->super->s_volume_name << "\n";
    outfile << "s_last_mounted: " << fs->super->s_last_mounted << "\n";
    outfile << "s_algorithm_usage_bitmap: " << fs->super->s_algorithm_usage_bitmap << "\n";
    outfile << "s_prealloc_blocks: " << fs->super->s_prealloc_blocks << "\n";
    outfile << "s_prealloc_dir_blocks: " << fs->super->s_prealloc_dir_blocks << "\n";
}

int main(int argc, char **argv) {
    ext2_filsys fs;
    const char *device_partition = "/dev/sda8";

    errcode_t errcode;

    errcode = ext2fs_open(device_partition, 0, 0, 0, unix_io_manager, &fs);

    if (errcode) {
        cout << "Error opening filesystem: " << errcode << endl;
        return 1;
    }
    
    log(fs);
    
    errcode = ext2fs_read_inode_bitmap(fs);

    if (errcode) {
        cout << "Error reading inode bitmap. Error code: " << errcode << endl;
    }
    int c=0;
    for (ext2_ino_t it = 1; it < fs->super->s_inodes_count; it++) {
        if (ext2fs_test_inode_bitmap(fs->inode_map, it)) {
            //cout << "Inode " << it << " is used" << endl;
            ++c;
        }
    }

    cout << c << " inodes are used." << endl;

    return 0;
}