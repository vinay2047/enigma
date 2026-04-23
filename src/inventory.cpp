#include "inventory.h"
#include <algorithm>

std::string Inventory::nameOf(ItemType t) {
    switch (t) {
        case ItemType::KEY_BRASS:   return "Brass Key";
        case ItemType::KEY_IRON:    return "Iron Key";
        case ItemType::CODE_NOTE:   return "Code Note";
        case ItemType::BOOK_CLUE:   return "Old Book";
        case ItemType::MIRROR_SHARD:return "Mirror Shard";
        default: return "Unknown";
    }
}

std::string Inventory::descOf(ItemType t) {
    switch (t) {
        case ItemType::KEY_BRASS:   return "A small brass key. It opens something nearby.";
        case ItemType::KEY_IRON:    return "A heavy iron key.";
        case ItemType::CODE_NOTE:   return "Scrawled numbers: 4821";
        case ItemType::BOOK_CLUE:   return "Lever order: Left, Right, Middle.";
        case ItemType::MIRROR_SHARD:return "A shard of glass.";
        default: return "";
    }
}

void Inventory::addItem(ItemType t) {
    if (!hasItem(t))
        m_items.push_back({t, nameOf(t), descOf(t)});
}

bool Inventory::hasItem(ItemType t) const {
    return std::any_of(m_items.begin(), m_items.end(),
                       [t](const InventoryItem& i){ return i.type == t; });
}

void Inventory::removeItem(ItemType t) {
    m_items.erase(std::remove_if(m_items.begin(), m_items.end(),
        [t](const InventoryItem& i){ return i.type == t; }), m_items.end());
}
