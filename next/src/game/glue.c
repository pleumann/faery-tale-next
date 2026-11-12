/* Resident part of the game: wrappers into banked modules and small shared helpers. */
#include <stdint.h>
#include "../platform/bank.h"
#include "../platform/layer2.h"
#include "state.h"
#include "world.h"
#include "view.h"
#include "sprites.h"
#include "banked.h"

/* ---- banked entry points: functions with suffix _b live in the named page ---- */

void tick_logic_b(void);
void revive_b(uint8_t new_hero);
void game_start_b(void);
void door_check_b(void);
uint8_t doorfind_b(uint16_t x, uint16_t y, uint8_t keytype);
void xfer_b(uint16_t x, uint16_t y, uint8_t flag);
void find_place_b(uint8_t flag);
void do_objects_b(void);
void leave_item_b(uint8_t i, uint8_t object);
void change_object_b(uint8_t id, uint8_t flag);
uint8_t ob_field_b(uint8_t list, uint8_t idx, uint8_t field, uint8_t write, uint16_t value);

void tick_logic(void) { BANK_ENTER(PAGE_LOGIC); tick_logic_b(); BANK_LEAVE(); }
void revive(uint8_t new_hero) { BANK_ENTER(PAGE_LOGIC); revive_b(new_hero); BANK_LEAVE(); }
void debug_encounter_b(void);
void debug_encounter(void) { BANK_ENTER(PAGE_LOGIC); debug_encounter_b(); BANK_LEAVE(); }
void game_start(void) { BANK_ENTER(PAGE_LOGIC); game_start_b(); BANK_LEAVE(); }

void door_check(void) { BANK_ENTER(PAGE_DOORS); door_check_b(); BANK_LEAVE(); }
uint8_t doorfind(uint16_t x, uint16_t y, uint8_t keytype)
{
    uint8_t r;
    BANK_ENTER(PAGE_DOORS);
    r = doorfind_b(x, y, keytype);
    BANK_LEAVE();
    return r;
}
void xfer(uint16_t x, uint16_t y, uint8_t flag) { BANK_ENTER(PAGE_DOORS); xfer_b(x, y, flag); BANK_LEAVE(); }
void find_place(uint8_t flag) { BANK_ENTER(PAGE_LOGIC); find_place_b(flag); BANK_LEAVE(); }

void do_objects(void) { BANK_ENTER(PAGE_OBJECTS); do_objects_b(); BANK_LEAVE(); }
void leave_item(uint8_t i, uint8_t object) { BANK_ENTER(PAGE_OBJECTS); leave_item_b(i, object); BANK_LEAVE(); }
void change_object(uint8_t id, uint8_t flag) { BANK_ENTER(PAGE_OBJECTS); change_object_b(id, flag); BANK_LEAVE(); }

static uint8_t ob_field(uint8_t list, uint8_t idx, uint8_t field, uint8_t write, uint16_t value)
{
    uint8_t r;
    BANK_ENTER(PAGE_OBJECTS);
    r = ob_field_b(list, idx, field, write, value);
    BANK_LEAVE();
    return r;
}
uint8_t ob_stat_get(uint8_t list, uint8_t idx) { return ob_field(list, idx, 3, 0, 0); }
void ob_stat_set(uint8_t list, uint8_t idx, uint8_t stat) { ob_field(list, idx, 3, 1, stat); }
uint8_t ob_id_get(uint8_t list, uint8_t idx) { return ob_field(list, idx, 2, 0, 0); }
void ob_id_set(uint8_t list, uint8_t idx, uint8_t id) { ob_field(list, idx, 2, 1, id); }
void ob_pos_set(uint8_t list, uint8_t idx, uint16_t x, uint16_t y)
{
    ob_field(list, idx, 0, 1, x);
    ob_field(list, idx, 1, 1, y);
}

/* ---- shared helpers ---- */

