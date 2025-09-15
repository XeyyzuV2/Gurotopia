#include "pch.hpp"

#include "drop_item.hpp"

#include <stdexcept>

void drop_item(ENetEvent& event, const std::vector<std::string> &&pipes)
{
    auto &peer = _peer[event.peer];

    short id = 0;
    short count = 0;
    try {
        id = std::stoi(pipes[5zu]);
        count = std::stoi(pipes[8zu]);
    }
    catch (const std::exception& e) {
        return; // Invalid input
    }

    for (const slot &slot : _peer[event.peer]->slots)
        if (slot.id == id)
            if (count > slot.count) count = slot.count;
            else if (count < 0) count = 0;

    peer->emplace(slot(id, -count)); // @note take away
    modify_item_inventory(event, {id, count});

    float x_nabor = (peer->facing_left ? peer->pos[0] - 1 : peer->pos[0] + 1); // @note peer's naboring tile (drop position)
    item_change_object(event, {id, count}, {x_nabor, peer->pos[1]});
}