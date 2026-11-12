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
static void debug_dump(void)
{
    if (dbg_h == 0xFF) {
        mmu_set(0, 0xFF);
        dbg_h = esx_f_open("DEBUG.BIN", ESX_MODE_W | ESX_MODE_OPEN_CREAT_TRUNC);
    }
    esx_f_write(dbg_h, &cycle, 1);
    esx_f_write(dbg_h, &anix, 1);
    esx_f_write(dbg_h, anim_list, sizeof(anim_list));
    esx_f_write(dbg_h, dbg_frame, 20);
}
#endif

void main(void)
{
    uint8_t demo_on = 1, demo_step = 0, demo_count = 0, last_region, h, dir, fire;
    uint16_t next_frame, img_x = 0xFFFF, img_y = 0xFFFF;

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
    seed1 = 12345;                      /* TODO (M7): seed from the title screen */
    menus_start();
    game_start();                       /* Julian, start village (revive) */
#ifdef TEST_DOOR
    xfer(0x49d0 + 3, 0x3dc0 + 20, TRUE);    /* test: on the threshold of village house #1 */
    hero_x = anim_list[0].abs_x = 0x49d0 + 3;
    hero_y = anim_list[0].abs_y = 0x3dc0 + 20;
    demo_on = 0;
#else
    xfer(21600, 15500, TRUE);           /* demo: stone ring */
#endif
    next_frame = frames;

    for (;;) {
        uint8_t d = input_dirs();
        int8_t jx = 0, jy = 0;

        if (d & IN_LEFT) jx = -1;
        if (d & IN_RIGHT) jx = 1;
        if (d & IN_UP) jy = -1;
        if (d & IN_DOWN) jy = 1;
        dir = com2[4 + 3 * jy + jx];
        fire = (d & IN_FIRE) ? 1 : 0;
        if (dir != 9 || fire)
            demo_on = 0;
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

        if (demo_on && cycle == 60) debug_encounter();
        if (key_down(0xFDFE, 0x04) && key_down(0x7FFE, 0x02)) debug_encounter();  /* SYM+D: ogres (test) */

        {
            static uint8_t oldir = 0xFF;
            uint8_t key = input_key();
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

        /* inventory picture or pause: wait for the next key (fmain.c:1365) */
        if (viewstatus == 4 || viewstatus == 1 || paused()) {
            if (viewstatus == 4 || viewstatus == 1) sprites_hide();
            next_frame = frames + 2;
            while ((int16_t)(frames - next_frame) < 0)
                ;
            continue;
        }
        if (viewstatus == 99) view_invalidate();

        hero_control(dir, fire);
        actors_update();
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

        combat_update();
        actors_sort();
        view_update();
        sprites_draw();

        /* 25 Hz game loop; scroll registers and sprite attributes change in the vblank */
        next_frame += 2;
        while ((int16_t)(frames - next_frame) < 0)
            ;
        view_scroll();
        sprites_flush();
#ifdef DEBUG_DUMP
        if (frames > 500 && frames < 560) debug_dump();
        if (frames >= 560 && dbg_h != 0xFF) { esx_f_close(dbg_h); dbg_h = 0xFE; }
#endif
        cycle++;
    }
}
