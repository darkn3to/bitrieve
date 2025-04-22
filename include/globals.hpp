#pragma once
#include <fstream>
#include <vector>

using namespace std;

/*ofstream test("test.txt");
ofstream test2("test2.txt");
ofstream test3("test3.txt");
ofstream bf("bf.txt");*/

#define RESET   "\033[0m"
#define RED     "\033[1;31m"
#define GREEN   "\033[1;32m"
#define YELLOW  "\033[1;33m"

struct Chunk {
    uint64_t addr, len;
};

vector<Chunk> chunks;
