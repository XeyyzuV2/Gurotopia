#include "pch.hpp"
#include "effects.hpp"

// This function maps an equippable item's ID to a punch effect ID.
// Data cross-referenced from community-provided documents.
int get_punch_id(int item_id) {
    switch (item_id) {
        case 138:  return 1;  // Cyclopean Visor
        case 366:  return 2;  // Heartbow
        case 472:  return 3;  // Tommygun
        case 594:  return 4;  // Elvish Longbow
        case 768:  return 5;  // Sawed-Off Shotgun
        case 900:  return 6;  // Dragon Hand
        case 910:  return 7;  // Reanimator Remote
        case 930:  return 8;  // Death Ray
        case 1016: return 9;  // Six Shooter
        case 1204: return 10; // Focused Eyes
        case 1378: return 11; // Ice Dragon Hand
        case 1512: return 14; // Pet Leprechaun
        case 1542: return 15; // Battle Trout
        case 1576: return 16; // Fiesta Dragon
        case 1676: return 17; // Squirt Gun
        case 1710: return 18; // Keytar
        case 1748: return 19; // Flamethrower
        case 1780: return 20; // Legendbot-009
        case 1782: return 21; // Dragon of Legend
        case 1804: return 22; // Zeus' Lightning Bolt
        case 1868: return 23; // Violet Protodrake Leash
        case 1874: return 24; // Ring of Force
        case 2212: return 32; // Black Crystal Dragon
        case 2218: return 33; // Mighty Snow Rod
        case 2220: return 34; // Teeny Tank
        case 2266: return 35; // Crystal Glaive
        case 2386: return 36; // Heavenly Scythe
        case 2388: return 37; // Heartbreaker Hammer
        case 2450: return 38; // Diamond Dragon
        case 2476: return 39; // Burning Eyes
        case 2572: return 42; // Flame Scythe
        case 2592: return 43; // Legendary Katana
        case 2720: return 44; // Electric Bow
        case 2752: return 45; // Pineapple Launcher
        case 2754: return 46; // Demonic Arm
        case 2756: return 47; // The Gungnir
        case 2802: return 49; // Poseidon's Trident
        case 2866: return 50; // Wizard's Staff
        case 3066: return 57; // Fire Hose
        case 3108: return 59; // Chainsaw Hand
        case 3214: return 60; // Axe of Winter
        case 3300: return 64; // Party Blaster
        case 3418: return 65; // Serpent Staff
        case 3686: return 68; // Toy Lock-Bot
        case 3716: return 69; // Neutron Gun
        case 5480: return 80; // Rayman's Fist
        default:   return 0;  // No effect
    }
}
