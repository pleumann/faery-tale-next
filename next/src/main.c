/* The Faery Tale Adventure - ZX Spectrum Next port.
   M4: game logic - enemies, AI, combat, objects, doors, day and night.

   Keys: Q/A/O/P or cursor keys (5-8) or joystick to walk, SPACE / fire to fight.
   Without input Julian walks by himself (demo), any key takes over. */

#include <stdint.h>
#include <z80.h>
#include <errno.h>
#include <string.h>
#include <arch/zxn.h>
#include "platform/hw.h"
#include "platform/layer2.h"
#include "platform/files.h"
#include "platform/input.h"
#include "game/world.h"
#include "game/view.h"
#include "game/actors.h"
#include "game/sprites.h"
#include "game/shapes.h"
#include "game/state.h"
#include "game/banked.h"
#include "game/ui.h"
#include "game/menus.h"
#include "game/audio.h"
#include "game/placard.h"
#include "game/intro.h"

/* Fatal error: hide Layer 2, show the ULA border in an error colour and
   the errno value as 8 attribute blocks (white = 1, MSB left). */
static void fail(uint8_t colour)
{
    uint8_t e = errno;
    uint8_t *attr = (uint8_t *)0x5800;
    IO_LAYER_2_CONFIG = 0;
    nextreg(NR_ULA_CTRL, 0x00);
    mmu_set(WINDOW_SLOT, 10);
    memset((uint8_t *)0x4000, 0, 6144);
    memset(attr, 0, 768);
    for (uint8_t i = 0; i < 8; i++, e <<= 1)
        for (uint8_t y = 0; y < 3; y++)
            for (uint8_t x = 0; x < 3; x++)
                attr[(10 + y) * 32 + i * 4 + x] = (e & 0x80) ? 0x78 : 0x08;
    z80_outp(0xFE, colour);
    for (;;)
        ;
}

/* Demo without input: Julian stands at the stone ring east of the village (objects on
   the ground), ogres are summoned, he fights when one comes close. */
static const uint8_t demo[][2] = {
    { 9, 50 }, { 5, 10 }, { 3, 10 }, { 9, 100 }, { 7, 10 }, { 1, 10 },
};

/* com2 in fsubs.asm: directions from the stick, index 4 + 3*y + x */
static const uint8_t com2[9] = { 0, 1, 2, 7, 9, 3, 6, 5, 4 };

#ifdef DEBUG_DUMP
#include <arch/zxn/esxdos.h>
extern uint8_t dbg_frame[20];
static uint8_t dbg_h = 0xFF;
/* time stamps in scan lines: frames * 311 + current line (NR 0x1E/0x1F) */
static uint16_t tstamp[10];
static uint16_t now(void)
{
    uint16_t f = frames;
    uint16_t line = ((uint16_t)(nextreg_read(0x1E) & 1) << 8) | nextreg_read(0x1F);
    return f * 311 + line;
}
#define TS(n) tstamp[n] = now()
static void debug_dump(void)
{
    if (dbg_h == 0xFF) {
        mmu_set(0, 0xFF);
        dbg_h = esx_f_open("DEBUG.BIN", ESX_MODE_W | ESX_MODE_OPEN_CREAT_TRUNC);
    }
    esx_f_write(dbg_h, (void *)&frames, 2);
    esx_f_write(dbg_h, tstamp, sizeof(tstamp));
    esx_f_write(dbg_h, &cycle, 1);
    esx_f_write(dbg_h, &anix, 1);
    esx_f_write(dbg_h, anim_list, sizeof(anim_list));
    esx_f_write(dbg_h, dbg_frame, 20);
    {   /* AY 1 and AY 2 registers 0..10 */
        uint8_t ayr[22], c, r;
        crit_enter();                   /* the music player writes the AY in the ISR */
        for (c = 0; c < 2; c++) {
            z80_outp(0xFFFD, c ? 0xFE : 0xFF);
            for (r = 0; r < 11; r++) { z80_outp(0xFFFD, r); ayr[c * 11 + r] = z80_inp(0xFFFD); }
        }
        z80_outp(0xFFFD, 0xFF);
        crit_leave();
        esx_f_write(dbg_h, ayr, 22);
    }
}
#endif

