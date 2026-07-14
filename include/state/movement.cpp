#include "pch.hpp"
#include "action/respawn.hpp"

#include "items/collision.hpp"

#include "movement.hpp"

void movement(ENetEvent& event, state state)
{
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);

    // ---- Collision checking — validate movement against world tiles ----
    if (!pPeer->recent_worlds.empty())
    {
        auto world = std::ranges::find(worlds, pPeer->recent_worlds.back(), &::world::name);
        if (world != worlds.end())
        {
            // Check the tile the player is moving INTO (punch.x/y = target in tank packet)
            int target_x = state.punch.x_int();
            int target_y = state.punch.y_int();

            // Bounds check
            if (target_x >= 0 && target_x < 100 && target_y >= 0 && target_y < 60)
            {
                int fg_id = world->blocks[cord(target_x, target_y)].fg;
                if (fg_id != 0 && fg_id < items.size())
                {
                    auto& item = items[fg_id];
                    collision::movement_rule rule = collision::get_movement_rule(
                        static_cast<::collision>(item.collision)
                    );

                    // Solid blocks block movement entirely
                    if (rule == collision::movement_rule::BLOCK)
                    {
                        // @note block movement — client already handles most of this,
                        // but we reject the position update to prevent speedhack teleport
                        // through walls. Only update facing direction, not position.
                        pPeer->facing_left = state.peer_state & peer_state::S_MOVE_LEFT;
                        state.netid = pPeer->netid;
                        state_visuals(*event.peer, std::move(state));
                        return;
                    }

                    // Platform check — can't fall through platforms
                    if (rule == collision::movement_rule::PLATFORM)
                    {
                        // If player is above the platform and moving down, block it
                        bool is_falling = !(state.peer_state & peer_state::S_JUMP) &&
                                          (state.speed.y > 0 || state.peer_state & peer_state::S_LAVA_HIT);
                        if (is_falling && pPeer->pos.y <= target_y * 32.0f)
                        {
                            // Check if player was above — block falling through
                            if (pPeer->pos.by_32(true).y < target_y)
                            {
                                // Lock to top of platform
                                state.pos.y = target_y * 32.0f;
                            }
                        }
                    }
                }
            }
        }
    }

    // ---- Core state update ----
    pPeer->pos = state.pos;
    pPeer->facing_left = state.peer_state & peer_state::S_MOVE_LEFT;

    /* add fireproof only take away 1 hp instead of 2 */
    if (state.peer_state & peer_state::S_LAVA_HIT) pPeer->pain_hp -= 2;
    if (pPeer->pain_hp <= 0) action::respawn(event, ""), pPeer->pain_hp = 10;

    state.netid = pPeer->netid;
    state_visuals(*event.peer, std::move(state)); // finished.
}
