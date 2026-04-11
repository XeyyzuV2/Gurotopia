#include "pch.hpp"

#include "gateway_edit.hpp"

void gateway_edit(ENetEvent& event, const ::hPipe &hPipe)
{
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);

    const short tilex = atoi(hPipe["tilex"].c_str());
    const short tiley = atoi(hPipe["tiley"].c_str());

    auto world = std::ranges::find(worlds, pPeer->recent_worlds.back(), &::world::name);
    if (world == worlds.end()) return;

    block &block = world->blocks[cord(tilex, tiley)];

    if (hPipe["dialog_name"] == "sign_edit") 
        block.label = hPipe["sign_text"];
    
    if (hPipe["dialog_name"] == "door_edit") 
        block.label = hPipe["door_name"];
        
    else if (hPipe["dialog_name"] == "gateway_edit") 
    {
        block.state[2] &= ~(S_PUBLIC | S_LOCKED);
        block.state[2] |= stoi(hPipe["checkbox_public"]) ? S_PUBLIC : S_LOCKED;
    }

    send_tile_update(event, {
        .id = block.fg,
        .punch = { tilex, tiley }
    }, block, *world);

    if (!hPipe["dialog_name"].empty())
    {
        for (::door &door : world->doors)
        {
            if (door.pos == ::pos{tilex, tiley}) 
            {
                door.dest = hPipe["door_target"];
                door.id = hPipe["door_id"];
                return;
            }
        }
        world->doors.emplace_back(::door(
            hPipe["door_target"],
            hPipe["door_id"],
            "", // @todo add password door
            { tilex, tiley }
        ));
    }
}