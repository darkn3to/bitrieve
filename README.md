<h1 align="center">bitrieve - File Recovery for Ext4 filesystem</h1>

Bitrieve is a data undelete module for Linux, designed to recover permanently deleted files on Ext4 file systems before they are overwritten. It uses file-system snapshots to recover deleted files.

## Features
• <b>Snapshot-based recovery</b> — reads previous inode and extent data to locate deleted files.

• Recovers <b>fully intact files</b>, including filenames and non-contiguous blocks.

• <b>Read-only operation</b> — recovered files are written to a recovered/ folder.

• <b>Secure deletion</b> — optional feature to overwrite files with random values.

## Working Overview
Bitrieve works in two primary phases: snapshot creation and file recovery.<br>During the snapshot phase, it collects and stores essential metadata—such as filenames, file sizes, and extent details—in a binary file (snapshot.bin). Each extent is recorded as a physical block address and its length, allowing precise reconstruction of the file’s original storage layout. This snapshot is lightweight, typically under 1 MB, and serves as the sole reference point for recovery. <br>In the recovery phase, Bitrieve searches this snapshot for the target file’s metadata. It then verifies whether the physical blocks referenced by the snapshot are still unallocated by consulting the EXT4 block bitmap. If the blocks have not been reused, it extracts the data and reconstructs the file, saving it in a separate recovered directory to maintain read-only interaction with the original filesystem. <br>For secure deletion, Bitrieve identifies the file’s inode and associated extents, then overwrites all its blocks with random values. It proceeds to unmount the filesystem, erase the metadata, remount it, and finally unlinks the file using its inode. Although this process ensures disk-level erasure, the tool currently does not target volatile memory, presenting an opportunity for future enhancements in memory-safe deletion.

## Usage 
1. Run the `make` command after cloning and opening the repository.
2. For snapshot creation:
   ```cmd
   sudo ./bitrieve create -dev /dev/sdaX -p <input_directory_path> -d <depth_parameter>
   ```
   For recovering file:
   ```cmd
   sudo ./bitrieve recover -dev /dev/sdaX -p <input_file_path> -o <output_directory_path>
   ```
   For deleting a file:
   ```cmd
   sudo ./bitrieve delete -dev /dev/sdaX -p <input_file_path>
   ```
   The `-dev` argument is used to specify the device. <br>The `p` argument specifies the path to the target file or directory and the `depth` argument determines how many levels (sub-directories) Bitrieve should cover while taking the snapshot of the fs.

## Future Scope
In its current form, Bitrieve supports snapshot-based recovery of individual files, but future enhancements aim to expand both functionality and coverage. One major extension is adding support for backing up and recovering entire directories, enabling users to preserve and restore complete folder structures rather than just individual files. Additionally, the secure deletion feature can be made more robust by incorporating manipulation of temporary memory areas such as swap space and other volatile storage regions, ensuring sensitive data is purged more comprehensively. There is also significant scope for improving partial file recovery—while currently disabled to avoid returning incomplete results, future versions could implement smarter heuristics to recover whatever file fragments are still intact, optionally marking or flagging partially reconstructed files for user awareness. These additions would greatly improve Bitrieve’s effectiveness and make it more resilient in real-world scenarios where perfect recovery is not always possible.
  
