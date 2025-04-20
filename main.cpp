#include <ext2fs/ext2fs.h>
#include "snapshot.hpp"

int main(int argc, char **argv) {
    ext2_filsys fs;
    const char *device_partition = "/dev/sda10";
    errcode_t errcode = ext2fs_open(device_partition, 0, 0, 0, unix_io_manager, &fs);

    if (errcode) {
        std::cerr << "Failed to open device: " << errcode << std::endl;
        return 1;
    }

    create_snapshot(fs);
    return 0;
}