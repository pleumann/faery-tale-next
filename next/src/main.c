/* The Faery Tale Adventure - ZX Spectrum Next port.
   M0: platform skeleton - IM2 interrupt, Layer 2 320x256, palette fades, SD loading. */

#include <stdint.h>
#include <z80.h>
#include <errno.h>
#include <string.h>
#include <arch/zxn.h>
#include "platform/hw.h"
#include "platform/layer2.h"
#include "platform/files.h"

static uint16_t pal[32];

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

    for (;;) {
        fade(0, 100);
        for (uint8_t i = 0; i < 100; i++)
            wait_frame();
        fade(100, 0);
    }
}
