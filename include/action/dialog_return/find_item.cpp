#include "pch.hpp"

#include "tools/string.hpp"
#include "on/SetClothing.hpp"

#include "find_item.hpp"

#include <stdexcept>

void find_item(ENetEvent& event, const std::vector<std::string> &&pipes)
{
    int item_id = 0;
    try {
        std::string id_str = readch(pipes[5zu], '_')[1]; // @note after searchableItemListButton
        item_id = std::stoi(id_str);
    }
    catch (const std::exception& e) {
        return; // Invalid input
    }
    
    _peer[event.peer]->emplace(slot(item_id, 200));
    inventory_visuals(event);
    on::SetClothing(event); // @note when a item gets added to inventory, clothing equiped resets..
}