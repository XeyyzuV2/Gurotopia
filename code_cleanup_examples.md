# Gurotopia Code Cleanup Plan

## Phase 1: Standardisasi Komentar dan Naming Conventions

### 1.1 Standardisasi Komentar
**BEFORE:**
```cpp
// @note peer's netid is world identity. this will be useful for many packet sending
signed netid{ 0 };
int user_id{}; // @note unqiue user id.
std::array<std::string, 2zu> ltoken{}; // @note {growid, password}
std::string prefix{ "w" }; // @note display name color, default: "w" (White)
```

**AFTER:**
```cpp
/**
 * @brief Network ID assigned to peer within current world
 * @details Used for packet routing and player identification in multiplayer context
 */
signed netid{0};

/**
 * @brief Unique user identifier for database operations
 * @details Generated using FNV1a hash of GrowID for consistency
 */
int user_id{};

/**
 * @brief Login credentials storage
 * @details [0] = GrowID, [1] = Password (base64 decoded)
 */
std::array<std::string, 2> login_token{};

/**
 * @brief Display name color prefix for chat messages
 * @details Default "w" (white), "2" (green) for world owners, "#@" for mods
 */
std::string name_prefix{"w"};
```

### 1.2 Naming Convention Standardization
**BEFORE:**
```cpp
std::array<std::string, 6zu> recent_worlds{};
std::array<std::string, 200zu> my_worlds{};
std::deque<std::chrono::steady_clock::time_point> messages;
std::array<Friend, 25> friends;
```

**AFTER:**
```cpp
std::array<std::string, MAX_RECENT_WORLDS> recent_worlds{};
std::array<std::string, MAX_OWNED_WORLDS> owned_worlds{};
std::deque<std::chrono::steady_clock::time_point> message_timestamps{};
std::array<Friend, MAX_FRIENDS> friend_list{};
```

## Phase 2: Constants dan Magic Numbers

### 2.1 Create Constants Header
```cpp
// include/constants/game_constants.hpp
#pragma once

namespace GameConstants {
    // World dimensions
    constexpr int WORLD_WIDTH = 100;
    constexpr int WORLD_HEIGHT = 60;
    constexpr int TOTAL_TILES = WORLD_WIDTH * WORLD_HEIGHT;
    
    // Player limits
    constexpr int MAX_RECENT_WORLDS = 6;
    constexpr int MAX_OWNED_WORLDS = 200;
    constexpr int MAX_FRIENDS = 25;
    constexpr int MAX_INVENTORY_SLOTS = 200;
    constexpr int DEFAULT_INVENTORY_SIZE = 16;
    
    // Item constants
    constexpr int FIST_ID = 18;
    constexpr int WRENCH_ID = 32;
    constexpr int WORLD_LOCK_ID = 242;
    
    // Packet types
    constexpr uint8_t PACKET_TYPE_STRING = 2;
    constexpr uint8_t PACKET_TYPE_ACTION = 3;
    constexpr uint8_t PACKET_TYPE_STATE = 4;
    constexpr uint8_t PACKET_TYPE_TILE_UPDATE = 5;
    
    // State types
    constexpr uint8_t STATE_MOVEMENT = 0x00;
    constexpr uint8_t STATE_TILE_CHANGE = 0x03;
    constexpr uint8_t STATE_TILE_ACTIVATE = 0x07;
    constexpr uint8_t STATE_ITEM_ACTIVATE = 0x0a;
    constexpr uint8_t STATE_ITEM_ACTIVATE_OBJECT = 0x0b;
}

namespace PacketConstants {
    constexpr size_t BASE_PACKET_SIZE = 61;
    constexpr std::byte PACKET_HEADER = std::byte{0x04};
    constexpr std::byte VARIANT_PACKET = std::byte{0x01};
    constexpr std::byte MAP_DATA_PACKET = std::byte{0x04};
    constexpr std::byte INVENTORY_PACKET = std::byte{0x09};
}
```

### 2.2 Refactor Magic Numbers
**BEFORE:**
```cpp
std::vector<std::byte> data(61, std::byte{ 00 });
data[0zu] = std::byte{ 04 };
data[4zu] = std::byte{ 01 };
```

