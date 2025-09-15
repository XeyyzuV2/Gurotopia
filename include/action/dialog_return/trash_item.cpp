#include "pch.hpp"

#include "trash_item.hpp"

void trash_item(ENetEvent& event, const std::vector<std::string> &&pipes)
{
    const short id = atoi(pipes[5zu].c_str());
    short count = atoi(pipes[8zu].c_str());

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