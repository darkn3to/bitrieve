<h1 align="center">bitrieve - File Recovery for Ext4 filesystem</h1>

Bitrieve is a data undelete module for Linux, designed to recover permanently deleted files on Ext4 file systems before they are overwritten. It uses file-system snapshots to recover deleted files.

## Features
• <b>Snapshot-based recovery</b> — reads previous inode and extent data to locate deleted files.

• Recovers <b>fully intact files</b>, including filenames and non-contiguous blocks.

• <b>Read-only operation</b> — recovered files are written to a recovered/ folder.

• <b>Secure deletion</b> — optional feature to overwrite files with random values.

## Working Overview
Bitrieve operates in two phases.

### 1. Snapshot Creation
The tool scans the filesystem and records metadata including:

• file names  
• inode numbers  
• file sizes  
• extent block mappings  

Each extent is stored as a physical block address and length inside a lightweight binary file (`snapshot.bin`).

This snapshot is typically **< 1MB** and serves as the reference for recovery.

### 2. File Recovery
To recover a file:

1. Metadata is located inside the snapshot
2. Block usage is verified via the EXT4 block bitmap
3. If blocks are still free, file data is reconstructed
4. The file is written to a `recovered/` directory

### Secure Deletion

For secure deletion, Bitrieve:

1. Locates the file's inode and associated extents
2. Overwrites all blocks with random data
3. Removes filesystem metadata
4. Unlinks the file

This ensures disk-level erasure of the file's contents.

(Currently volatile memory regions such as swap are not targeted.)

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
   <br><b>NOTE:</b> Ensure that the device (/dev/sdaX) is not being used in case of performing deletion (just being open in file explorer or in any application also counts as in use) as it may interfere with the operations. 

## Limitations

• Recovery only works if file blocks have not been overwritten.
• Currently optimized for individual file recovery.
• Partial recovery is not enabled.

## Future Scope

• Directory-level snapshot and recovery  
• Improved partial file reconstruction  
• Enhanced secure deletion covering swap and temporary storage

### This project was tested on Ubuntu 22.04 EXT4 Filesystem.