/* fmain2.c:1560 */
void move_extent(uint8_t e, uint16_t x, uint16_t y)
{
    struct extent *ex = &extent_list[e];
    ex->x1 = x - 250;
    ex->y1 = y - 200;
    ex->x2 = x + 250;
    ex->y2 = y + 200;
}

/* fmain.c:22 - cfile_entry, image_base, can_talk */
static const int8_t setfig_table[14][3] = {
    { 13,0,1 },  /* wizard */
    { 13,4,1 },  /* priest */
    { 14,0,0 },  /* guard */
    { 14,1,0 },  /* guard (back) */
    { 14,2,0 },  /* princess */
    { 14,4,1 },  /* king */
    { 14,6,0 },  /* noble */
    { 14,7,0 },  /* sorceress */
    { 15,0,0 },  /* bartender */
    { 16,0,0 },  /* witch */
    { 16,6,0 },  /* spectre */
    { 16,7,0 },  /* ghost */
    { 17,0,1 },  /* ranger */
    { 17,4,1 },  /* beggar */
};

int8_t setfig_image_base(uint8_t id) { return setfig_table[id][1]; }
uint8_t setfig_cfile(uint8_t id) { return (uint8_t)setfig_table[id][0]; }

/* fmain2.c:367 */
uint16_t pagecolors[32] = {
    0x0000,0x0FFF,0x0E96,0x0B63,0x0631,0x07BF,0x0333,0x0DB8,
    0x0223,0x0445,0x0889,0x0BBC,0x0521,0x0941,0x0F82,0x0FC7,
    0x0040,0x0070,0x00B0,0x06F6,0x0005,0x0009,0x000D,0x037F,
    0x0C00,0x0F50,0x0FA0,0x0FF6,0x0EB6,0x0EA5,0x000F,0x0BDF };

static uint16_t fader[32];

/* fmain2.c:377 */
void fade_page(int16_t r, int16_t g, int16_t b, uint8_t limit)
{
    int16_t r1, g1, b1, g2;
    uint8_t i;

    if (region_num == 4) pagecolors[31] = 0x0980;
    else if (region_num == 9) pagecolors[31] = secret_timer ? 0x00f0 : 0x0445;
    else pagecolors[31] = 0x0bdf;

    if (r > 100) r = 100;
    if (g > 100) g = 100;
    if (b > 100) b = 100;
    if (limit) {
        if (r < 10) r = 10;                     /* night limits */
        if (g < 25) g = 25;
        if (b < 60) b = 60;
        g2 = (100 - g) / 3;
    } else {
        if (r < 0) r = 0;
        if (g < 0) g = 0;
        if (b < 0) b = 0;
        g2 = 0;
    }
    for (i = 0; i < 32; i++) {
        r1 = (pagecolors[i] & 0x0f00) >> 4;
        g1 = pagecolors[i] & 0x00f0;
        b1 = pagecolors[i] & 0x000f;
        if (light_timer && r1 < g1) r1 = g1;
        r1 = (r * r1) / 1600;
        g1 = (g * g1) / 1600;
        b1 = (b * b1 + g2 * g1) / 100;
        if (limit) {
            if (i >= 16 && i <= 24 && g > 20) {
                if (g < 50) b1 += 2; else if (g < 75) b1++;
            }
            if (b1 > 15) b1 = 15;
        }
        fader[i] = (r1 << 8) + (g1 << 4) + b1;
    }
    palette_load_rgb4(0, fader, 32, 100);
    sprites_palette(fader, 100);
}

/* fmain2.c:1653 */
void day_fade(void)
{
    int16_t ll = light_timer ? 200 : 0;
    if ((daynight & 3) == 0 || viewstatus > 97) {
        if (region_num < 8)                     /* no night inside buildings */
            fade_page(lightlevel - 80 + ll, lightlevel - 61, lightlevel - 62, TRUE);
        else
            fade_page(100, 100, 100, TRUE);
    }
}

void load_all(void)
{
    while (new_region < NO_REGION)
        load_new_region();
    view_invalidate();
}
