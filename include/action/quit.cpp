#include "pch.hpp"
#include "action/quit_to_exit.hpp"
#include "quit.hpp"

void action::quit(ENetEvent& event, const std::string& header) 
{
    action::quit_to_exit(event, "", true);
    
    if (event.peer == nullptr) return;
    if (event.peer->data != nullptr) 
    {
        // Calculate session duration and add to total play time before erasing the peer object
        auto& peer = _peer[event.peer];
        auto session_duration = std::chrono::steady_clock::now() - peer->login_time;
        auto session_seconds = std::chrono::duration_cast<std::chrono::seconds>(session_duration).count();
        peer->play_time_seconds += session_seconds;

        event.peer->data = nullptr;
        _peer.erase(event.peer);
    }
    enet_peer_reset(event.peer);
}