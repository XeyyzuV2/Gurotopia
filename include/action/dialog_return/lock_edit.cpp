#include "pch.hpp"

#include "lock_edit.hpp"

void lock_edit(ENetEvent& event, const std::vector<std::string> &&pipes)
{
    auto &owner_peer = _peer[event.peer];
    auto it = worlds.find(owner_peer->recent_worlds.back());
    if (it == worlds.end()) return;
    world &world = it->second;

    // Handle adding a player to the access list
    if (pipes[4] == "playerNetID")
    {
        int netid_to_add = 0;
        try {
            netid_to_add = std::stoi(pipes[5]);
        }
        catch(const std::exception& e) { return; }

        int userid_to_add = 0;

        // Find the user ID of the player being added
        for (const auto& p : _peer) {
            if (p.second->netid == netid_to_add) {
                userid_to_add = p.second->user_id;
                break;
            }
        }
        if (userid_to_add == 0) return; // Player not found

        // Define admin limits based on lock type
        int admin_limit = 0;
        switch (world.lock_type) {
            case 1: admin_limit = 1; break; // Small Lock
            case 2: admin_limit = 3; break; // Big Lock
            case 3: admin_limit = 6; break; // Huge Lock
            default: admin_limit = 0; break;
        }

        // Count current admins
        int current_admins = 0;
        for (int admin_id : world.admin) {
            if (admin_id != 0) {
                current_admins++;
            }
        }

        if (current_admins >= admin_limit) {
            packet::create(*event.peer, false, 0, { "OnTalkBubble", owner_peer->netid, "This lock cannot accept any more users.", 0u, 1u });
            return;
        }

        // Add the new admin to an empty slot
        for (int& admin_slot : world.admin) {
            if (admin_slot == 0) {
                admin_slot = userid_to_add;
                packet::create(*event.peer, false, 0, { "OnTalkBubble", owner_peer->netid, "User has been added to the access list.", 0u, 1u });
                return;
            }
        }
    }

    if (pipes[10] == "checkbox_public" && (pipes[11] == "1" || pipes[11] == "0"))
    {
        try {
            world._public = std::stoi(pipes[11]);
        }
        catch(const std::exception& e) {} // Ignore on failure
        // @todo add public lock visuals
    }
}