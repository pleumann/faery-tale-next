/* The Faery Tale Adventure - ZX Spectrum Next port.
   M1 check: intro picture fade, then all tile banks from TILES.DAT shown in turn. */

#include <stdint.h>
#include <z80.h>
#include <errno.h>
#include <string.h>
#include <arch/zxn.h>
#include "platform/hw.h"
#include "platform/layer2.h"
#include "platform/files.h"
#include "assets.h"
#include "narr.h"

static uint16_t pal[32];

/* fmain2.c:367 */
static const uint16_t pagecolors[32] = {
    0x0000,0x0FFF,0x0E96,0x0B63,0x0631,0x07BF,0x0333,0x0DB8,
    0x0223,0x0445,0x0889,0x0BBC,0x0521,0x0941,0x0F82,0x0FC7,
    0x0040,0x0070,0x00B0,0x06F6,0x0005,0x0009,0x000D,0x037F,
    0x0C00,0x0F50,0x0FA0,0x0FF6,0x0EB6,0x0EA5,0x000F,0x0BDF };

#define TILE_PAGES 40               /* 4 pages for one tile bank (32K) */
#define SRC  ((uint8_t *)0x4000)    /* slot 2 */
#define DST  ((uint8_t *)0x6000)    /* slot 3 */

static void layer2_clear(uint8_t colour)
{
    for (uint8_t p = 0; p < 10; p++) {
        mmu_set(3, L2_PAGE_A + p);
        memset(DST, colour, 8192);
    }
    mmu_set(3, 11);
}

/* Draw the 64 tiles of the loaded bank as 16 x 4 grid at (32, 64). Tiles are 16x32,
   column-major (512 bytes), Layer 2 is column-major too: 32 bytes per tile column. */
static void show_bank(void)
{
    for (uint8_t t = 0; t < 64; t++) {
        uint16_t src = (uint16_t)t * TILE_BYTES;          /* offset in 32K bank */
        uint16_t x = 32 + (t & 15) * 16;
        uint8_t y = 64 + (t >> 4) * 32;
        for (uint8_t c = 0; c < 16; c++, x++, src += 32) {
            mmu_set(2, TILE_PAGES + (src >> 13));
            mmu_set(3, L2_PAGE_A + (x >> 5));
            memcpy(DST + ((x & 31) << 8) + y, SRC + (src & 0x1FFF), 32);
        }
    }
    mmu_set(2, 10);
    mmu_set(3, 11);
}

/* Fade like fade_down()/fade_normal() in fmain2.c: 5% steps, one per frame. */
static void fade(uint8_t from, uint8_t to)
{
    int8_t step = (to > from) ? 5 : -5;
    uint8_t p = from;
    for (;;) {
        wait_frame();
        palette_load_rgb4(0, pal, 32, p);
        if (p == to)
            break;
        p += step;
    }
}

/* Fatal error: hide Layer 2, show the ULA border in an error colour and
   the errno value as 8 attribute cells in the top ULA row (white = 1). */
static void fail(uint8_t colour)
{
    uint8_t e = errno;
    uint8_t *attr = (uint8_t *)0x5800;
    IO_LAYER_2_CONFIG = 0;
    nextreg(NR_ULA_CTRL, 0x00);
    mmu_set(WINDOW_SLOT, 10);
    memset((uint8_t *)0x4000, 0, 6144);
    memset(attr, 0, 768);
    /* 8 bits as 3x3-cell blocks across row 10: white = 1, blue = 0 */
    for (uint8_t i = 0; i < 8; i++, e <<= 1)
        for (uint8_t y = 0; y < 3; y++)
            for (uint8_t x = 0; x < 3; x++)
                attr[(10 + y) * 32 + i * 4 + x] = (e & 0x80) ? 0x78 : 0x08;
    z80_outp(0xFE, colour);
    for (;;)
        ;
}

void main(void)
{
    nextreg(NR_CPU_SPEED, 3);           /* 28 MHz */
    irq_init();
    layer2_init();
    palette_load_rgb4(0, pal, 32, 0);   /* black while loading */

    if (!file_load("FTA/PAGE0.PAL", pal, sizeof(pal))) fail(2);
    if (!file_load_pages("FTA/PAGE0.L2", L2_PAGE_A)) fail(6);

    fade(0, 100);
    for (uint8_t i = 0; i < 100; i++)
        wait_frame();
    fade(100, 0);

    memcpy(pal, pagecolors, sizeof(pal));
    for (;;) {
        for (uint8_t bank = 0; bank < 17; bank++) {
            uint8_t h = file_open("FTA/TILES.DAT");
            if (h == FILE_ERROR) fail(3);
            if (file_seek(h, (uint32_t)bank * TILE_BANK_BYTES)) fail(4);
            if (file_read_pages(h, TILE_PAGES, TILE_BANK_BYTES) != TILE_BANK_BYTES) fail(5);
            file_close(h);
            layer2_clear(0);
            show_bank();
            palette_load_rgb4(0, pal, 32, 100);
            for (uint8_t i = 0; i < 100; i++)
                wait_frame();
        }
        if (narr_data[0] != '%') fail(1);     /* generated text is linked in */
    }
}
