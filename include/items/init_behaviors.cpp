#include "pch.hpp"
#include "items/init_behaviors.hpp"
#include "items/registry.hpp"
#include "items/properties.hpp"
#include "tools/ransuu.hpp"
#include "tools/create_dialog.hpp"
#include "on/ConsoleMessage.hpp"
#include "on/Action.hpp"
#include "on/NameChanged.hpp"
#include "on/SetClothing.hpp"
#include "action/join_request.hpp"
#include "action/quit_to_exit.hpp"
#include "commands/weather.hpp"

using namespace std::chrono;
using namespace std::literals::chrono_literals;

// @note convenience: add_drop helper from world.cpp (declared in world.hpp)
extern void add_drop(ENetEvent &event, ::slot im, ::pos pos, ::world &world);
extern void remove_fire(ENetEvent &event, state state, block &block, world& world);
extern void send_tile_update(ENetEvent &event, state s, block &b, world& w);
extern void send_particle_effect(ENetEvent &event, const ::pos& pos, ::pos speed);
extern void item_activate_object(ENetEvent& event, ::state state);
extern u_short modify_item_inventory(ENetEvent& event, ::slot slot);
extern int  add_object(ENetEvent& event, ::slot slot, const ::pos& pos, ::world &world);
extern int  get_weather_id(int id);

// ============================================================================
// ITEM BEHAVIOR HANDLERS
// ============================================================================
// Each handler receives the item_context and returns item_action.
// - HANDLED: skip default pipeline
// - BREAK: break the block (fg or bg)
// - NONE: continue with generic property/type-based handling
// ============================================================================

