/* Resident part of the game: wrappers into banked modules and small shared helpers. */
#include <stdint.h>
#include "../platform/bank.h"
#include "../platform/layer2.h"
#include "state.h"
#include "world.h"
#include "view.h"
#include "sprites.h"
#include "banked.h"
#include "actors.h"
#include "menus.h"
#include "audio.h"
#include "placard.h"
#include "intro.h"
#include "assets.h"
#include "../platform/files.h"

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

void hero_control_b(uint8_t dir, uint8_t fire);
void actors_update_b(void);
void combat_update_b(void);
void actors_sort_b(void);
void hero_control(uint8_t dir, uint8_t fire) { BANK_ENTER(PAGE_MOVE); hero_control_b(dir, fire); BANK_LEAVE(); }
void actors_update(void) { BANK_ENTER(PAGE_MOVE); actors_update_b(); BANK_LEAVE(); }
void combat_update(void) { BANK_ENTER(PAGE_COMBAT); combat_update_b(); BANK_LEAVE(); }
void witch_undo_b(void);
void witch_draw_b(void);
void witch_undo(void) { BANK_ENTER(PAGE_COMBAT); witch_undo_b(); BANK_LEAVE(); }
void witch_draw(void) { BANK_ENTER(PAGE_COMBAT); witch_draw_b(); BANK_LEAVE(); }
void actors_sort(void) { BANK_ENTER(PAGE_COMBAT); actors_sort_b(); BANK_LEAVE(); }

void sprites_init_b(void);
void sprites_palette_b(const uint16_t *rgb4, uint8_t pct);
void sprites_draw_b(void);
void sprites_flush_b(void);
void sprites_init(void) { BANK_ENTER(PAGE_DRAW); sprites_init_b(); BANK_LEAVE(); }
void sprites_palette(const uint16_t *rgb4, uint8_t pct) { BANK_ENTER(PAGE_DRAW); sprites_palette_b(rgb4, pct); BANK_LEAVE(); }
void sprites_draw(void) { BANK_ENTER(PAGE_DRAW); sprites_draw_b(); BANK_LEAVE(); }
void sprites_flush(void) { BANK_ENTER(PAGE_DRAW); sprites_flush_b(); BANK_LEAVE(); }
void sprites_hide_b(void);
void pointer_init_b(void);
void pointer_set_b(uint16_t x, uint8_t y, uint8_t visible);
void pointer_init(void) { BANK_ENTER(PAGE_DRAW); pointer_init_b(); BANK_LEAVE(); }
void pointer_set(uint16_t x, uint8_t y, uint8_t v) { BANK_ENTER(PAGE_DRAW); pointer_set_b(x, y, v); BANK_LEAVE(); }
void sprites_hide(void) { BANK_ENTER(PAGE_DRAW); sprites_hide_b(); BANK_LEAVE(); }

void menu_key_b(uint8_t key);
void print_options_b(void);
void set_options_b(void);
uint8_t paused_b(void);
void menus_start_b(void);
void show_inventory_b(void);
void get_turtle_b(void);
void menu_key(uint8_t key) { BANK_ENTER(PAGE_MENU); menu_key_b(key); BANK_LEAVE(); }
void menu_click_b(uint8_t slot);
void menu_click(uint8_t slot) { BANK_ENTER(PAGE_MENU); menu_click_b(slot); BANK_LEAVE(); }
void print_options(void) { BANK_ENTER(PAGE_MENU); print_options_b(); BANK_LEAVE(); }
void set_options(void) { BANK_ENTER(PAGE_MENU); set_options_b(); BANK_LEAVE(); }
uint8_t paused(void) { uint8_t r; BANK_ENTER(PAGE_MENU); r = paused_b(); BANK_LEAVE(); return r; }
void menus_start(void) { BANK_ENTER(PAGE_MENU); menus_start_b(); BANK_LEAVE(); }
void show_inventory(void) { BANK_ENTER(PAGE_MENU); show_inventory_b(); BANK_LEAVE(); }
void get_turtle(void) { BANK_ENTER(PAGE_LOGIC); get_turtle_b(); BANK_LEAVE(); }

void audio_init_b(void);
void audio_tick_b(void);
void effect_b(uint8_t num, uint16_t period);
void setmood_b(uint8_t now);
void audio_options_b(uint8_t music, uint8_t sound);
void stopscore_b(void);

uint8_t music_irq_on;
/* calls into the player from the main program: interrupts off meanwhile */
#define AUDIO_CALL(x) do { crit_enter(); { BANK_ENTER(PAGE_AUDIO); x; BANK_LEAVE(); } crit_leave(); } while (0)

void audio_init(void)
{
    uint8_t h = file_open("FTA/MUSIC.DAT");
    if (h != FILE_ERROR) {
        file_read_pages(h, 76, MUSIC_SIZE);
        file_close(h);
    }
    AUDIO_CALL(audio_init_b());
    music_irq_on = 1;
}

