/* Resident entry points into the banked game modules (see platform/bank.h) and
   small resident helpers they share. */
#ifndef GAME_BANKED_H
#define GAME_BANKED_H

#include <stdint.h>

/* logic.c (PAGE_LOGIC) */
void tick_logic(void);          /* fmain.c:2013-2221 - runs on frames without a tile scroll */
void revive(uint8_t new_hero);  /* fmain.c:2814 */
void game_start(void);          /* first brother, like the start of main() */
void find_place(uint8_t flag);
void debug_encounter(void);     /* test helper: ogres around the hero */

/* doors.c (PAGE_DOORS) */
void door_check(void);          /* fmain.c:1871-1956 - beds and doors */
uint8_t doorfind(uint16_t x, uint16_t y, uint8_t keytype);
void xfer(uint16_t x, uint16_t y, uint8_t flag);

/* objects.c (PAGE_OBJECTS) */
void do_objects(void);
void leave_item(uint8_t i, uint8_t object);
void change_object(uint8_t id, uint8_t flag);
#define OBLIST_GLOBAL 10
uint8_t ob_stat_get(uint8_t list, uint8_t idx);
void ob_stat_set(uint8_t list, uint8_t idx, uint8_t stat);
void ob_pos_set(uint8_t list, uint8_t idx, uint16_t x, uint16_t y);
uint8_t ob_id_get(uint8_t list, uint8_t idx);
void ob_id_set(uint8_t list, uint8_t idx, uint8_t id);

/* resident helpers */
void savegame(uint8_t slot, uint8_t save);      /* save.c: slot 0..7 = A..H */
void move_extent(uint8_t e, uint16_t x, uint16_t y);
int8_t setfig_image_base(uint8_t id);
uint8_t setfig_cfile(uint8_t id);
void fade_page(int16_t r, int16_t g, int16_t b, uint8_t limit);
void day_fade(void);
void load_all(void);            /* load_new_region until stable + view refresh */

#endif