namespace {

// @brief 758 — Roulette Wheel
item_action on_roulette_wheel(item_context& ctx)
{
    auto& ransuu = *(::ransuu*)nullptr; // will be replaced with actual instance
    // @note static ransuu would be ideal, but the codebase uses stack-allocated ones
    ::ransuu rng;
    const u_char number = rng[{0, 36}];
    const char color = (number == 0) ? '2' : (rng[{0, 3}] < 2) ? 'b' : '4';
    const std::string message = std::format(
        "[`{}{}`` spun the wheel and got `{}{}``!]",
        ctx.pPeer->prefix, ctx.pPeer->growid, color, number
    );
    peers(ctx.pPeer->recent_worlds.back(), PEER_SAME_WORLD, [&](ENetPeer& p)
    {
        send_varlist(&p, { "OnTalkBubble", ctx.pPeer->netid, message }, -1, 2000);
        on::ConsoleMessage(&p, message, 2000);
    });
    return item_action::HANDLED;
}

// @note 1404 — Door Mover
item_action on_door_mover(item_context& ctx)
{
    extern bool door_mover(world &world, const ::pos &pos);
    if (!door_mover(*ctx.world, ctx.state.punch))
    {
        send_varlist(ctx.event.peer, { "OnTalkBubble", ctx.pPeer->netid, "There's no room to put the door there! You need 2 empty spaces vertically.", 0u, 1u });
        return item_action::CANCEL;
    }
    std::string remember_name = ctx.world->name;
    peers(ctx.pPeer->recent_worlds.back(), PEER_SAME_WORLD, [&](ENetPeer& p)
    {
        action::quit_to_exit({.peer = &p}, "", true);
        action::join_request({.peer = &p}, "", remember_name);
    });
    return item_action::CONSUME;
}

// @note 822 — Water Bucket
item_action on_water_bucket(item_context& ctx)
{
    if (ctx.block->state[3] & S_FIRE)
        remove_fire(ctx.event, ctx.state, *ctx.block, *ctx.world);
    else
        ctx.block->state[3] ^= S_WATER;
    send_tile_update(ctx.event, std::move(ctx.state), *ctx.block, *ctx.world);
    return item_action::CONSUME;
}

// @note 1866 — Block Glue
item_action on_block_glue(item_context& ctx)
{
    ctx.block->state[3] ^= S_GLUE;
    send_tile_update(ctx.event, std::move(ctx.state), *ctx.block, *ctx.world);
    return item_action::CONSUME;
}

// @note 3062 — Pocket Lighter
item_action on_pocket_lighter(item_context& ctx)
{
    if (ctx.block->fg == 0 && ctx.block->bg == 0)
    {
        send_varlist(ctx.event.peer, { "OnTalkBubble", ctx.pPeer->netid, "There's nothing to burn!", 0u, 1u });
        return item_action::CANCEL;
    }
    if (ctx.block->state[3] & (S_FIRE | S_WATER)) return item_action::HANDLED;

    ctx.block->state[3] |= S_FIRE;

    std::string message = "`7[```4MWAHAHAHA!! FIRE FIRE FIRE```7]``";
    peers(ctx.pPeer->recent_worlds.back(), PEER_SAME_WORLD, [&](ENetPeer& p)
    {
        send_varlist(&p, { "OnTalkBubble", ctx.pPeer->netid, message, 0u });
        on::ConsoleMessage(&p, message);
    });

    if (ctx.block->fg == 3090) // @note Highly Combustible Box
        ctx.block->fg = 3128;   // @note Combusted Box

    send_particle_effect(ctx.event, {0x96, 0x96}, ctx.state.punch.by_32());
    send_tile_update(ctx.event, std::move(ctx.state), *ctx.block, *ctx.world);
    return item_action::CONSUME;
}

// @note 3400 — Love Potion #8
item_action on_love_potion(item_context& ctx)
{
    if (ctx.block->fg != 10) return item_action::CANCEL; // @note Rock check
    ctx.block->fg = 392; // @note Heartstone
    send_particle_effect(ctx.event, {0x00, 0x2c}, ctx.state.punch.by_32());
    send_tile_update(ctx.event, std::move(ctx.state), *ctx.block, *ctx.world);
    return item_action::CONSUME;
}

// @note 1488 — Experience Potion
item_action on_exp_potion(item_context& ctx)
{
    send_varlist(ctx.event.peer, { "OnTalkBubble", ctx.pPeer->netid, "`#GULP! You got smarter!", 0u });
    ctx.pPeer->add_xp(ctx.event, 10000);
    return item_action::CONSUME;
}

// @note 2480 — Megaphone
item_action on_megaphone(item_context& ctx)
{
    send_varlist(ctx.event.peer, {
        "OnDialogRequest",
        ::create_dialog()
            .set_default_color("`o")
            .add_label_with_icon("big", "`wMegaphone``", ctx.item->id)
            .add_textbox("Enter a message you want to broadcast to every player in Growtopia! This will use up 1 Megaphone")
            .add_text_input("message", "", "", 128)
            .end_dialog("megaphone", "Nevermind", "Broadcast")
    });
    // @note actual broadcast happens in dialog_return/megaphone.cpp — we just opened the dialog
    return item_action::HANDLED;
}

// @note 408 — Duct Tape
item_action on_duct_tape(item_context& ctx)
{
    peers(ctx.pPeer->recent_worlds.back(), PEER_SAME_WORLD, [&](ENetPeer& p)
    {
        ::peer* _p = static_cast<::peer*>(p.data);
        if (ctx.state.punch == _p->pos.by_32(true))
        {
            _p->state |= S_DUCT_TAPE;
            on::SetClothing(p);
        }
    });
    return item_action::CONSUME;
}

// @note 3404/3406 — Lollipops
item_action on_lollipop(item_context& ctx)
{
    send_varlist(ctx.event.peer, { "OnTalkBubble", ctx.pPeer->netid, "`#YUM!:D", 0u });
    return item_action::CONSUME;
}

// @note Paint Bucket helpers
item_action on_paint_red(item_context& ctx)   { ctx.block->state[3] |= S_RED;    send_particle_effect(ctx.event, {bgra::RED, 0xa8}, ctx.state.punch.by_32()); send_tile_update(ctx.event, std::move(ctx.state), *ctx.block, *ctx.world); return item_action::CONSUME; }
item_action on_paint_yellow(item_context& ctx){ ctx.block->state[3] |= S_YELLOW; send_particle_effect(ctx.event, {bgra::RED | bgra::GREEN, 0xa8}, ctx.state.punch.by_32()); send_tile_update(ctx.event, std::move(ctx.state), *ctx.block, *ctx.world); return item_action::CONSUME; }
item_action on_paint_green(item_context& ctx) { ctx.block->state[3] |= S_GREEN;  send_particle_effect(ctx.event, {bgra::GREEN, 0xa8}, ctx.state.punch.by_32()); send_tile_update(ctx.event, std::move(ctx.state), *ctx.block, *ctx.world); return item_action::CONSUME; }
item_action on_paint_aqua(item_context& ctx)  { ctx.block->state[3] |= S_AQUA;   send_particle_effect(ctx.event, {bgra::BLUE | bgra::GREEN, 0xa8}, ctx.state.punch.by_32()); send_tile_update(ctx.event, std::move(ctx.state), *ctx.block, *ctx.world); return item_action::CONSUME; }
item_action on_paint_blue(item_context& ctx)  { ctx.block->state[3] |= S_BLUE;   send_particle_effect(ctx.event, {bgra::BLUE, 0xa8}, ctx.state.punch.by_32()); send_tile_update(ctx.event, std::move(ctx.state), *ctx.block, *ctx.world); return item_action::CONSUME; }
item_action on_paint_purple(item_context& ctx){ ctx.block->state[3] |= S_PURPLE; send_particle_effect(ctx.event, {bgra::BLUE | bgra::RED, 0xa8}, ctx.state.punch.by_32()); send_tile_update(ctx.event, std::move(ctx.state), *ctx.block, *ctx.world); return item_action::CONSUME; }
item_action on_paint_charcoal(item_context& ctx){ ctx.block->state[3] |= S_CHARCOAL; send_particle_effect(ctx.event, {bgra::BLUE | bgra::GREEN | bgra::RED | bgra::ALPHA, 0xa8}, ctx.state.punch.by_32()); send_tile_update(ctx.event, std::move(ctx.state), *ctx.block, *ctx.world); return item_action::CONSUME; }
item_action on_paint_vanish(item_context& ctx){ ctx.block->state[3] &= ~S_VANISH; send_particle_effect(ctx.event, {bgra::BLUE | bgra::GREEN | bgra::RED, 0xa8}, ctx.state.punch.by_32()); send_tile_update(ctx.event, std::move(ctx.state), *ctx.block, *ctx.world); return item_action::CONSUME; }

// @note ATM (1008) and other provider items handled through generic PROVIDER logic,
// but we register the ATM-specific gem drop here.
item_action on_atm(item_context& ctx)
{
    ::ransuu rng;
    u_char gems = rng[{1, 100}];
    for (short i : {100, 50, 10, 5, 1})
        for (; gems >= i; gems -= i)
            add_drop(ctx.event, {112, i}, ctx.state.punch.by_32(), *ctx.world);
    return item_action::HANDLED;
}

// @note Provider items with generic item+2 drops (chicken, cow, coffee maker, sheep)
item_action on_provider_drop_next(item_context& ctx)
{
    ::ransuu rng;
    add_drop(ctx.event, ::slot(ctx.item->id + 2, rng[{1, 2}]), ctx.state.punch.by_32(), *ctx.world);
    ctx.block->tick = steady_clock::now();
    send_tile_update(ctx.event, std::move(ctx.state), *ctx.block, *ctx.world);
    ctx.pPeer->add_xp(ctx.event, 1);
    return item_action::HANDLED;
}

// @note 5116 — Tea set (drops item-2 instead of item+2)
item_action on_provider_tea_set(item_context& ctx)
{
    ::ransuu rng;
    add_drop(ctx.event, ::slot(ctx.item->id - 2, rng[{1, 2}]), ctx.state.punch.by_32(), *ctx.world);
    ctx.block->tick = steady_clock::now();
    send_tile_update(ctx.event, std::move(ctx.state), *ctx.block, *ctx.world);
    ctx.pPeer->add_xp(ctx.event, 1);
    return item_action::HANDLED;
}

// @note 2798 — Well (drops water bucket)
item_action on_provider_well(item_context& ctx)
{
    ::ransuu rng;
    add_drop(ctx.event, ::slot(822, rng[{1, 2}]), ctx.state.punch.by_32(), *ctx.world);
    ctx.block->tick = steady_clock::now();
    send_tile_update(ctx.event, std::move(ctx.state), *ctx.block, *ctx.world);
    ctx.pPeer->add_xp(ctx.event, 1);
    return item_action::HANDLED;
}

// @note 928 — Science Station (drops chemicals)
item_action on_provider_science(item_context& ctx)
{
    ::ransuu rng;
    short chemical =
        (!rng[{0, 16}]) ? 918 : // P
        (!rng[{0, 8}])  ? 920 : // B
        (!rng[{0, 6}])  ? 924 : // Y
        (!rng[{0, 4}])  ? 916 : // R
        914;                      // G
    add_drop(ctx.event, {chemical, 1}, ctx.state.punch.by_32(), *ctx.world);
    ctx.block->tick = steady_clock::now();
    send_tile_update(ctx.event, std::move(ctx.state), *ctx.block, *ctx.world);
    ctx.pPeer->add_xp(ctx.event, 1);
    return item_action::HANDLED;
}

// @note 456 — Dice (random block)
item_action on_dice(item_context& ctx)
{
    ::ransuu rng;
    u_char value = rng[{0, 5}];
    auto random = std::ranges::find(ctx.world->random_blocks, ctx.state.punch, &::random_block::pos);
    if (random == ctx.world->random_blocks.end())
        ctx.world->random_blocks.emplace_back(::random_block{value, ctx.state.punch});
    else
        random->value = value;

    // Apply damage value to visually change the block
    ctx.state.type = (value << 24) | 0x000008;
    ctx.state.id = 6;
    ctx.state.netid = ctx.pPeer->netid;
    state_visuals(*ctx.event.peer, std::move(ctx.state));
    return item_action::HANDLED;
}

// @note 1300 — Roshambo (rock paper scissors)
item_action on_roshambo(item_context& ctx)
{
    ::ransuu rng;
    u_char value = rng[{1, 3}];
    auto random = std::ranges::find(ctx.world->random_blocks, ctx.state.punch, &::random_block::pos);
    if (random == ctx.world->random_blocks.end())
        ctx.world->random_blocks.emplace_back(::random_block{value, ctx.state.punch});
    else
        random->value = value;

    ctx.state.type = (value << 24) | 0x000008;
    ctx.state.id = 6;
    ctx.state.netid = ctx.pPeer->netid;
    state_visuals(*ctx.event.peer, std::move(ctx.state));
    return item_action::HANDLED;
}

// @note 392 (Heartstone), 3402 (GBC), 9350 (Super GBC) — love chest drops
item_action on_love_chest(item_context& ctx)
{
    ::ransuu rng;
    short reward =
        (!rng[{0, 99}]) ? 1458 :  // GHC
        (!rng[{0, 20}]) ? 362 :   // Angel Wings
        (!rng[{0, 8}])  ? 366 :   // Heartbow
        (!rng[{0, 8}])  ? 1470 :  // Ruby Necklace
        (!rng[{0, 20}]) ? 2384 :  // Love Bug
        (!rng[{0, 4}])  ? 2396 :  // Valensign
        (!rng[{0, 10}]) ? 3388 :  // Heartbreaker Hammer
        (!rng[{0, 10}]) ? 2390 :  // Teeny Angel Wings
        (!rng[{0, 10}]) ? 3396 :  // Lovebird Pendant
        (!rng[{0, 2}])  ? 3404 :  // Sour Lollipop
        (!rng[{0, 4}])  ? 3406 :  // Sweet Lollipop
        (!rng[{0, 2}])  ? 3408 :  // Pink Marble Arch
        388;                        // Perfume

    add_drop(ctx.event, ::slot(reward, (reward == 3408 || reward == 3404) ? 10 : 1), ctx.state.punch.by_32(), *ctx.world);
    if (reward == 1458)
    {
        std::string message = std::format("msg|`4The Power of Love! `2{} found a `#Golden Heart Crystal`2 in a `#{}`2!", ctx.pPeer->growid, ctx.item->raw_name);
        peers(ctx.pPeer->recent_worlds.back(), PEER_ALL, [message](ENetPeer &p) { send_action(p, "log", message.c_str()); });
    }
    if (++ctx.pPeer->gbc_pity % 100 == 0) modify_item_inventory(ctx.event, ::slot{9350, 1});
    return item_action::HANDLED;
}

} // anonymous namespace

