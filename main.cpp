#include <string>
#include "snapshot.hpp"
#include "recover.hpp"

void list_deleted_files(ext2_filsys fs);

int main(int argc, char** argv) {
    if (argc < 4) {
        cerr << "Sufficient parameters not found. \nUsage:\n"
             << "  ./bitrieve create  -dev <device> -p <path> -d <depth>\n"
             << "  ./bitrieve recover -dev <device> -p <path> -o <output_path>\n"
             << "  ./bitrieve ls      -dev <device>\n";
        return 1;
    }

    string command = argv[1];
    string device, path, output;
    unsigned int depth = 0;

    for (int i = 2; i < argc; ++i) {
        string arg = argv[i];
        if (arg == "-dev" && i + 1 < argc) {
            device = argv[++i];
        } 
        else if (arg == "-p" && i + 1 < argc) {
            path = argv[++i];
            if (path.empty()) {
                cerr << "Error: -p <path> is required for recovery.\n";
                return 1;
            }
        } 
        else if (arg == "-d" && i + 1 < argc) {
            depth = stoi(argv[++i]);
            if (depth < 1 || depth > 10) {
                cout << "Depth is invalid. Please enter a value between 1 - 10." << endl;
                return 1;
            }
        } 
        else if (arg == "-o" && i + 1 < argc) {
            output = argv[++i]; 
        } 
        else {
            cerr << "Unknown argument: " << arg << "\n";
            return 1;
        }
    }

    if (device.empty()) {
        cerr << "Error: device not specified. Use -dev <device> after calling ./bitrieve.\n";
        return 1;
    }

    ext2_filsys fs;
    errcode_t errcode = ext2fs_open(device.c_str(), 0, 0, 0, unix_io_manager, &fs);

    if (errcode) {
        cerr << errcode <<" : Failed to open device: " << device << endl;
        return 1;
    }

    if (command == "create") {
        create_snapshot(fs, path, depth, device);
    } 
    else if (command == "recover") {
        if (output.empty()) {
            cerr << "Error: -o <output_path> is required for recovery.\n";
            return 1;
        }
        recover(path, fs, output, device);
    } 
    else if (command == "ls") {
        //list_deleted_files(fs);
    } 
    else {
        cerr << "Unknown command: " << command << "\n";
        return 1;
    }

    return 0;
}
