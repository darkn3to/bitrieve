#include <fstream>
#include <vector>

using namespace std;

/*ofstream test("test.txt");
ofstream test2("test2.txt");
ofstream test3("test3.txt");
ofstream bf("bf.txt");*/
ofstream snapshot("snapshot.txt");
//ofstream snapshot("snapshot.bin", std::ios::binary);

struct Chunk {
    uint64_t lblock, addr, len;
};

vector<Chunk> chunks;
