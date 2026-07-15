#pragma once

#include <functional>
#include <unordered_map>
#include <string>
#include "items/properties.hpp"

class ENetPeer;
struct block;
struct state;
class world;
struct item;

// @brief Context passed to every item behavior handler.
struct item_context {
    class peer* pPeer;
    class world* world;
    class block* block;
    const class item* item;
    struct state state;
    ENetEvent& event;
};

// @brief Result returned by an item behavior handler.
// Tells the caller what to do next.
enum class item_action : u_char {
    NONE,           // no special action; continue with default pipeline
    HANDLED,        // behavior handled; skip remaining defaults
    BREAK,          // break the block (fg or bg)
    RETURN_ITEM,    // return item to backpack (for CAT_RETURN items)
    CONSUME,        // item was consumed; remove 1 from inventory
    CANCEL,         // abort; do nothing
};

// @brief Signature for an item behavior callback.
using item_behavior_fn = std::function<item_action(item_context& ctx)>;

// @brief Behavior registry — maps item IDs to their custom handlers.
// Items without a registered handler fall through to the generic
// property/type-based pipeline.
class item_registry {
public:
    // @brief Register a callback for a specific item ID.
    // @param item_id  The Growtopia item ID (e.g. 758 for Roulette Wheel)
    // @param fn       The behavior handler
    // @param name     Optional human-readable name for debugging
    static void register_item(u_short item_id, item_behavior_fn fn, const std::string& name = "");

    // @brief Get the handler for an item, or nullptr if none registered.
    static const item_behavior_fn* get_handler(u_short item_id);

    // @brief Get the registered name for debugging, or empty string
    static const std::string& get_name(u_short item_id);

    // @brief Remove all handlers (useful for reload)
    static void clear();

private:
    struct entry {
        item_behavior_fn fn;
        std::string name;
    };
    static std::unordered_map<u_short, entry>& table();
};