/* The music player runs in the frame interrupt (irq.asm calls music_isr). It maps its
   code page into slot 0 and its data into slot 1 and restores both afterwards; during
   esxDOS calls it skips (the ROM and DivMMC must stay mapped). */
extern uint8_t in_esx;

void music_isr(void)
{
    uint8_t m0, m1;
    if (!music_irq_on || in_esx) return;
    m0 = nextreg_read(NR_MMU0);
    m1 = nextreg_read(NR_MMU0 + 1);
    mmu_set(0, PAGE_AUDIO);
    audio_tick_b();
    mmu_set(0, m0);
    mmu_set(1, m1);
}

void audio_update(void) { }             /* the player runs in the interrupt now */

void effect(uint8_t num, uint16_t period) { AUDIO_CALL(effect_b(num, period)); }
void setmood(uint8_t now) { AUDIO_CALL(setmood_b(now)); }
void audio_options(uint8_t music, uint8_t sound) { AUDIO_CALL(audio_options_b(music, sound)); }
void stopscore(void) { AUDIO_CALL(stopscore_b()); }

void music_play_b(uint8_t off);
void music_play(uint8_t off) { AUDIO_CALL(music_play_b(off)); }

void font_load_b(void);
void map_message_b(void);
void message_off_b(void);
void placard_text_b(uint8_t n);
void placard_name_b(void);
void placard_ssp_b(const char *s);
void placard_pen_b(uint8_t c);
void placard_b(void);
void placard_clear_b(void);
void font_load(void) { BANK_ENTER(PAGE_PLACARD); font_load_b(); BANK_LEAVE(); }
void map_message(void) { BANK_ENTER(PAGE_PLACARD); map_message_b(); BANK_LEAVE(); }
void message_off(void) { BANK_ENTER(PAGE_PLACARD); message_off_b(); BANK_LEAVE(); }
void placard_text(uint8_t n) { BANK_ENTER(PAGE_PLACARD); placard_text_b(n); BANK_LEAVE(); }
void placard_name(void) { BANK_ENTER(PAGE_PLACARD); placard_name_b(); BANK_LEAVE(); }
void placard_pen(uint8_t c) { BANK_ENTER(PAGE_PLACARD); placard_pen_b(c); BANK_LEAVE(); }
void placard_center_b(uint8_t on);
void placard_center(uint8_t on) { BANK_ENTER(PAGE_PLACARD); placard_center_b(on); BANK_LEAVE(); }
void placard(void) { BANK_ENTER(PAGE_PLACARD); placard_b(); BANK_LEAVE(); }
void bigdraw_b(void);
void bigdraw(void) { BANK_ENTER(PAGE_PLACARD); bigdraw_b(); BANK_LEAVE(); }
void placard_clear(void) { BANK_ENTER(PAGE_PLACARD); placard_clear_b(); BANK_LEAVE(); }
void placard_ssp(const char *s)
{
    static char buf[200];               /* the string may live in the caller's page */
    uint8_t i = 0;
    while (s[i] && i < sizeof(buf) - 1) { buf[i] = s[i]; i++; }
    buf[i] = 0;
    { BANK_ENTER(PAGE_PLACARD); placard_ssp_b(buf); BANK_LEAVE(); }
}

void intro_b(void);
void rescue_b(void);
void win_b(void);
void intro(void) { BANK_ENTER(PAGE_INTRO); intro_b(); BANK_LEAVE(); }
void rescue(void) { BANK_ENTER(PAGE_INTRO); rescue_b(); BANK_LEAVE(); }
void win(void) { BANK_ENTER(PAGE_INTRO); win_b(); BANK_LEAVE(); }

/* Amiga Delay(): ticks of 1/50 s; the music keeps playing */
void delay(uint16_t ticks)
{
    uint16_t end = frames + ticks;
    while ((int16_t)(frames - end) < 0)
        audio_update();
}

/* fmain2.c:623 */
void fade_down(void)
{
    int8_t i;
    for (i = 100; i >= 0; i -= 5) { fade_page(i, i, i, FALSE); delay(1); }
}

void fade_normal(void)
{
    int8_t i;
    for (i = 0; i <= 100; i += 5) { fade_page(i, i, i, FALSE); delay(1); }
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
uint8_t setfig_can_talk(uint8_t id) { return (uint8_t)setfig_table[id][2]; }

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
    {   /* the day/night fade calls this often with the same values */
        static int16_t lr = -1000, lg, lb;
        static uint8_t ll, llt, lreg;
        static uint16_t l31;
        if (!palette_dirty && r == lr && g == lg && b == lb && limit == ll && (light_timer != 0) == llt &&
            pagecolors[31] == l31 && region_num == lreg)
            return;
        lr = r; lg = g; lb = b; ll = limit; llt = light_timer != 0; l31 = pagecolors[31]; lreg = region_num;
    }
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
    palette_dirty = 0;
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
