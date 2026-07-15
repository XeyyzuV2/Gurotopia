#include "pch.hpp"
#include "on/NameChanged.hpp"
#include "on/SetClothing.hpp"
#include "on/Action.hpp"
#include "on/ConsoleMessage.hpp"
#include "commands/weather.hpp"
#include "item_activate.hpp"
#include "tools/ransuu.hpp"
#include "tools/create_dialog.hpp"
#include "action/quit_to_exit.hpp"
#include "action/join_request.hpp"
#include "item_activate_object.hpp"

#include "items/registry.hpp"
#include "items/properties.hpp"
#include "items/collision.hpp"

#include "tile_change.hpp"

using namespace std::chrono;
using namespace std::literals::chrono_literals;

void tile_change(ENetEvent& event, state state)
{
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);
    try
    {
        auto world = std::ranges::find(worlds, pPeer->recent_worlds.back(), &::world::name);
        if (world == worlds.end()) return;

        ::block &block = world->blocks[cord(state.punch.x, state.punch.y)];

        auto item = std::ranges::find(items, (state.id != 32 && state.id != 18) ? state.id : (block.fg != 0) ? block.fg : block.bg, &::item::id);
        if (item->id == 0) return;

        // ---- Fire check (anyone can extinguish) ----
        if (block.state[3] & S_FIRE)
            if (pPeer->clothing[hand] == 3066/* fire hose */)
            {
                remove_fire(event, state, block, *world);
                return;
            }

        // ---- Access check ----
        if (!(item->cat & static_cast<u_char>(item_category::CAT_PUBLIC)))
            if ((world->owner && !world->is_public && !pPeer->role) &&
                (pPeer->user_id != world->owner && !std::ranges::contains(world->access, pPeer->user_id))) return;

        // ====================================================================
        // MODE: PUNCHING (state.id == 18)
        // ====================================================================
        if (state.id == 18)
        {
            static bool punch{};
            if (!punch)
            {
                punch = true;
                // @note Rayman's Fist multi-punch
                if (pPeer->clothing[hand] == 5480)
                {
                    ::state copy_state = state;
                    int dx = (pPeer->facing_left) ? -1 : 1;
                    int dy = (state.punch.y == state.pos.by_32(true).y) ? 0
                           : (state.punch.y < state.pos.by_32(true).y) ? -1 : 1;

                    copy_state.punch.x += dx; copy_state.punch.y += dy;
                    tile_change(event, std::move(copy_state));
                    copy_state.punch.x += dx; copy_state.punch.y += dy;
                    tile_change(event, std::move(copy_state));
                }
                punch = false;
            }

            // ---- Try item-specific behavior registry ----
            ransuu ransuu;
            u_char apply_damage_value{};
            bool registry_handled = false;

            if (auto* handler = item_registry::get_handler(item->id))
            {
                item_context ctx{
                    .pPeer = pPeer,
                    .world = &*world,
                    .block = &block,
                    .item = &*item,
                    .state = state,
                    .event = event
                };
                auto action = (*handler)(ctx);
                switch (action)
                {
                    case item_action::HANDLED:
                        registry_handled = true;
                        break;
                    case item_action::CANCEL:
                        return;
                    case item_action::CONSUME:
                        modify_item_inventory(event, ::slot(item->id, -1));
                        return;
                    default:
                        break; // NONE = continue
                }
            }

            if (registry_handled) return;

            // ---- Digger's Spade ----
            if (pPeer->clothing[hand] == 2952)
            {
                if (item->id == 2 || item->id == 14)
                {
                    if (block.fg != 0) block.hits[0] = 3;
                    else block.hits[1] = 3;

                    int color = (item->id == 2) ? ransuu[{0x02, 0x03}] : ransuu[{0x0e, 0x0f}];
                    send_particle_effect(event, {color, 0x61}, state.punch.by_32());
                }
            }

            // ---- Type-based generic behavior (fallback) ----
            switch (item->type)
            {
                case type::STRONG: throw std::runtime_error("It's too strong to break.");
                case type::MAIN_DOOR: throw std::runtime_error("(stand over and punch to use)");
                case type::LOCK:
                {
                    if (is_tile_lock(item->id)) break;
                    if (world->owner != pPeer->user_id)
                        throw std::runtime_error(std::format("`5[```w{}`` `$World Locked`` by (null)`5]``", world->name));
                    break;
                }
                case type::PROVIDER:
                {
                    if ((steady_clock::now() - block.tick) / 1s >= item->tick)
                    {
                        // @note provider items with custom behavior are handled
                        // via the item registry (init_behaviors.cpp). If unregistered,
                        // just reset timer so it produces again — no item-specific drop.
                        if (!item_registry::get_handler(item->id))
                        {
                            // Generic provider: attempt item+2 drop as default
                            add_drop(event, ::slot(item->id + 2, ransuu[{1, 2}]), state.punch.by_32(), *world);
                        }
                        block.tick = steady_clock::now();
                        send_tile_update(event, std::move(state), block, *world);
                        pPeer->add_xp(event, 1);
                        return;
                    }
                    break;
                }
                case type::SEED:
                {
                    if ((steady_clock::now() - block.tick) / 1s >= item->tick)
                    {
                        block.hits[0] = 99;
                        add_drop(event, ::slot(item->id - 1, ransuu[{2, 12}]), state.punch.by_32(), *world);
                    }
                    break;
                }
                case type::WEATHER_MACHINE:
                {
                    ::block &weather_machine = world->blocks[cord(world->weather.x, world->weather.y)];
                    if (!(block.state[2] & S_TOGGLE) && !(weather_machine.state[2] & S_TOGGLE))
                        weather_machine.state[2] &= ~S_TOGGLE;
                    block.state[2] ^= S_TOGGLE;
                    world->weather = state.punch;
                    peers(pPeer->recent_worlds.back(), PEER_SAME_WORLD, [block, item](ENetPeer& p)
                    {
                        send_varlist(&p, { "OnSetCurrentWeather", (block.state[2] & S_TOGGLE) ? get_weather_id(item->id) : 0 });
                    });
                    break;
                }
                case type::TOGGLEABLE_BLOCK:
                case type::TOGGLEABLE_ANIMATED_BLOCK:
                case type::CHEST:
                {
                    block.state[2] ^= S_TOGGLE;
                    if (item->id == 226)
                    {
                        on::ConsoleMessage(event.peer,
                            (block.state[2] & S_TOGGLE)
                                ? "Signal jammer enabled. This world is now `4hidden`` from the universe."
                                : "Signal jammer disabled.  This world is `2visible`` to the universe.");
                    }
                    break;
                }
                case type::RANDOM:
                {
                    apply_damage_value =
                        (item->id == 456) ? ransuu[{0, 5}] :
                        (item->id == 1300) ? ransuu[{1, 3}] : 0;

                    auto random = std::ranges::find(world->random_blocks, state.punch, &::random_block::pos);
                    if (random == world->random_blocks.end())
                        world->random_blocks.emplace_back(::random_block{apply_damage_value, state.punch});
                    else random->value = apply_damage_value;
                    break;
                }
            }

            // ---- Damage application ----
            tile_apply_damage(event, std::move(state), block, apply_damage_value);

            if (block.hits[0] >= item->hits) block.fg = 0, block.hits[0] = 0;
            else if (block.hits[1] >= item->hits) block.bg = 0, block.hits[1] = 0;
            else return;

            // ---- Block destruction ----
            block.label = "";
            block.state[2] = 0x00;
            block.state[3] &= ~S_VANISH;

            // @note love chests (392/3402/9350) handled via item registry now

            if (item->type == type::LOCK && !is_tile_lock(item->id))
            {
                if (!pPeer->role) { pPeer->prefix.front() = 'w'; on::NameChanged(event); }
                world->owner = 0;
            }

            // ---- Property-based break behavior ----
            if (has_category(*item, item_category::CAT_RETURN))
            {
                int uid = add_object(event, ::slot(item->id, 1), state.pos, *world);
                item_activate_object(event, ::state{.id = uid, .punch = state.punch});
            }
            else if (has_property(*item, item_property::PROP_NO_SEED))
            {
                // @note property & 04 — "This item never drops any seeds." (silent)
            }
            else // normal break: gem + seed drop + XP
            {
                if (item->type != type::SEED)
                {
                    u_char rarity_to_gem =
                        (item->rarity >= 87) ? 22 :
                        (item->rarity >= 68) ? 18 :
                        (item->rarity >= 53) ? 14 :
                        (item->rarity >= 41) ? 11 :
                        (item->rarity >= 36) ? 10 :
                        (item->rarity >= 32) ? 9 :
                        (item->rarity >= 24) ? 5 : 1;

                    if (!ransuu[{0, (rarity_to_gem > 1) ? 1 : 4}])
                    {
                        u_char gems = ransuu[{1, rarity_to_gem}];
                        for (short i : {10, 5, 1})
                            for (; gems >= i; gems -= i)
                                add_drop(event, {112, i}, state.punch.by_32(), *world);
                    }
                    if (!ransuu[{0, (rarity_to_gem > 1) ? 2 : 4}]) add_drop(event, ::slot(item->id + 1, 1), state.punch.by_32(), *world);
                    else if (!ransuu[{0, (rarity_to_gem > 1) ? 4 : 8}]) add_drop(event, ::slot(item->id, 1), state.punch.by_32(), *world);
                }
                pPeer->add_xp(event, std::trunc(1.0f + item->rarity / 5.0f));
            }
        }
        // ====================================================================
        // MODE: CONSUMABLE / CLOTHING (item held, not fist/wrench)
        // ====================================================================
        else if (item->cloth_type != clothing::none)
        {
            if (state.punch != pPeer->pos.by_32(true))
                throw std::runtime_error("To wear clothing, use on yourself");
            item_activate(event, state);
            return;
        }
        else if (item->type == type::CONSUMEABLE)
        {
            // Try registry first
            if (auto* handler = item_registry::get_handler(item->id))
            {
                item_context ctx{.pPeer = pPeer, .world = &*world, .block = &block, .item = &*item, .state = state, .event = event};
                auto action = (*handler)(ctx);
                if (action != item_action::NONE)
                {
                    if (action == item_action::CONSUME)
                        modify_item_inventory(event, ::slot(item->id, -1));
                    return;
                }
            }

            // Generic consumable handling
            if (item->raw_name.contains(" Blast"))
            {
                send_varlist(event.peer, {
                    "OnDialogRequest",
                    std::format(
                        "set_default_color|`o\n"
                        "embed_data|id|{0}\n"
                        "add_label_with_icon|big|`w{1}``|left|{0}|\n"
                        "add_label|small|This item creates a new world! Enter a unique name for it.|left\n"
                        "add_text_input|name|New World Name||24|\n"
                        "end_dialog|create_blast|Cancel|Create!|\n",
                        item->id, item->raw_name
                    )
                });
            }

            if (item->raw_name.contains("Paint Bucket - ") && pPeer->clothing[hand] != 3494)
                throw std::runtime_error("you need a Paintbrush to apply paint!");
            if (item->raw_name.contains("Hair Dye"))
            {
                if (state.punch != pPeer->pos.by_32(true)) throw std::runtime_error("Don't spill your dye!");
                else if (world->blocks[cord(pPeer->pos.by_32(true).x, pPeer->pos.by_32(true).y)].fg != 230)
                    throw std::runtime_error("You'll make a huge mess if you do that outside the Bathtub!");
                on::Action(event, "shower");
                send_varlist(event.peer, { "OnTalkBubble", pPeer->netid, "You dyed your hair!", 0u, 1u });
            }

            // @note paint buckets are now handled via registry, so this is just for non-registered items
            send_tile_update(event, std::move(state), block, *world);
            modify_item_inventory(event, ::slot(item->id, -1));
            pPeer->add_xp(event, 1);
            return;
        }
        // ====================================================================
        // MODE: WRENCHING (state.id == 32)
        // ====================================================================
        else if (state.id == 32)
        {
            switch (item->type)
            {
                case type::LOCK:
                {
                    if (is_tile_lock(item->id)) break;
                    if (pPeer->user_id == world->owner)
                    {
                        send_varlist(event.peer, {
                            "OnDialogRequest",
                            std::format(
                                "set_default_color|`o\n"
                                "add_label_with_icon|big|`wEdit {}``|left|{}|\n"
                                "add_popup_name|LockEdit|\n"
                                "add_label|small|`wAccess list:``|left\n"
                                "embed_data|tilex|{}\n"
                                "embed_data|tiley|{}\n"
                                "add_spacer|small|\n"
                                "add_label|small|Currently, you're the only one with access.``|left\n"
                                "add_spacer|small|\n"
                                "add_player_picker|playerNetID|`wAdd``|\n"
                                "add_checkbox|checkbox_public|Allow anyone to Build and Break|{}\n"
                                "add_checkbox|checkbox_disable_music|Disable Custom Music Blocks|{}\n"
                                "add_text_input|tempo|Music BPM|100|3|\n"
                                "add_checkbox|checkbox_disable_music_render|Make Custom Music Blocks invisible|0\n"
                                "add_checkbox|checkbox_set_as_home_world|Set as Home World|0|\n"
                                "add_text_input|minimum_entry_level|World Level: |{}|3|\n"
                                "add_smalltext|Set minimum world entry level.|\n"
                                "add_button|sessionlength_dialog|`wSet World Timer``|noflags|0|0|\n"
                                "add_button|changecat|`wCategory: None``|noflags|0|0|\n"
                                "add_button|getKey|Get World Key|noflags|0|0|\n"
                                "end_dialog|lock_edit|Cancel|OK|\n",
                                item->raw_name, item->id, state.punch.x, state.punch.y,
                                to_char(world->is_public),
                                (world->lock_state & DISABLE_MUSIC) ? "1" : "0",
                                world->minimum_entry_level
                            )
                        });
                    }
                    break;
                }
                case type::DOOR:
                case type::PORTAL:
                {
                    std::string dest, id;
                    for (::door& door : world->doors)
                        if (door.pos == state.punch) dest = door.dest, id = door.id;

                    send_varlist(event.peer, {
                        "OnDialogRequest",
                        std::format(
                            "set_default_color|`o\n"
                            "add_label_with_icon|big|`wEdit {}``|left|{}|\n"
                            "add_text_input|door_name|Label|{}|100|\n"
                            "add_popup_name|DoorEdit|\n"
                            "add_text_input|door_target|Destination|{}|24|\n"
                            "add_smalltext|Enter a Destination in this format: `2WORLDNAME:ID``|left|\n"
                            "add_smalltext|Leave `2WORLDNAME`` blank (:ID) to go to the door with `2ID`` in the `2Current World``.|left|\n"
                            "add_text_input|door_id|ID|{}|11|\n"
                            "add_checkbox|checkbox_locked|Is open to public|1\n"
                            "embed_data|tilex|{}\n"
                            "embed_data|tiley|{}\n"
                            "end_dialog|door_edit|Cancel|OK|",
                            item->raw_name, item->id, block.label, dest, id, state.punch.x, state.punch.y
                        )
                    });
                    break;
                }
                case type::SIGN:
                {
                    send_varlist(event.peer, {
                        "OnDialogRequest",
                        std::format(
                            "set_default_color|`o\n"
                            "add_popup_name|SignEdit|\n"
                            "add_label_with_icon|big|`wEdit {}``|left|{}|\n"
                            "add_textbox|What would you like to write on this sign?``|left|\n"
                            "add_text_input|sign_text||{}|128|\n"
                            "embed_data|tilex|{}\n"
                            "embed_data|tiley|{}\n"
                            "end_dialog|sign_edit|Cancel|OK|",
                            item->raw_name, item->id, block.label, state.punch.x, state.punch.y
                        )
                    });
                    break;
                }
                case type::ENTRANCE:
                {
                    send_varlist(event.peer, {
                        "OnDialogRequest",
                        std::format(
                            "set_default_color|`o\n"
                            "add_label_with_icon|big|`wEdit {}``|left|{}|\n"
                            "add_checkbox|checkbox_public|Is open to public|{}\n"
                            "embed_data|tilex|{}\n"
                            "embed_data|tiley|{}\n"
                            "end_dialog|gateway_edit|Cancel|OK|\n",
                            item->raw_name, item->id, to_char((block.state[2] & S_PUBLIC)), state.punch.x, state.punch.y
                        )
                    });
                    break;
                }
                case type::VENDING_MACHINE:
                {
                    send_varlist(event.peer, {
                        "OnDialogRequest",
                        ::create_dialog()
                            .set_default_color("`o")
                            .add_label_with_icon("big", std::format("`w{}``", item->raw_name), item->id)
                            .add_spacer("small")
                            .embed_data("tilex", state.punch.x)
                            .embed_data("tiley", state.punch.y)
                            .add_textbox("This machine is empty.")
                            .add_item_picker("stockitem", "`wPut an item in``", "Choose an item to put in the machine!")
                            .add_smalltext("Upgrade to a DigiVend Machine for `44,000 Gems``.")
                            .add_button("upgradedigital", "Upgrade to DigiVend")
                            .add_spacer("small")
                            .end_dialog("vending", "Close", "")
                    });
                    break;
                }
                case type::DISPLAY_BLOCK:
                    break; // @todo
            }
            return;
        }
        // ====================================================================
        // MODE: PLACING (any other held item)
        // ====================================================================
        else
        {
            if (block.fg != 0)
            {
                bool update_tile{};
                switch (items[world->blocks[cord(state.punch.x, state.punch.y)].fg].type)
                {
                    case type::DISPLAY_BLOCK:
                    {
                        world->displays.emplace_back(::display(item->id, state.punch));
                        update_tile = true;
                        break;
                    }
                    case type::SEED:
                    {
                        for (::item &i : items)
                        {
                            if ((i.splice[0] == state.id && i.splice[1] == block.fg) ||
                                (i.splice[1] == state.id && i.splice[0] == block.fg))
                            {
                                auto splice0 = std::ranges::find(items, i.splice[0], &::item::id);
                                auto splice1 = std::ranges::find(items, i.splice[1], &::item::id);
                                send_varlist(event.peer, {
                                    "OnTalkBubble", pPeer->netid,
                                    std::format("`w{}`` and `w{}`` have been spliced to make a `${} Tree``!",
                                        splice0->raw_name, splice1->raw_name, i.raw_name.substr(0, i.raw_name.length()-5)),
                                    0u, 1u
                                });
                                block.tick = steady_clock::now();
                                block.fg = i.id;
                                update_tile = true;
                                break;
                            }
                        }
                        break;
                    }
                }
                if (update_tile)
                    send_tile_update(event, std::move(state), block, *world);
                return;
            }

            // ---- Collision check (via modular system) ----
            if (!collision::can_place_at(item->collision, state.pos, state.punch))
                return;

            bool lock_visuals{}; // @note set if world lock was just placed

            // ---- Type-based placement logic ----
            switch (item->type)
            {
                case type::LOCK:
                {
                    if (is_tile_lock(item->id)) break;
                    if (!world->owner)
                    {
                        world->owner = pPeer->user_id;
                        lock_visuals = true;
                        if (!pPeer->role)
                        {
                            pPeer->prefix.front() = '2';
                            on::NameChanged(event);
                        }
                        if (std::ranges::find(pPeer->my_worlds, world->name) == pPeer->my_worlds.end())
                        {
                            std::ranges::rotate(pPeer->my_worlds, pPeer->my_worlds.begin() + 1);
                            pPeer->my_worlds.back() = world->name;
                        }
                        std::string msg = std::format("`5[```w{}`` has been `$World Locked`` by {}`5]``", world->name, pPeer->growid);
                        peers(pPeer->recent_worlds.back(), PEER_SAME_WORLD, [&](ENetPeer& peer)
                        {
                            send_varlist(&peer, { "OnTalkBubble", pPeer->netid, msg });
                            on::ConsoleMessage(&peer, msg);
                        });
                    }
                    else throw std::runtime_error("Only one `$World Lock`` can be placed in a world, you'd have to remove the other one first.");
                    break;
                }
                case type::ENTRANCE:
                    block.state[2] |= S_PUBLIC;
                    break;
                case type::PROVIDER:
                    block.tick = steady_clock::now();
                    break;
                case type::SEED:
                    block.state[2] |= 0x11;
                    block.tick = steady_clock::now();
                    break;
            }
            block.state[2] |= (pPeer->facing_left) ? S_LEFT : S_RIGHT;
            (item->type == type::BACKGROUND) ? block.bg = state.id : block.fg = state.id;
            pPeer->emplace(::slot(item->id, -1));
        }

        // ---- Finalize ----
        state.netid = pPeer->netid;
        state_visuals(*event.peer, std::move(state));
        if (lock_visuals)
        {
            state_visuals(*event.peer, ::state{
                .type = 0x0f,
                .netid = world->owner,
                .peer_state = peer_state::S_EXTENDED,
                .id = state.id,
                .punch = state.punch
            });
        }
    }
    catch (const std::exception& exc)
    {
        if (exc.what() && *exc.what())
            send_varlist(event.peer, { "OnTalkBubble", pPeer->netid, exc.what(), 0u, 1u });
    }
}
