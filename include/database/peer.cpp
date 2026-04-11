#include "pch.hpp"
#include "database.hpp"
#include "items.hpp"
#include "world.hpp"
#include "on/SetClothing.hpp"
#include "on/CountryState.hpp"
#include "commands/punch.hpp"

#include "peer.hpp"

using namespace std::chrono;

bool peer::exists(const std::string& growid)
{
    ::hStmt hStmt{ "SELECT 1 FROM peer WHERE growid = ? LIMIT 1" };

    MYSQL_BIND param = make_bind_in(growid);
    mysql_stmt_bind_param(hStmt.pStmt, &param);
    mysql_stmt_execute(hStmt.pStmt);

    return (!mysql_stmt_store_result(hStmt.pStmt) && mysql_stmt_num_rows(hStmt.pStmt) > 0);
}

template<typename T>
void peer::mysql_insert(const std::string& column, const T& value)
{
    ::hStmt hStmt{ std::format("INSERT INTO peer ({}) VALUES (?)", column).c_str() };

    MYSQL_BIND param = make_bind_in(value);
    mysql_stmt_bind_param(hStmt.pStmt, &param);
    mysql_stmt_execute(hStmt.pStmt);
}
template void peer::mysql_insert<signed>(const std::string&, const signed&);
template void peer::mysql_insert<unsigned>(const std::string&, const unsigned&);
template void peer::mysql_insert<float>(const std::string&, const float&);
template void peer::mysql_insert<std::string>(const std::string&, const std::string&);

template<typename T>
void peer::mysql_update(const std::string& column, const T& value)
{
    ::hStmt hStmt{ std::format("UPDATE peer SET {} = ? WHERE growid = ?", column).c_str() };

    MYSQL_BIND params[2] = {
        make_bind_in(value),           // SET
        make_bind_in(this->ltoken[0]) // WHERE
    };
    mysql_stmt_bind_param(hStmt.pStmt, params);
    mysql_stmt_execute(hStmt.pStmt);
}
template void peer::mysql_update<signed>(const std::string&, const signed&);
template void peer::mysql_update<unsigned>(const std::string&, const unsigned&);
template void peer::mysql_update<float>(const std::string&, const float&);
template void peer::mysql_update<std::string>(const std::string&, const std::string&);

template<typename T>
T peer::mysql_select(const std::string &column, const char *arg)
{
    T value{};
    ::hStmt hStmt{ std::format("SELECT {}({}) FROM peer WHERE growid = ? LIMIT 1", arg, column).c_str() };

    MYSQL_BIND param = make_bind_in(this->ltoken[0]);
    mysql_stmt_bind_param(hStmt.pStmt, &param);

    MYSQL_BIND result = make_bind_out(value);
    mysql_stmt_bind_result(hStmt.pStmt, &result);

    mysql_stmt_execute(hStmt.pStmt);
    mysql_stmt_fetch(hStmt.pStmt);
    return value;
}
/* since we will only select during mysql_select_all */ // @note add templates here if use select outside of this file.

void peer::mysql_select_all()
{
    this->user_id    = this->mysql_select<signed>("uid");
    this->ltoken[0]  = this->mysql_select<std::string>("growid");
    this->ltoken[1]  = this->mysql_select<std::string>("password");
    this->created_at = this->mysql_select<std::time_t>("created_at", "UNIX_TIMESTAMP");
}

short peer::emplace(slot s) 
{
    if (auto it = std::ranges::find(this->slots, s.id, &::slot::id); it != this->slots.end()) 
    {
        short excess = std::max(0, (it->count + s.count) - 200);
        it->count = std::min(it->count + s.count, 200);
        if (it->count == 0)
        {
            auto item = std::ranges::find(items, it->id, &::item::id);
            if (item->cloth_type != clothing::none) this->clothing[item->cloth_type] = 0;
        }
        return excess;
    }
    else this->slots.emplace_back(std::move(s)); // @note no such item in inventory, so we create a new entry.
    return 0;
}

