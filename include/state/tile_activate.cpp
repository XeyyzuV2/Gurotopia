#include "pch.hpp"
#include "action/join_request.hpp"
#include "action/quit_to_exit.hpp"
#include "tile_activate.hpp"

void tile_activate(ENetEvent& event, state state)
{
    auto &peer = _peer[event.peer];
    auto w = worlds.find(peer->recent_worlds.back());
    if (w == worlds.end()) return;

    block &block = w->second.blocks[cord(state.punch[0], state.punch[1])];
    item &item = items[block.fg]; // @todo handle bg

    switch (item.type)
    {
        case type::MAIN_DOOR:
        {
            action::quit_to_exit(event, "", false);
            break;
        }
        case type::DOOR: // @todo add door-to-door with door::id
        case type::PORTAL:
        {
            bool has_dest{ false };
            for (::door &door : w->second.doors)
            {
                if (door.pos == state.punch) 
                {
                    has_dest = true;
                    if (!door.password.empty()) {
                        // Door has a password, prompt the user
                        packet::create(*event.peer, false, 0, {
                            "OnDialogRequest",
                            std::format(
                                "set_default_color|`o\n"
                                "add_label_with_icon|big|`wPassword``|left|{0}|\n"
                                "add_textbox|This door is password protected.|left|\n"
                                "add_text_input|password_input|Password||15|\n"
                                "embed_data|tilex|{1}\n"
                                "embed_data|tiley|{2}\n"
                                "end_dialog|door_password_prompt|Cancel|Enter|\n",
                                item.id, state.punch[0], state.punch[1]
                            ).c_str()
                        });
                        return; // Stop here and wait for dialog return
                    }

                    const std::string_view destination{ door.dest };

                    if (destination.starts_with(":")) {
                        // Same-world teleport via Door ID
                        const std::string target_id = std::string{destination.substr(1)};
                        bool found_target = false;
                        for (::door& target_door : w->second.doors) {
                            if (target_door.id == target_id) {
                                packet::create(*event.peer, true, 0, { "OnSetPos", std::vector<float>{static_cast<float>(target_door.pos[0] * 32), static_cast<float>(target_door.pos[1] * 32)} });
                                packet::create(*event.peer, true, 0, { "OnPlayPositioned", "audio/teleport.wav" });
                                found_target = true;
                                break;
                            }
                        }
                        if (!found_target) {
                             packet::create(*event.peer, false, 0, { "OnTalkBubble", peer->netid, "Could not find a door with that ID.", 0u, 1u });
                        }
                    }
                    else {
                        // Inter-world teleport
                        action::quit_to_exit(event, "", true);
                        action::join_request(event, "", destination);
                    }
                    break;
                }
            }
            if (!has_dest)
            {
                packet::create(*event.peer, true, 0, {
                    "OnSetPos", 
                    std::vector<float>{peer->rest_pos.front(), peer->rest_pos.back()}
                });
                packet::create(*event.peer, false, 0, {
                    "OnZoomCamera", 
                    std::vector<float>{10000.0f}, // @todo
                    1000u
                });
                packet::create(*event.peer, true, 0, { "OnSetFreezeState", 0u });
                packet::create(*event.peer, true, 0, { "OnPlayPositioned", "audio/teleport.wav" });
            }
            break;
        }
    }

    state_visuals(event, std::move(state)); // finished.
}