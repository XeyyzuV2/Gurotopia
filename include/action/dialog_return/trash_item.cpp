#include "pch.hpp"

#include "trash_item.hpp"

#include <stdexcept>
#include <cmath>

void trash_item(ENetEvent& event, const std::vector<std::string> &&pipes)
{
    short id = 0;
    short count = 0;
    try {
        id = std::stoi(pipes[5zu]);
        count = std::stoi(pipes[8zu]);
    }
    catch (const std::invalid_argument& e) {
        // non-numeric input
        return;
    }
    catch (const std::out_of_range& e) {
        // number is too large
        return;
    }

    for (const slot &slot : _peer[event.peer]->slots)
        if (slot.id == id)
            if (count > slot.count) count = slot.count;
            else if (count < 0) count = 0;

    auto& peer = _peer[event.peer];

    // Calculate and award gems based on rarity
    int gems_earned = static_cast<int>(std::floor(static_cast<float>(items[id].rarity) / 2.0f)) * count;
    if (gems_earned > 0) {
        peer->gems += gems_earned;
    }

    peer->emplace(slot(id, -count)); // @note take away
    modify_item_inventory(event, {id, count});

    packet::create(*event.peer, false, 0, {
        "OnConsoleMessage",
        std::format("{} `w{}`` recycled, `w{}`` gems earned.", count, items[id].raw_name, gems_earned).c_str()
    });
}