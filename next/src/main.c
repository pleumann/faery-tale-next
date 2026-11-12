/* The Faery Tale Adventure - ZX Spectrum Next port.
   M2: map viewer - region loading, hardware-scrolled playfield.

   Keys: Q/A/O/P or cursor keys (5-8) or joystick to scroll, SPACE/fire = fast,
         R = back to the start village, I = inside buildings, K = dungeons.
   Without input the view pans by itself (demo), any direction key takes over. */

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
}

/* Jump to a place: (x, y) is the hero position, the camera centres on it. */
static void teleport(uint16_t x, uint16_t y, uint8_t region)
{
    map_x = x - 144;
    map_y = y - 70;
    new_region = region;
    load_new_region();
    gen_mini();
    set_region_palette();
    view_invalidate();
}

#define DEMO_INDOOR  127    /* special dx values: teleport, then hold */
#define DEMO_DUNGEON 126
#define DEMO_START   125

/* Demo pan: dx, dy per frame, number of frames. Holds of 250 frames at the start and
   after crossing into region 2 (3000 px west) make screenshots comparable. */
static const struct { int8_t dx, dy; uint8_t frames; } demo[] = {
    { 0, 0, 250 },
    { -8, 0, 125 }, { -8, 0, 125 }, { -8, 0, 125 }, { 0, 0, 250 },
    { 0, 4, 100 },
    { 8, 0, 125 }, { 8, 0, 125 }, { 8, 0, 125 }, { 0, -4, 100 },
    { DEMO_INDOOR, 0, 250 }, { 0, 4, 100 }, { DEMO_DUNGEON, 0, 250 }, { DEMO_START, 0, 1 },
};

void main(void)
{
    uint8_t demo_on = 1, demo_step = 0, demo_count = 0, last_region, h;

    nextreg(NR_CPU_SPEED, 3);           /* 28 MHz */
    irq_init();
    layer2_init();
    palette_load_rgb4(0, pagecolors, 32, 0);
    view_init();
    world_init();

    h = file_open("FTA/TILES.DAT");
    if (h == FILE_ERROR) fail(2);
    file_close(h);

    teleport(19036, 15755, 3);          /* start village, fmain.c:2853 */

    for (;;) {
        uint8_t d = input_dirs();
        uint8_t speed = (d & IN_FIRE) ? 16 : 4;

        if (key_down(0xFBFE, 0x08)) { demo_on = 0; teleport(19036, 15755, 3); }  /* R */
        if (key_down(0xDFFE, 0x04)) { demo_on = 0; teleport(5511, 33780, 8); }   /* I */
        if (key_down(0xBFFE, 0x04)) { demo_on = 0; teleport(5511, 33780, 9); }   /* K */

        if (d & (IN_UP | IN_DOWN | IN_LEFT | IN_RIGHT))
            demo_on = 0;
        if (demo_on) {
            int8_t dx = demo[demo_step].dx;
            if (demo_count == 0 && dx == DEMO_INDOOR) teleport(5511, 33780, 8);
            else if (demo_count == 0 && dx == DEMO_DUNGEON) teleport(5511, 33780, 9);
            else if (demo_count == 0 && dx == DEMO_START) teleport(19036, 15755, 3);
            if (dx < DEMO_START) {
                map_x += dx;
                map_y += demo[demo_step].dy;
            }
            if (++demo_count == demo[demo_step].frames) {
                demo_count = 0;
                if (++demo_step == sizeof(demo) / sizeof(demo[0]))
                    demo_step = 0;
            }
        } else {
            if (d & IN_LEFT) map_x -= speed;
            if (d & IN_RIGHT) map_x += speed;
            if (d & IN_UP) map_y -= speed;
            if (d & IN_DOWN) map_y += speed;
        }

        last_region = region_num;
        gen_mini();
        if (region_num != last_region) {
            set_region_palette();
            view_invalidate();
        }
        view_update();
    }
}
