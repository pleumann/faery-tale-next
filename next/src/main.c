/* The Faery Tale Adventure - ZX Spectrum Next port.
   M3: the hero walks through the world (hardware sprites, collision, occlusion).

   Keys: Q/A/O/P or cursor keys (5-8) or joystick to walk (8 directions),
         R = back to the start village, I = inside buildings, K = dungeons.
   Without input the hero walks by himself (demo), any direction key takes over. */

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

/* fmain2.c:367 - colour 31 is set per region in fade_page() */
static uint16_t pagecolors[32] = {
    0x0000,0x0FFF,0x0E96,0x0B63,0x0631,0x07BF,0x0333,0x0DB8,
    0x0223,0x0445,0x0889,0x0BBC,0x0521,0x0941,0x0F82,0x0FC7,
    0x0040,0x0070,0x00B0,0x06F6,0x0005,0x0009,0x000D,0x037F,
    0x0C00,0x0F50,0x0FA0,0x0FF6,0x0EB6,0x0EA5,0x000F,0x0BDF };

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

static void set_region_palette(void)
{
    if (region_num == 4) pagecolors[31] = 0x0980;
    else if (region_num == 9) pagecolors[31] = 0x0445;
    else pagecolors[31] = 0x0BDF;
    palette_load_rgb4(0, pagecolors, 32, 100);
    sprites_palette(pagecolors, 100);
}

/* Put the hero somewhere: the camera centres on it. */
static void teleport(uint16_t x, uint16_t y, uint8_t region)
{
    actors_init_hero(x, y);
    map_x = x - 144;
    map_y = y - 70;
    new_region = region;
    load_new_region();
    gen_mini();
    set_region_palette();
    view_invalidate();
}

/* Demo walk without input: direction (0 = NW .. 7 = W, 9 = stand), number of frames */
static const uint8_t demo[][2] = {
    { 9, 40 }, { 5, 30 }, { 3, 40 }, { 1, 40 }, { 7, 60 }, { 5, 20 }, { 4, 30 }, { 0, 30 },
};

/* com2 in fsubs.asm: directions from the stick, index 4 + 3*y + x */
static const uint8_t com2[9] = { 0, 1, 2, 7, 9, 3, 6, 5, 4 };

void main(void)
{
    uint8_t demo_on = 1, demo_step = 0, demo_count = 0, last_region, h, dir;
    uint16_t next_frame;

    nextreg(NR_CPU_SPEED, 3);           /* 28 MHz */
    irq_init();
    layer2_init();
    palette_load_rgb4(0, pagecolors, 32, 0);
    view_init();
    world_init();
    sprites_init();

    h = file_open("FTA/TILES.DAT");
    if (h == FILE_ERROR) fail(2);
    file_close(h);
    if (!sprites_load_set(0, SHAPE_PAGE_PHIL)) fail(3);     /* Julian */

    teleport(19036, 15755, 3);          /* start village, fmain.c:2853 */
    next_frame = frames;

    for (;;) {
        uint8_t d = input_dirs();
        int8_t jx = 0, jy = 0;

        if (key_down(0xFBFE, 0x08)) { demo_on = 0; teleport(19036, 15755, 3); }  /* R */
        if (key_down(0xDFFE, 0x04)) { demo_on = 0; teleport(5511, 33780, 8); }   /* I */
        if (key_down(0xBFFE, 0x04)) { demo_on = 0; teleport(5511, 33780, 9); }   /* K */

        if (d & IN_LEFT) jx = -1;
        if (d & IN_RIGHT) jx = 1;
        if (d & IN_UP) jy = -1;
        if (d & IN_DOWN) jy = 1;
        dir = com2[4 + 3 * jy + jx];
        if (dir != 9)
            demo_on = 0;
        if (demo_on) {
            dir = demo[demo_step][0];
            if (++demo_count == demo[demo_step][1]) {
                demo_count = 0;
                if (++demo_step == sizeof(demo) / sizeof(demo[0]))
                    demo_step = 0;
            }
        }

        hero_input(dir);
        actors_update();

        last_region = region_num;
        gen_mini();
        if (region_num != last_region) {
            set_region_palette();
            view_invalidate();
        }
        view_update();
        sprites_draw();

        /* 25 Hz game loop; scroll registers and sprite attributes change in the vblank */
        next_frame += 2;
        while ((int16_t)(frames - next_frame) < 0)
            ;
        view_scroll();
        sprites_flush();
        cycle++;
    }
}
