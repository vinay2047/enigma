#pragma once
#include <string>

struct SaveData {
    int   currentRoom       {0};
    float playerX           {0.f};
    float playerY           {1.7f};
    float playerZ           {0.f};
    float timerLeft         {600.f};

    // Puzzle solved flags (indexed by puzzle id 0-6)
    bool  puzzleSolved[7]   {};

    // Inventory flags
    bool  hasKeyBrass       {false};
    bool  hasKeyIron        {false};
    bool  hasCodeNote       {false};
    bool  hasBookClue       {false};
};

class SaveLoad {
public:
    static bool save(const std::string& path, const SaveData& data);
    static bool load(const std::string& path, SaveData& out);
};
