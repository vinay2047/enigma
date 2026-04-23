#include "saveload.h"
#include <fstream>
#include <cstring>

bool SaveLoad::save(const std::string& path, const SaveData& data) {
    std::ofstream f(path, std::ios::binary);
    if (!f) return false;
    f.write(reinterpret_cast<const char*>(&data), sizeof(SaveData));
    return f.good();
}

bool SaveLoad::load(const std::string& path, SaveData& out) {
    std::ifstream f(path, std::ios::binary);
    if (!f) return false;
    f.read(reinterpret_cast<char*>(&out), sizeof(SaveData));
    return f.good();
}
