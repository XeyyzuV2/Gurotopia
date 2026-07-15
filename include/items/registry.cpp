#include "pch.hpp"
#include "items/registry.hpp"

std::unordered_map<u_short, item_registry::entry>& item_registry::table()
{
    static std::unordered_map<u_short, entry> t;
    return t;
}

void item_registry::register_item(u_short item_id, item_behavior_fn fn, const std::string& name)
{
    table()[item_id] = entry{std::move(fn), name};
}

const item_behavior_fn* item_registry::get_handler(u_short item_id)
{
    auto it = table().find(item_id);
    if (it != table().end())
        return &it->second.fn;
    return nullptr;
}

const std::string& item_registry::get_name(u_short item_id)
{
    static const std::string empty;
    auto it = table().find(item_id);
    if (it != table().end())
        return it->second.name;
    return empty;
}

void item_registry::clear()
{
    table().clear();
}