// ============================================================================
// PUBLIC API — call once at startup
// ============================================================================
void register_item_behaviors()
{
    // --- Roulette / Chance items ---
    item_registry::register_item(758, on_roulette_wheel, "Roulette Wheel");

    // --- Tools / Utility ---
    item_registry::register_item(1404, on_door_mover, "Door Mover");
    item_registry::register_item(822,  on_water_bucket, "Water Bucket");
    item_registry::register_item(1866, on_block_glue, "Block Glue");
    item_registry::register_item(3062, on_pocket_lighter, "Pocket Lighter");
    item_registry::register_item(2480, on_megaphone, "Megaphone");
    item_registry::register_item(408,  on_duct_tape, "Duct Tape");

    // --- Consumables ---
    item_registry::register_item(3400, on_love_potion, "Love Potion #8");
    item_registry::register_item(1488, on_exp_potion, "Experience Potion");
    item_registry::register_item(3404, on_lollipop, "Sour Lollipop");
    item_registry::register_item(3406, on_lollipop, "Sweet Lollipop");

    // --- Paint Buckets ---
    item_registry::register_item(3478, on_paint_red,      "Paint Bucket - Red");
    item_registry::register_item(3480, on_paint_yellow,   "Paint Bucket - Yellow");
    item_registry::register_item(3482, on_paint_green,    "Paint Bucket - Green");
    item_registry::register_item(3484, on_paint_aqua,     "Paint Bucket - Aqua");
    item_registry::register_item(3486, on_paint_blue,     "Paint Bucket - Blue");
    item_registry::register_item(3488, on_paint_purple,   "Paint Bucket - Purple");
    item_registry::register_item(3490, on_paint_charcoal, "Paint Bucket - Charcoal");
    item_registry::register_item(3492, on_paint_vanish,   "Paint Bucket - Vanish");

    // --- Provider specials ---
    item_registry::register_item(1008, on_atm, "ATM");
    item_registry::register_item(872,  on_provider_drop_next, "Chicken");
    item_registry::register_item(866,  on_provider_drop_next, "Cow");
    item_registry::register_item(1632, on_provider_drop_next, "Coffee Maker");
    item_registry::register_item(3888, on_provider_drop_next, "Sheep");
    item_registry::register_item(5116, on_provider_tea_set, "Tea Set");
    item_registry::register_item(2798, on_provider_well, "Well");
    item_registry::register_item(928,  on_provider_science, "Science Station");

    // --- Random / Chance blocks ---
    item_registry::register_item(456,  on_dice, "Dice");
    item_registry::register_item(1300, on_roshambo, "Roshambo");

    // --- Love Chests ---
    item_registry::register_item(392,  on_love_chest, "Heartstone");
    item_registry::register_item(3402, on_love_chest, "GBC");
    item_registry::register_item(9350, on_love_chest, "Super GBC");

    // @note: Add more items here. One line each, no core file edits needed.
}
