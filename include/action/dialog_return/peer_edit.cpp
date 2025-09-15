#include "pch.hpp"

#include "on/NameChanged.hpp" // @note peer name changes when role updates
#include "on/SetBux.hpp" // @note update gem count @todo if changed. (currently it updates everytime peer is edited)

#include "peer_edit.hpp"

#include <stdexcept>

void peer_edit(ENetEvent& event, const std::vector<std::string> &&pipes)
{
    const std::string name = pipes[5zu];
    bool status = false;
    u_char role = 0;
    short level = 0;
    signed gems = 0;

    try {
        status = std::stoi(pipes[8zu]);
        role = std::stoi(pipes[11zu]);
        level = std::stoi(pipes[13zu]);
        gems = std::stoi(pipes[15zu]);
    }
    catch (const std::exception& e) {
        return; // Invalid input
    }

    if (status) // @note online
    {
        peers(event, PEER_ALL, [&event, name, role, level, gems](ENetPeer& p) 
        {
            if (_peer[&p]->ltoken[0] == name)
            {
                auto &_p = _peer[&p];
                _p->role = role;
                _p->level[0] = level;
                _p->gems = gems;

                on::NameChanged(event);
                on::SetBux(event);
                return;
            }
        });
    }
    else // @note offline
    {
        ::peer &offline = ::peer().read(name);
        offline.role = role;
        offline.level[0] = level;
        offline.gems = gems;
    }
}