#include "pch.hpp"
#include "database.hpp"
#include "world_db.hpp"

using namespace std::chrono;

// ============================================================================
// Database table creation — called once at startup
// ============================================================================
void init_world_database()
{
    hStmt create(R"(
        CREATE TABLE IF NOT EXISTS world_state (
            name       VARCHAR(100) PRIMARY KEY,
            owner      INT NOT NULL DEFAULT 0,
            data       MEDIUMBLOB,
            updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP
        ) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4
    )");
    mysql_stmt_execute(create.pStmt);
}

// ============================================================================
// Serialization — world → binary blob
// ============================================================================
static std::vector<u_char> serialize_world(const world& w)
{
    std::vector<u_char> buf;
    auto write8  = [&](u_char v)  { buf.push_back(v); };
    auto write16 = [&](u_short v) { buf.push_back(v & 0xFF); buf.push_back((v >> 8) & 0xFF); };
    auto write32 = [&](u_int v)   { buf.push_back(v & 0xFF); buf.push_back((v >> 8) & 0xFF); buf.push_back((v >> 16) & 0xFF); buf.push_back((v >> 24) & 0xFF); };
    auto write64 = [&](long long v) { write32(static_cast<u_int>(v & 0xFFFFFFFF)); write32(static_cast<u_int>((v >> 32) & 0xFFFFFFFF)); };
    auto write_str = [&](const std::string& s) {
        write16(static_cast<u_short>(s.size()));
        buf.insert(buf.end(), s.begin(), s.end());
    };

    // Version
    write8(1);

    // World metadata
    write32(static_cast<u_int>(w.owner));
    write8(w.is_public ? 1 : 0);
    write8(w.lock_state);
    write8(w.minimum_entry_level);
    write32(static_cast<u_int>(w.weather.x));
    write32(static_cast<u_int>(w.weather.y));

    // Access list
    write32(static_cast<u_int>(w.access.size()));
    for (int uid : w.access)
        write32(static_cast<u_int>(uid));

    // Blocks (6000 tiles)
    const auto& blocks = w.blocks;
    write32(static_cast<u_int>(blocks.size()));
    for (const auto& b : blocks)
    {
        write16(static_cast<u_short>(b.fg));
        write16(static_cast<u_short>(b.bg));
        buf.insert(buf.end(), b.state, b.state + 4);
        buf.push_back(b.hits[0]);
        buf.push_back(b.hits[1]);
        // tick: store as elapsed seconds from epoch to avoid serialization issues
        auto tick_dur = b.tick.time_since_epoch();
        auto tick_ms = duration_cast<milliseconds>(tick_dur).count();
        write64(tick_ms);
        write_str(b.label);
    }

    // Doors
    write32(static_cast<u_int>(w.doors.size()));
    for (const auto& d : w.doors)
    {
        write_str(d.dest);
        write_str(d.id);
        write_str(d.password);
        write32(static_cast<u_int>(d.pos.x));
        write32(static_cast<u_int>(d.pos.y));
    }

    // Displays
    write32(static_cast<u_int>(w.displays.size()));
    for (const auto& d : w.displays)
    {
        write32(d.id);
        write32(static_cast<u_int>(d.pos.x));
        write32(static_cast<u_int>(d.pos.y));
    }

    // Random blocks
    write32(static_cast<u_int>(w.random_blocks.size()));
    for (const auto& r : w.random_blocks)
    {
        write8(r.value);
        write32(static_cast<u_int>(r.pos.x));
        write32(static_cast<u_int>(r.pos.y));
    }

    // Objects
    write32(static_cast<u_int>(w.objects.size()));
    for (const auto& o : w.objects)
    {
        write16(o.id);
        write16(o.count);
        write32(static_cast<u_int>(o.pos.x));
        write32(static_cast<u_int>(o.pos.y));
        write32(o.uid);
    }
    write32(w.last_object_uid);

    return buf;
}

