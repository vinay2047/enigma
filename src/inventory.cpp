#include "inventory.h"
#include <algorithm>

std::string Inventory::nameOf(ItemType t) {
    switch (t) {
        case ItemType::KEY_BRASS:   return "Brass Key";
        case ItemType::KEY_IRON:    return "Iron Key";
        case ItemType::CODE_NOTE:   return "Code Note";
        case ItemType::BOOK_CLUE:   return "Old Book";
        case ItemType::MIRROR_SHARD:return "Mirror Shard";
        case ItemType::CRYSTAL_RED:  return "Red Crystal";
        case ItemType::CRYSTAL_BLUE: return "Blue Crystal";
        case ItemType::CRYSTAL_GREEN:return "Green Crystal";
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
        case ItemType::CRYSTAL_RED:  return "A warm crimson crystal, pulsing with energy.";
        case ItemType::CRYSTAL_BLUE: return "An icy blue crystal, cold to the touch.";
        case ItemType::CRYSTAL_GREEN:return "A verdant crystal that hums faintly.";
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
