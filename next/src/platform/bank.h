/* Banked code: modules are linked into 8K pages with org 0x0000 and run in MMU slot 0.
   Calls go through small resident wrappers that map the page and restore the caller's. */
#ifndef PLATFORM_BANK_H
#define PLATFORM_BANK_H

#include <stdint.h>
#include "hw.h"

#define ROM_PAGE        0xFF

#define PAGE_LOGIC      80      /* game/logic.c   - daily tick, AI, encounters, revive */
#define PAGE_DOORS      81      /* game/doors.c   - doors, xfer, find_place */
#define PAGE_OBJECTS    82      /* game/objects.c - object lists */
#define PAGE_MOVE       83      /* game/move.c    - actor loop, combat */
#define PAGE_DRAW       84      /* game/sprites.c - actors as hardware sprites */
#define PAGE_COMBAT     85      /* game/combat.c  - melee, missiles, depth sort */
#define PAGE_MENU       86      /* game/menus.c   - option menus */
#define PAGE_AUDIO      87      /* game/audio.c   - AY music and effects */
#define PAGE_PLACARD    88      /* game/placard.c - placards, still screens */
#define PAGE_INTRO      89      /* game/intro.c   - title, intro, rescue, victory */

extern uint8_t cur_bank;        /* page in slot 0 (ROM_PAGE when no banked code runs) */

#define BANK_ENTER(p)   uint8_t bank_saved_ = cur_bank; cur_bank = (p); mmu_set(0, (p))
#define BANK_LEAVE()    do { cur_bank = bank_saved_; mmu_set(0, bank_saved_); } while (0)

#endif
