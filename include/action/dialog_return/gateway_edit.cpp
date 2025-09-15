#include "pch.hpp"

#include "gateway_edit.hpp"

#include <stdexcept>

void gateway_edit(ENetEvent& event, const std::vector<std::string> &&pipes)
{
    short tilex = 0;
    short tiley = 0;

    try {
        tilex = std::stoi(pipes[5zu]);
        tiley = std::stoi(pipes[8zu]);
    }
    catch (const std::exception& e) {
        return; // Invalid input
    }

    auto it = worlds.find(_peer[event.peer]->recent_worlds.back());
    if (it == worlds.end()) return;

    block &block = it->second.blocks[cord(tilex, tiley)];

    if (pipes[3zu] == "door_edit" || pipes[3zu] == "sign_edit") {
        block.label = pipes[11zu];
    }
    else if (pipes[3zu] == "gateway_edit") {
        try {
            block._public = std::stoi(pipes[11zu]);
        }
        catch(const std::exception& e) {} // On failure, do nothing.
    }

    tile_update(event, {
        .id = block.fg,
        .pos = { tilex * 32.0f, tiley * 32.0f },
        .punch = { tilex, tiley }
    }, block, it->second);

    if (pipes[10zu] == "door_name" && pipes.size() > 12zu)
    {
        for (::door& door : it->second.doors)
        {
            if (door.pos == std::array<int, 2ULL>{ tilex, tiley }) 
            {
                door.dest = pipes[13];
                door.id = pipes[15];
                door.password = pipes[17];
                return;
            }
        }
        it->second.doors.emplace_back(door(
            pipes[13],
            pipes[15],
            "", // @todo add password door
            { tilex, tiley }
        ));
    }
}