void main(void)
{
    uint8_t demo_on, demo_step, demo_count, last_region, h, dir, fire;
    uint16_t next_frame, img_x, img_y;
    uint16_t walk_x = 0, walk_y = 0;
    uint8_t walk_to = 0;

    nextreg(NR_CPU_SPEED, 3);           /* 28 MHz */
    ui_font_from_rom();                 /* while the ROM is still at 0x0000 */
    irq_init();
    layer2_init();
    view_init();
    world_init();
    sprites_init();
    shapes_init();

    h = file_open("FTA/TILES.DAT");
    if (h == FILE_ERROR) fail(2);
    file_close(h);

    ui_init();
    audio_init();
    file_idle_hook = audio_update;
    font_load();
    pointer_init();

    for (;;) {                          /* one game per pass */
#if defined(DEMO) || defined(TEST_DOOR) || defined(TEST_KEYS) || defined(DEBUG_DUMP) || defined(TEST_FEATURES) || defined(TEST_RESCUE) || defined(TEST_BRIDGE)
    demo_on = 1;
#else
    demo_on = 0;
    intro();                            /* title and book; seeds the random numbers */
#endif
    demo_step = demo_count = 0;
    img_x = img_y = 0xFFFF;
    quitflag = 0;
    menus_start();
    game_start();                       /* Julian, start village (revive) */
#ifdef TEST_DOOR
    xfer(0x49d0 + 3, 0x3dc0 + 20, TRUE);    /* test: on the threshold of village house #1 */
    hero_x = anim_list[0].abs_x = 0x49d0 + 3;
    hero_y = anim_list[0].abs_y = 0x3dc0 + 20;
    demo_on = 0;
#endif
#ifdef TEST_BRIDGE
    xfer(18304, 16000, TRUE);           /* test: on the bridge west of Tambry (sector 48) */
    demo_on = 0;
#endif
#ifdef TEST_REGION
    xfer(16700, 15500, TRUE);           /* test: walk west across the region 3/2 border */
#else
    if (demo_on) xfer(21600, 15500, TRUE);  /* demo: stone ring */
#endif
    next_frame = frames;

    while (!quitflag) {
        uint8_t d = input_dirs();
        int8_t jx = 0, jy = 0;

        if (d & IN_LEFT) jx = -1;
        if (d & IN_RIGHT) jx = 1;
        if (d & IN_UP) jy = -1;
        if (d & IN_DOWN) jy = 1;
        dir = com2[4 + 3 * jy + jx];
        fire = (d & IN_FIRE) ? 1 : 0;

        /* mouse (fmain.c decode_mouse / HandlerInterface): click on menu entries, hold
           the left button on the compass to walk, the right button to fight */
        mouse_update();
        if (mouse_present) {
            static uint8_t last_buttons;
            uint8_t pressed = mouse_buttons & ~last_buttons;
            last_buttons = mouse_buttons;
            if (mouse_y >= 168) {
                uint8_t row = (mouse_y - 168) >> 3;
                if ((pressed & MOUSE_LEFT) && row < 6) {
                    if (mouse_x >= 212 && mouse_x < 236) { menu_click(row * 2); demo_on = 0; }
                    else if (mouse_x >= 240 && mouse_x < 264) { menu_click(row * 2 + 1); demo_on = 0; }
                }
                if ((mouse_buttons & MOUSE_LEFT) && mouse_x >= 283 && mouse_x < 308 &&
                    mouse_y >= 183 && mouse_y < 207)
                    dir = com2[4 + 3 * ((mouse_y - 183) / 8 - 1) + ((mouse_x - 283) / 8 - 1)];
            }
            if (mouse_buttons & MOUSE_RIGHT) fire = 1;

            /* Port addition (not in the original): click or hold the left button on the
               playfield to walk there - diagonally while both dx and dy are left */
            if ((mouse_buttons & MOUSE_LEFT) && mouse_y < VIEW_Y + VIEW_H &&
                mouse_x >= VIEW_X && mouse_x < VIEW_X + VIEW_W) {
                walk_x = map_x + (mouse_x - VIEW_X);
                walk_y = map_y + (mouse_y - VIEW_Y);
                walk_to = 1;
                demo_on = 0;
            }
            pointer_set(mouse_x, mouse_y, 1);
        }
        if (dir != 9 && !(mouse_buttons & MOUSE_LEFT))
            walk_to = 0;                        /* keys and joystick take over */
        if (walk_to && dir == 9) {
            int16_t dx = (int16_t)(walk_x - hero_x), dy = (int16_t)(walk_y - hero_y);
            int8_t xd = 0, yd = 0;
            if (dx > 3) xd = 1; else if (dx < -3) xd = -1;
            if (dy > 2) yd = 1; else if (dy < -2) yd = -1;
            if ((xd == 0 && yd == 0) || frustflag > 20)
                walk_to = 0;                    /* arrived, or stuck */
            else
                dir = com2[4 + 3 * yd + xd];
        }
        if (dir != 9 || fire)
            demo_on = 0;
#ifdef TEST_REGION
        if (demo_on) { dir = 7; fire = 0; } else
#endif
        if (demo_on) {
            dir = demo[demo_step][0];
            fire = (nearest_person && perdist < 30 && anim_list[nearest_person].type == ENEMY);
            if (fire) dir = 9;
            else if (++demo_count == demo[demo_step][1]) {
                demo_count = 0;
                if (++demo_step == sizeof(demo) / sizeof(demo[0]))
                    demo_step = 0;
            }
        }

#ifndef TEST_REGION
        if (demo_on && cycle == 60) debug_encounter();
#endif
        if (key_down(0xFDFE, 0x04) && key_down(0x7FFE, 0x02)) debug_encounter();  /* SYM+D: ogres (test) */

        {
            static uint8_t oldir = 0xFF;
            uint8_t key = input_key();
#ifdef TEST_FEATURES
            /* scripted checks: magic map, then the witch's tower */
            { static uint16_t loops; loops++;
              if (loops == 2) { stuff[13] = 3; stuff[7] = 1; set_options(); }
              if (loops == 30) key = 14;         /* SYMBOL SHIFT + 5: bird totem = map */
              if (loops == 120) key = 'X';
              if (loops == 140) { new_region = 8; xfer(11013, 36804 + 60, FALSE); find_place(2); }
            }
#endif
#ifdef TEST_RESCUE
            /* scripted check: walk into the princess's cell (extent 83) */
            { static uint16_t loops; loops++;
              if (loops == 2) { new_region = 8; xfer(10850, 35660, FALSE); find_place(2); } }
#endif
#ifdef TEST_KEYS
            /* scripted key presses for automated checks: Items/List, then any key */
            { static uint16_t loops; loops++;
              if (loops == 40) key = 'I';         /* inventory before the ogres come */
              if (loops == 140) key = 'X'; }
#endif
            if (key) {
                demo_on = 0;
                menu_key(key);
            }
            if (dir != oldir) { drawcompass(dir); oldir = dir; }
        }

        if (viewstatus == 2) { delay(200); viewstatus = 99; }

        /* inventory picture or pause: wait for the next key (fmain.c:1365) */
        if (viewstatus == 4 || viewstatus == 1 || paused()) {
            if (viewstatus == 4 || viewstatus == 1) sprites_hide();
            if (viewstatus == 1) {                      /* the map: colour 31 flashes */
                static const uint16_t white = 0xFFF, black = 0;
                palette_load_rgb4(31, (frames & 16) ? &white : &black, 1, 100);
            }
            next_frame = frames + 2;
            while ((int16_t)(frames - next_frame) < 0)
                audio_update();
            continue;
        }
        if (viewstatus == 99) view_invalidate();

#ifdef DEBUG_DUMP
        TS(0);
#endif
        hero_control(dir, fire);
        actors_update();
#ifdef DEBUG_DUMP
        TS(1);
#endif
        door_check();

        last_region = region_num;
        gen_mini();
        if (region_num != last_region)
            view_invalidate();

        /* the slow world logic runs only on frames without a tile scroll (fmain.c:2007) */
        if (viewstatus == 99 || viewstatus == 98 || viewstatus == 3 ||
            ((map_x >> 4) == img_x && (map_y >> 5) == img_y)) {
            if (viewstatus == 99) viewstatus = 98;
            else if (viewstatus == 98 || viewstatus == 3) viewstatus = 0;
            tick_logic();
        }
        img_x = map_x >> 4;
        img_y = map_y >> 5;
#ifdef DEBUG_DUMP
        TS(2);
#endif
        world_prefetch();
        combat_update();
#ifdef DEBUG_DUMP
        TS(3);
#endif
        actors_sort();
#ifdef DEBUG_DUMP
        TS(4);
#endif
        witch_undo();
        view_update();
        witch_draw();
#ifdef DEBUG_DUMP
        TS(5);
#endif
        sprites_draw();
#ifdef DEBUG_DUMP
        TS(6);
#endif

        /* 25 Hz game loop; scroll registers and sprite attributes change in the vblank */
        next_frame += 2;
        while ((int16_t)(frames - next_frame) < 0)
            audio_update();
        audio_update();
        view_scroll();
        sprites_flush();
#ifdef DEBUG_DUMP
        if (frames > 500 && frames < 1500) debug_dump();
        if (frames >= 1500 && dbg_h != 0xFF && dbg_h != 0xFE) { esx_f_close(dbg_h); dbg_h = 0xFE; }
#endif
        cycle++;
    }
    if (leave_program) {
        /* Quit: there is no way back to BASIC (ROM, bank 5 and system variables are in
           use), so do what the user expects from leaving a program on the Next: a soft
           reset into the NextZXOS menu (NR 0x02 bit 0) */
        fade_down();
        nextreg(0x02, 0x01);
        for (;;)
            ;
    }
    }
}
