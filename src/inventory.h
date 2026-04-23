#pragma once
#include <vector>
#include <string>
#include <glm/glm.hpp>

enum class ItemType {
    NONE = 0,
    KEY_BRASS,
    KEY_IRON,
    CODE_NOTE,
    BOOK_CLUE,
    MIRROR_SHARD
};

struct InventoryItem {
    ItemType    type;
    std::string name;
    std::string description;
};

class Inventory {
public:
    void addItem(ItemType t);
    bool hasItem(ItemType t) const;
    void removeItem(ItemType t);
    const std::vector<InventoryItem>& items() const { return m_items; }
    int  size() const { return (int)m_items.size(); }

    static std::string nameOf(ItemType t);
    static std::string descOf(ItemType t);

private:
    std::vector<InventoryItem> m_items;
};
