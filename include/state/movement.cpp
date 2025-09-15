#include "pch.hpp"
#include "movement.hpp"

#include <cmath>

void movement(ENetEvent& event, state state) 
{
    auto &peer = _peer[event.peer];

    // anti-speedhack/teleport validation
    if (peer->pos[0] != 0 && peer->pos[1] != 0) // ignore check if player just spawned/not initialized
    {
        float old_x_px = peer->pos[0] * 32.0f;
        float old_y_px = peer->pos[1] * 32.0f;

        float dx = state.pos[0] - old_x_px;
        float dy = state.pos[1] - old_y_px;

        // Using squared distance to avoid sqrt(). 100*100 = 10000.
        // This prevents a player from moving more than 100 pixels (~3 tiles) in a single update.
        if ((dx * dx + dy * dy) > 10000.0f) {
            return; // Invalid movement, just ignore the packet. This will cause the client to rubber-band.
        }
    }
    
    peer->pos = { state.pos[0] / 32.0f, state.pos[1] / 32.0f };
    peer->facing_left = state.peer_state & 0x10;
    state.netid = peer->netid; // @todo
    
    state_visuals(event, std::move(state)); // finished.
}