// ============================================================================
// Deserialization — binary blob → world
// ============================================================================
static void deserialize_world(world& w, const std::vector<u_char>& buf)
{
    if (buf.empty()) return;

    size_t pos = 0;
    auto read8  = [&]() -> u_char       { if (pos >= buf.size()) return 0; return buf[pos++]; };
    auto read16 = [&]() -> u_short      { if (pos + 2 > buf.size()) return 0; u_short v = buf[pos] | (buf[pos+1] << 8); pos += 2; return v; };
    auto read32 = [&]() -> u_int        { if (pos + 4 > buf.size()) return 0; u_int v = buf[pos] | (buf[pos+1] << 8) | (buf[pos+2] << 16) | (buf[pos+3] << 24); pos += 4; return v; };
    auto read64 = [&]() -> long long    { u_int lo = read32(); u_int hi = read32(); return static_cast<long long>(lo) | (static_cast<long long>(hi) << 32); };
    auto read_str = [&]() -> std::string {
        u_short len = read16();
        if (pos + len > buf.size()) return {};
        std::string s(reinterpret_cast<const char*>(buf.data() + pos), len);
        pos += len;
        return s;
    };

    u_char version = read8();
    if (version != 1) return; // unknown format

    // Metadata
    w.owner = static_cast<int>(read32());
    w.is_public = read8() != 0;
    w.lock_state = read8();
    w.minimum_entry_level = read8();
    w.weather.x = static_cast<float>(static_cast<int>(read32()));
    w.weather.y = static_cast<float>(static_cast<int>(read32()));

    // Access list
    u_int access_count = read32();
    w.access.clear();
    w.access.reserve(access_count);
    for (u_int i = 0; i < access_count; ++i)
        w.access.push_back(static_cast<int>(read32()));

    // Blocks
    u_int block_count = read32();
    w.blocks.resize(block_count);
    for (u_int i = 0; i < block_count && i < 6000; ++i)
    {
        auto& b = w.blocks[i];
        b.fg = static_cast<short>(read16());
        b.bg = static_cast<short>(read16());
        for (int j = 0; j < 4; ++j) b.state[j] = read8();
        b.hits[0] = read8();
        b.hits[1] = read8();
        long long tick_ms = read64();
        if (tick_ms > 0)
            b.tick = time_point<steady_clock>(milliseconds(tick_ms));
        b.label = read_str();
    }

    // Doors
    u_int door_count = read32();
    w.doors.clear();
    w.doors.reserve(door_count);
    for (u_int i = 0; i < door_count; ++i)
    {
        std::string dest     = read_str();
        std::string id       = read_str();
        std::string password = read_str();
        ::pos p(static_cast<float>(static_cast<int>(read32())),
                static_cast<float>(static_cast<int>(read32())));
        w.doors.emplace_back(dest, id, password, p);
    }

    // Displays
    u_int display_count = read32();
    w.displays.clear();
    w.displays.reserve(display_count);
    for (u_int i = 0; i < display_count; ++i)
    {
        u_int id = read32();
        ::pos p(static_cast<float>(static_cast<int>(read32())),
                static_cast<float>(static_cast<int>(read32())));
        w.displays.emplace_back(id, p);
    }

    // Random blocks
    u_int random_count = read32();
    w.random_blocks.clear();
    w.random_blocks.reserve(random_count);
    for (u_int i = 0; i < random_count; ++i)
    {
        u_char v = read8();
        ::pos p(static_cast<float>(static_cast<int>(read32())),
                static_cast<float>(static_cast<int>(read32())));
        w.random_blocks.emplace_back(v, p);
    }

    // Objects
    u_int object_count = read32();
    w.objects.clear();
    w.objects.reserve(object_count);
    for (u_int i = 0; i < object_count; ++i)
    {
        u_short id    = read16();
        u_short count = read16();
        ::pos p(static_cast<float>(static_cast<int>(read32())),
                static_cast<float>(static_cast<int>(read32())));
        u_int uid = read32();
        w.objects.emplace_back(id, count, p, uid);
    }
    w.last_object_uid = read32();
}

// ============================================================================
// Public API
// ============================================================================
void save_world(const world& w)
{
    auto data = serialize_world(w);

    hStmt stmt("INSERT INTO world_state (name, owner, data) VALUES (?, ?, ?) "
               "ON DUPLICATE KEY UPDATE owner = VALUES(owner), data = VALUES(data), updated_at = NOW()");

    MYSQL_BIND params[3] = {
        make_bind_in(w.name),
        make_bind_in(w.owner),
        make_bind_in(std::string(reinterpret_cast<const char*>(data.data()), data.size()))
    };
    mysql_stmt_bind_param(stmt.pStmt, params);
    mysql_stmt_execute(stmt.pStmt);
}

void load_world(world& w)
{
    hStmt stmt("SELECT data FROM world_state WHERE name = ?");

    MYSQL_BIND param = make_bind_in(w.name);
    mysql_stmt_bind_param(stmt.pStmt, &param);
    mysql_stmt_execute(stmt.pStmt);
    mysql_stmt_store_result(stmt.pStmt);

    // Bind result
    unsigned long data_len = 0;
    std::vector<u_char> data_buf(1048576, 0); // 1MB max
    MYSQL_BIND result{};
    result.buffer_type = MYSQL_TYPE_BLOB;
    result.buffer = data_buf.data();
    result.buffer_length = static_cast<u_long>(data_buf.size());
    result.length = &data_len;
    mysql_stmt_bind_result(stmt.pStmt, &result);

    if (mysql_stmt_fetch(stmt.pStmt) == 0 && data_len > 0)
    {
        data_buf.resize(data_len);
        deserialize_world(w, data_buf);
    }
}

void delete_world(const std::string& name)
{
    hStmt stmt("DELETE FROM world_state WHERE name = ?");
    MYSQL_BIND param = make_bind_in(name);
    mysql_stmt_bind_param(stmt.pStmt, &param);
    mysql_stmt_execute(stmt.pStmt);
}
