#include "pch.hpp"
#include "door_password_prompt.hpp"
#include "action/join_request.hpp"
#include "action/quit_to_exit.hpp"
#include "database/world.hpp"

#include <stdexcept>

void door_password_prompt(ENetEvent& event, const std::vector<std::string> &&pipes)
{
    auto& peer = _peer[event.peer];
    auto it = worlds.find(peer->recent_worlds.back());
    if (it == worlds.end()) return;

    int tilex = 0, tiley = 0;
    std::string entered_password;

    try {
        for (size_t i = 4; i < pipes.size(); ++i) {
            if (pipes[i] == "tilex") {
                tilex = std::stoi(pipes[i + 1]);
            } else if (pipes[i] == "tiley") {
                tiley = std::stoi(pipes[i + 1]);
            } else if (pipes[i] == "password_input") {
                entered_password = pipes[i + 1];
            }
        }
    } catch (const std::exception&) {
        return; // Failed to parse dialog data
    }

    // Find the door
    for (::door& door : it->second.doors) {
        if (door.pos[0] == tilex && door.pos[1] == tiley) {
            // Check password
            if (door.password == entered_password) {
                // Correct password, teleport player
                const std::string_view world_name{ door.dest };
                action::quit_to_exit(event, "", true);
                action::join_request(event, "", world_name);
            } else {
                // Incorrect password
                packet::create(*event.peer, false, 0, { "OnTalkBubble", peer->netid, "Incorrect password!", 0u, 1u });
            }
            return;
        }
    }
}
