#include "pch.hpp"
#include "tools/string.hpp"
#include "trash.hpp"

#include <cmath>

void action::trash(ENetEvent& event, const std::string& header)
{
    std::string itemID = readch(header, '|')[4];
    
    item &item = items[atoi(itemID.c_str())];

    if (item.cat == 0x80)
    {
        packet::create(*event.peer, false, 0, { "OnTextOverlay", "You'd be sorry if you lost that!" });
        return;
    }

    for (const slot &slot : _peer[event.peer]->slots)
        if (slot.id == item.id)
        {
            int gems_per_item = static_cast<int>(std::floor(static_cast<float>(item.rarity) / 2.0f));

            packet::create(*event.peer, false, 0, {
                "OnDialogRequest",
                std::format(
                    "set_default_color|`o\n"
                    "add_label_with_icon|big|`4Recycle`` `w{0}``|left|{1}|\n"
                    "add_label|small|You will get {3} gems per item.|left|\n"
                    "add_textbox|How many to `4destroy``? (you have {2})|left|\n"
                    "add_text_input|count||0|5|\n"
                    "embed_data|itemID|{1}\n"
                    "end_dialog|trash_item|Cancel|OK|\n",
                    item.raw_name, slot.id, slot.count, gems_per_item
                ).c_str()
            });
            return;
        }
}