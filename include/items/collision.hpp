#pragma once

#include "proton/Math/vec2.hpp"
#include "database/items.hpp"  // for collision enum

// @note Collision checking system.
// In the official Growtopia server, collision type determines how a tile
// interacts with player movement, not just whether a block can be placed.
//
// Usage:
//   if (!collision::can_place_at(item->collision, player_pos, target_pos))
//       return; // blocked

namespace collision_check {

// @brief Can the player place a block at target_pos?
// Returns false if collision would prevent placement.
inline bool can_place_at(::collision col, const ::pos& player_pos, const ::pos& target_pos)
{
    switch (col)
    {
        case ::collision::FULL:
            // Can't place inside yourself
            if (player_pos.by_32(true) == target_pos) return false;
            return true;

        case ::collision::NO_COLLISION:
        case ::collision::ON_TOP:       // platforms can be placed under feet
        case ::collision::IF_ACCESS:
        case ::collision::TOGGLE:
        case ::collision::IF_VIP:
        case ::collision::IF_GUILD:
        case ::collision::ADVENTURE_ITEM:
        case ::collision::ACTIVATE:
        case ::collision::BALLOON_WARZ_TEAM:
        case ::collision::STEP_ON:
            return true;

        case ::collision::HORIZONTAL:
        case ::collision::VERTICAL:
            // One-way tiles: can place freely, movement is what matters
            return true;

        default:
            return true;
    }
}

// @brief Describes the movement rule for a collision type.
// Used by the movement handler (state/movement.cpp) to decide whether
// a player can walk into/onto/through a tile.
enum class movement_rule : u_char {
    BLOCK,      // solid — cannot pass
    PASS,       // free — walk through
    PLATFORM,   // stand on top, jump through from below
    SLAB,       // half-height block
    ONE_WAY_X,  // pass from left, block from right (or vice versa)
    ONE_WAY_Y,  // pass from above, block from below
    ACCESS,     // pass only if in world access list
    VIP,        // pass only if VIP role
    GUILD,      // pass only if same guild
    ACTIVATE,   // pass triggers activation (door, portal, etc.)
};

// @brief Get the movement rule for a collision type.
inline movement_rule get_movement_rule(::collision col)
{
    switch (col)
    {
        case ::collision::NO_COLLISION:   return movement_rule::PASS;
        case ::collision::FULL:           return movement_rule::BLOCK;
        case ::collision::ON_TOP:         return movement_rule::PLATFORM;
        case ::collision::SLAB:           return movement_rule::SLAB;
        case ::collision::HORIZONTAL:     return movement_rule::ONE_WAY_X;
        case ::collision::VERTICAL:       return movement_rule::ONE_WAY_Y;
        case ::collision::IF_ACCESS:      return movement_rule::ACCESS;
        case ::collision::IF_VIP:         return movement_rule::VIP;
        case ::collision::IF_GUILD:       return movement_rule::GUILD;
        case ::collision::ACTIVATE:       return movement_rule::ACTIVATE;
        case ::collision::ADVENTURE_ITEM: return movement_rule::PASS;
        case ::collision::BALLOON_WARZ_TEAM: return movement_rule::PASS;
        case ::collision::STEP_ON:        return movement_rule::PASS;
        case ::collision::TOGGLE:
        default:                          return movement_rule::BLOCK;
    }
}

// @brief Check if a tile at col_pos is passable given the player's movement direction.
// @param col       The tile's collision type
// @param player_x  Player's current X
// @param player_y  Player's current Y
// @param target_x  Tile X being moved into
// @param target_y  Tile Y being moved into
// @param is_falling  Whether the player is falling (gravity)
// @return true if the player can move into that tile
inline bool is_passable(::collision col, float player_x, float player_y, int target_x, int target_y, bool is_falling = false)
{
    auto rule = get_movement_rule(col);
    switch (rule)
    {
        case movement_rule::PASS:
            return true;

        case movement_rule::BLOCK:
            return false;

        case movement_rule::PLATFORM:
            // Can stand on top, but jump through from below
            if (is_falling || player_y <= target_y)
                return false; // standing on it
            return true;      // jumping through from below

        case movement_rule::SLAB:
            // Half-height — pass if player is in the upper half
            return false; // @todo check if player Y is above slab midpoint

        case movement_rule::ONE_WAY_X:
            // Pass from left, block from right (based on player facing)
            return true; // @todo implement direction check

        case movement_rule::ONE_WAY_Y:
            // Pass from above only
            return player_y > target_y;

        case movement_rule::ACCESS:
        case movement_rule::VIP:
        case movement_rule::GUILD:
            // @todo check peer permissions
            return false;

        case movement_rule::ACTIVATE:
            return true; // pass through, but fire activation event

        default:
            return false;
    }
}

} // namespace collision
