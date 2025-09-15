#include "pch.hpp"
#include "RequestWorldSelectMenu.hpp"

void on::RequestWorldSelectMenu(ENetEvent& event) 
{
    auto &peer = _peer[event.peer];
    auto section = [&peer](const auto& range, const char* color) 
    {
        std::string result;
        for (const auto& name : range) 
            if (!name.empty()) 
            {
                auto it = worlds.find(name); // @todo reduce iteration.
                result.append((it != worlds.end()) ? 
                    std::format("add_floater|{}|{}|0.5|{}\n", name, it->second.visitors, color) :
                    std::format("add_floater|{}|0|0.5|{}\n", name, color));
            }
        return result;
    };

    // Get all worlds with players
    std::vector<std::pair<std::string, int>> popular_worlds;
    for (const auto& world : worlds) {
        if (world.second.visitors > 0) {
            popular_worlds.push_back({world.first, world.second.visitors});
        }
    }

    // Sort worlds by visitor count, descending
    std::sort(popular_worlds.begin(), popular_worlds.end(), [](const auto& a, const auto& b) {
        return a.second > b.second;
    });

    // Trim the list to the top 10
    if (popular_worlds.size() > 10) {
        popular_worlds.resize(10);
    }

    // Extract just the names to pass to the existing section lambda
    std::vector<std::string> top_10_names;
    for (const auto& world_pair : popular_worlds) {
        top_10_names.push_back(world_pair.first);
    }

    packet::create(*event.peer, false, 0, {
        "OnRequestWorldSelectMenu", 
            std::format(
                "add_filter|\n"
                "add_heading|Top Worlds<ROW2>|\n{}{}"
                "add_heading|My Worlds<CR>|\n{}"
                "add_heading|Recently Visited Worlds<CR>|\n{}",
            "add_floater|wotd_world|\u013B WOTD|0|0.5|3529161471\n", 
            section(top_10_names, "3529161471"),
            section(peer->my_worlds, "2147418367"), 
            section(peer->recent_worlds, "3417414143")
        ).c_str(), 
        0
    });
    packet::create(*event.peer, false, 0, {
        "OnConsoleMessage", 
        std::format("Where would you like to go? (`w{}`` online)", peers(event).size()).c_str()
    });
}