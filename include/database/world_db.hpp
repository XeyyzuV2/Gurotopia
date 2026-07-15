#pragma once

// @note World persistence — save/load world state to MariaDB.
// Adds a `world_state` table storing serialized block arrays + metadata.

extern void init_world_database();
extern void save_world(const class world& w);
extern void load_world(class world& w);
extern void delete_world(const std::string& name);
