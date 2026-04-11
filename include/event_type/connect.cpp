#include "pch.hpp"
#include "action/quit.hpp"
#include "connect.hpp"

void _connect(ENetEvent& event) 
{
    if (peers().size() > host->peerCount) 
    {
        packet::action(*event.peer, "log", 
            std::format(
                "msg|`4SERVER OVERLOADED`` : Sorry, our servers are currently at max capacity with {} online, please try later. We are working to improve this!",
                host->peerCount
            ));
        packet::action(*event.peer, "logon_fail", ""); // @note triggers action|quit on client.
    }
    else 
    {
        enet_peer_send(event.peer, 0, enet_packet_create((enet_uint8[4]){ 01 }, 4, ENET_PACKET_FLAG_RELIABLE));

        event.peer->data = new peer();
    }
}
