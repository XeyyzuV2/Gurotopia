#include "pch.hpp"
#include "tools/string.hpp" // @note base64_decode()
#include "https/server_data.hpp"

#include "protocol.hpp"

void action::protocol(ENetEvent& event, const std::string& header)
{
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);
    try 
    {
        std::vector<std::string> pipes = readch(header, '|');
        if (pipes.size() < 4zu) throw std::runtime_error("");

        std::string growid{}, password{};
        if (pipes[2zu] == "ltoken")
        {
            const std::string decoded = base64_decode(pipes[3zu]);
            if (decoded.empty()) throw std::runtime_error("");

            if (std::size_t pos = decoded.find("growId="); pos != std::string::npos) 
            {
                pos += sizeof("growId=")-1zu;
                growid = decoded.substr(pos, decoded.find('&', pos) - pos);
            }
            if (std::size_t pos = decoded.find("password="); pos != std::string::npos) 
            {
                pos += sizeof("password=")-1zu;
                password = decoded.substr(pos, decoded.find('&', pos) - pos);
            }
        } // @note delete decoded
        if (growid.empty() || password.empty()) throw std::runtime_error("");
        pPeer->ltoken = { growid, password/*@todo*/ };
    }
    catch (...) { 
        packet::action(*event.peer, "logon_fail", "");
        return; // @note stop processing invalid protocol data
    }

    if (!pPeer->exists(pPeer->ltoken[0]))
    {
        pPeer->mysql_insert("growid", pPeer->ltoken[0]);
        
        pPeer->mysql_update<std::string>("password", pPeer->ltoken[1]);
    }
    pPeer->mysql_select_all();

    packet::create(*event.peer, false, 0, {"SetHasGrowID", 1, pPeer->ltoken[0].c_str(), ""}); // @todo temp fix, i will change later.

    packet::create(*event.peer, false, 0, {
        "OnSendToServer",
        (signed)gServer_data.port,
        0,
        pPeer->user_id,
        std::format("{}|0|0", gServer_data.server).c_str(),
        1,
        pPeer->ltoken[0].c_str()
    }); // @note  PACKET_DISCONNECT
}
