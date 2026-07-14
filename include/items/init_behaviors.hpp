#pragma once

// @note Call once at startup (after decode_items()) to register all
// item-specific behaviors. This replaces the hardcoded switch(item->id)
// blocks in tile_change.cpp with a modular registration system.
// To add/edit a behavior, change only this file — no core file edits needed.
extern void register_item_behaviors();
