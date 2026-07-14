#pragma once

// @note Item property bitfield — mirrors the flags in items.dat.
// Each item's `property` byte can contain multiple flags combined via |
// The official server uses these to determine behavior, NOT just item type.
enum item_property : u_char {
    PROP_NONE               = 0,
    PROP_DROP_SEED          = 1 << 0, // 0x01  — drops a seed variant when smashed
    PROP_NO_SEED            = 1 << 1, // 0x02  — never drops seeds even if type::SEED
    PROP_DROPLESS           = 1 << 2, // 0x04  — never drops a block when smashed (Gurotopia's &04)
    PROP_PERMANENT          = 1 << 3, // 0x08  — block can't be destroyed normally
    PROP_MULTI_FACING       = 1 << 4, // 0x10  — can face multiple directions
    PROP_NO_SHADOW          = 1 << 5, // 0x20  — tile casts no shadow
    PROP_WRENCHABLE         = 1 << 6, // 0x40  — wrench action opens a dialog
    PROP_AUTO_PICKUP        = 1 << 7, // 0x80  — item auto-returns to inventory on break
};

// @note Item category flags (combined with cat field)
enum item_category : u_char {
    CAT_RETURN              = 1 << 1, // 0x02  — can't be destroyed, returns to backpack
    CAT_SURPRISING_FRUIT    = 1 << 3, // 0x08  — tree bears surprising fruit
    CAT_PUBLIC              = 1 << 4, // 0x10  — anyone can smash even in locked world
    CAT_HOLIDAY             = 1 << 6, // 0x40  — only creatable during events
    CAT_UNTRADEABLE         = 1 << 7, // 0x80  — cannot be dropped or traded
};

// @brief Check if an item has a given property flag
inline bool has_property(const class item& item, item_property flag) {
    return (item.property & static_cast<u_char>(flag)) != 0;
}

// @brief Check if an item has a given category flag
inline bool has_category(const class item& item, item_category flag) {
    return (item.cat & static_cast<u_char>(flag)) != 0;
}
