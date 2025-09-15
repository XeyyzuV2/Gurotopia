#include "pch.hpp"
#include "on/SetClothing.hpp"
#include "tools/string.hpp"
#include "setSkin.hpp"

void action::setSkin(ENetEvent& event, const std::string& header)
{
    std::vector<std::string> pipes = readch(header, '|');

    try {
        _peer[event.peer]->skin_color = std::stoul(pipes[3zu]);
    }
    catch (const std::exception& e) {
        // Invalid input from client (not a number). Ignore to prevent crash.
        return;
    }

    on::SetClothing(event);
}