**AFTER:**
```cpp
std::vector<std::byte> data(PacketConstants::BASE_PACKET_SIZE, std::byte{0x00});
data[0] = PacketConstants::PACKET_HEADER;
data[4] = PacketConstants::VARIANT_PACKET;
```

## Phase 3: Error Handling Improvement

### 3.1 Custom Exception Classes
```cpp
// include/exceptions/game_exceptions.hpp
#pragma once
#include <stdexcept>
#include <string>

namespace GameExceptions {
    class GameException : public std::runtime_error {
    public:
        explicit GameException(const std::string& message) 
            : std::runtime_error(message) {}
    };
    
    class InvalidWorldNameException : public GameException {
    public:
        InvalidWorldNameException() 
            : GameException("Invalid world name: only alphanumeric characters allowed") {}
    };
    
    class InsufficientPermissionsException : public GameException {
    public:
        InsufficientPermissionsException() 
            : GameException("You don't have permission to perform this action") {}
    };
    
    class ItemNotFoundException : public GameException {
    public:
        explicit ItemNotFoundException(int item_id) 
            : GameException("Item with ID " + std::to_string(item_id) + " not found") {}
    };
}
```

### 3.2 Structured Error Handling
**BEFORE:**
```cpp
try {
    // some operation
} catch (...) { 
    puts("unknown error occured during decoding items.dat"); 
}
```

**AFTER:**
```cpp
try {
    // some operation
} catch (const std::filesystem::filesystem_error& e) {
    Logger::error("Filesystem error: {}", e.what());
    throw GameExceptions::GameException("Failed to load items.dat: " + std::string(e.what()));
} catch (const std::exception& e) {
    Logger::error("Unexpected error during items.dat parsing: {}", e.what());
    throw GameExceptions::GameException("Items data parsing failed");
}
```

## Phase 4: Modularisasi

### 4.1 Utility Functions
```cpp
// include/utils/coordinate_utils.hpp
#pragma once
#include "constants/game_constants.hpp"

namespace CoordinateUtils {
    /**
     * @brief Convert 2D coordinates to 1D array index
     * @param x X coordinate (0-99)
     * @param y Y coordinate (0-59)  
     * @return 1D index for tile array
     */
    constexpr int to_index(int x, int y) {
        return y * GameConstants::WORLD_WIDTH + x;
    }
    
    /**
     * @brief Convert 1D index back to 2D coordinates
     * @param index 1D array index
     * @return std::pair<int, int> {x, y} coordinates
     */
    constexpr std::pair<int, int> to_coordinates(int index) {
        return {index % GameConstants::WORLD_WIDTH, index / GameConstants::WORLD_WIDTH};
    }
    
    /**
     * @brief Check if coordinates are within world bounds
     */
    constexpr bool is_valid_coordinate(int x, int y) {
        return x >= 0 && x < GameConstants::WORLD_WIDTH && 
               y >= 0 && y < GameConstants::WORLD_HEIGHT;
    }
}
```

### 4.2 String Utilities
```cpp
// include/utils/string_utils.hpp
#pragma once
#include <string>
#include <vector>
#include <algorithm>

namespace StringUtils {
    /**
     * @brief Split string by delimiter
     * @param input Input string to split
     * @param delimiter Character to split on
     * @return Vector of string parts
     */
    std::vector<std::string> split(const std::string& input, char delimiter);
    
    /**
     * @brief Convert string to uppercase
     */
    std::string to_upper(std::string input);
    
    /**
     * @brief Check if string contains only alphanumeric characters
     */
    bool is_alphanumeric(const std::string& input);
    
    /**
     * @brief Base64 decode utility
     */
    std::string base64_decode(const std::string& encoded);
}
```

## Benefits of This Cleanup:

1. **Maintainability**: Kode lebih mudah dibaca dan dipahami
2. **Consistency**: Naming convention yang seragam
3. **Debuggability**: Error messages yang lebih informatif  
4. **Extensibility**: Struktur yang lebih modular untuk fitur baru
5. **Documentation**: Self-documenting code dengan comments yang jelas