void peer::add_xp(ENetEvent &event, u_short value) 
{
    u_short &lvl = this->level.front();
    u_short &xp = this->level.back() += value; // @note factor the new xp amount

    for (; lvl < 125; )
    {
        u_short xp_formula = 50 * (lvl * lvl + 2); // @author https://www.growtopiagame.com/forums/member/553046-kasete
        if (xp < xp_formula) break;

        xp -= xp_formula;
        lvl++;

        if (lvl == 50) 
        {
            modify_item_inventory(event, ::slot{1400, 1}); // @note Mini Growtopian
            /* @todo based on account age give peer other items... */
        }
        if (lvl == 125) on::CountryState(event);

        /* @todo make particle effect smaller like growtopia */
        state_visuals(*event.peer, {
            .type = 0x11, // @note PACKET_SEND_PARTICLE_EFFECT
            .pos = this->pos, 
            .speed = ::pos{0, 0x2e}
        });
        packet::create(*event.peer, false, 0, {
            "OnTalkBubble", this->netid,
            std::format("`{}{}`` is now level {}!", this->prefix, this->ltoken[0], lvl).c_str()
        });
    }
}

void peer::update_effects()
{
    this->punch_effect = 0;
    for (float cloth : this->clothing)
    {
        u_short punch_id = get_punch_id(static_cast<u_int>(cloth));
        if (punch_id != 0)
            this->punch_effect = punch_id;
    }
}

ENetHost *host;

std::vector<ENetPeer*> peers(const std::string &world, peer_condition condition, std::function<void(ENetPeer&)> fun)
{
    std::vector<ENetPeer*> _peers{};
    _peers.reserve(host->peerCount);

    for (ENetPeer &peer : std::span(host->peers, host->peerCount))
        if (peer.state == ENET_PEER_STATE_CONNECTED)
        {
            if (condition == peer_condition::PEER_SAME_WORLD)
            {
                ::peer *pOthers = static_cast<::peer*>(peer.data);
                if (pOthers->netid == 0 || (pOthers->recent_worlds.back() != world)) continue;
            }
            fun(peer);
            _peers.push_back(&peer);
        }

    return _peers;
}

void safe_disconnect_peers(int code)
{
    puts("killing gurotopia...");

    for (ENetPeer &p : std::span(host->peers, host->peerCount))
        if (p.state == ENET_PEER_STATE_CONNECTED)
            enet_peer_disconnect(&p, 0);

    enet_host_flush(host);
    enet_host_destroy(host);
    host = nullptr;
    enet_deinitialize();
    puts("killed gurotopia safely!");
}

state get_state(const std::vector<u_char> &&packet) 
{
    const int *_4bit = reinterpret_cast<const int*>(packet.data());
    const float *_4bit_f = reinterpret_cast<const float*>(packet.data());
    return state{
        .type = _4bit[1],
        .netid = _4bit[2],
        .uid = _4bit[3],
        .peer_state = _4bit[4],
        .count = _4bit_f[5],
        .id = _4bit[6],
        .pos = ::pos{_4bit_f[7], _4bit_f[8]},
        .speed = ::pos{_4bit_f[9], _4bit_f[10]},
        .idk = _4bit[11],
        .punch = ::pos{_4bit[12], _4bit[13]},
        .size = _4bit[14]
    };
}

std::vector<u_char> compress_state(const state &state) 
{
    std::vector<u_char> data(sizeof(::state) + 1, 0x00);
    int *_4bit = reinterpret_cast<int*>(data.data());
    float *_4bit_f = reinterpret_cast<float*>(data.data());
    _4bit[0] = state.packet_create;
    _4bit[1] = state.type;
    _4bit[2] = state.netid;
    _4bit[3] = state.uid;
    _4bit[4] = state.peer_state;
    _4bit_f[5] = state.count;
    _4bit[6] = state.id;
    _4bit_f[7] = state.pos.x;
    _4bit_f[8] = state.pos.y;
    _4bit_f[9] = state.speed.x;
    _4bit_f[10] = state.speed.y;
    _4bit[11] = state.idk;
    _4bit[12] = state.punch.x;
    _4bit[13] = state.punch.y;
    _4bit[14] = state.size;
    return data;
}

void send_inventory_state(ENetEvent &event)
{
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);

    std::vector<u_char> data = compress_state(::state{
        .type = 0x09, // @note PACKET_SEND_INVENTORY_STATE
        .netid = pPeer->netid,
        .peer_state = 0x08
    });

    std::size_t size = pPeer->slots.size();
    data.resize(data.size() + 5zu + (size * sizeof(int)));

    int *_4bit = reinterpret_cast<int*>(&data[58zu]);

    *_4bit++ = std::byteswap<int>(pPeer->slot_size);
    *_4bit++ = std::byteswap<int>(size);
    for (const ::slot &slot : pPeer->slots)
        *_4bit++ = slot.id | (slot.count & 0xff) << 16;

	enet_peer_send(event.peer, 0, enet_packet_create(data.data(), data.size(), ENET_PACKET_FLAG_RELIABLE));
}
