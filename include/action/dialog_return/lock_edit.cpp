#include "pch.hpp"

#include "lock_edit.hpp"

void lock_edit(ENetEvent& event, const ::hPipe &hPipe)
{
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);

    auto world = std::ranges::find(worlds, pPeer->recent_worlds.back(), &::world::name);
    if (world == worlds.end()) return;

    ::pos pos{};
    pos.x = atoi(hPipe["tilex"].c_str());
    pos.y = atoi(hPipe["tiley"].c_str());
    
    world->is_public = atoi(hPipe["checkbox_public"].c_str());
    if (atoi(hPipe["checkbox_disable_music"].c_str()) != 0)
        world->lock_state |= DISABLE_MUSIC;
    else world->lock_state &= ~DISABLE_MUSIC;
    world->minimum_entry_level = atoi(hPipe["minimum_entry_level"].c_str());

    ::block &block = world->blocks[cord(pos.x, pos.y)];

    if (world->is_public) 
    {
        packet::create(*event.peer, false, 0, {
            "OnConsoleMessage",
            std::format("`2{}`` has set the `$World Lock`` to `$PUBLIC", pPeer->ltoken[0]).c_str()
        });
        block.state[2] |= S_PUBLIC;
    }
    else {
        packet::create(*event.peer, false, 0, {
            "OnConsoleMessage",
            std::format("`2{}`` has set the `$World Lock`` to `4PRIVATE``", pPeer->ltoken[0]).c_str()
        });
        block.state[2] &= ~S_PUBLIC;
    }

    send_tile_update(event, {
        .id = block.fg,
        .punch = pos
    }, block, *world);
}