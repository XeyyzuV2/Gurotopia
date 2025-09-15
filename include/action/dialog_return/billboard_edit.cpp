#include "pch.hpp"

#include "on/BillboardChange.hpp"

#include "billboard_edit.hpp"

#include <stdexcept>

void billboard_edit(ENetEvent& event, const std::vector<std::string> &&pipes)
{
    auto &peer = _peer[event.peer];

    try {
        if (pipes[4zu] == "billboard_item")
        {
            const short billboard_item = std::stoi(pipes[5zu]);
            if (billboard_item == 18 || billboard_item == 32) return;

            peer->billboard = {
                .id = billboard_item,
                .show = std::stoi(pipes[7zu]) != 0,
                .isBuying = std::stoi(pipes[9zu]) != 0,
                .price = std::stoi(pipes[11zu]),
                .perItem = std::stoi(pipes[13zu]) != 0,
            };
        }
        else // @note billboard_toggle
        {
            peer->billboard = {
                .id = peer->billboard.id,
                .show = std::stoi(pipes[5zu]) != 0,
                .isBuying = std::stoi(pipes[7zu]) != 0,
                .price = std::stoi(pipes[9zu]),
                .perItem = std::stoi(pipes[11zu]) != 0,
            };
        }
    }
    catch(const std::exception& e) {
        return; // Invalid input
    }

    on::BillboardChange(event);
}