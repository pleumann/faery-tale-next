#include <arch/zxn.h>
#include <string.h>
#include "hw.h"
#include "layer2.h"

void layer2_init(void)
{
    nextreg(NR_L2_CTRL, 0x10);          /* 320x256x8, palette offset 0 */
    nextreg(NR_GLOBAL_TRANSP, 0xE3);
    nextreg(NR_FALLBACK, 0x00);
    nextreg(NR_ULA_CTRL, 0x80);         /* ULA output off */
    nextreg(NR_L2_XSCROLL_LSB, 0);
    nextreg(NR_L2_XSCROLL_MSB, 0);
    nextreg(NR_L2_YSCROLL, 0);
    layer2_clip(0, 0, 319, 255);
    layer2_show(L2_BANK_A);
    for (uint8_t p = 0; p < 10; p++) {          /* clear the buffer */
        mmu_set(3, L2_PAGE_A + p);
        memset((uint8_t *)0x6000, 0, 8192);
    }
    mmu_set(3, 11);
    IO_LAYER_2_CONFIG = 0x02;           /* Layer 2 visible */
}

void layer2_show(uint8_t bank16k)
{
    nextreg(NR_L2_ACTIVE_BANK, bank16k);
}

void layer2_clip(uint16_t x1, uint8_t y1, uint16_t x2, uint8_t y2)
{
    nextreg(NR_CLIP_CTRL, 0x01);        /* reset L2 clip index */
    nextreg(NR_CLIP_L2, (uint8_t)(x1 >> 1));
    nextreg(NR_CLIP_L2, (uint8_t)(x2 >> 1));
    nextreg(NR_CLIP_L2, y1);
    nextreg(NR_CLIP_L2, y2);
}

/* Scale a 4-bit Amiga channel by pct and reduce it to 3 bits. */
static uint8_t chan3(uint8_t c4, uint8_t pct)
{
    uint16_t v = (uint16_t)c4 * pct;    /* 0..1500 */
    return (uint8_t)((v * 7 + 750) / 1500);
}

uint8_t palette_dirty = 1;

void palette_load_rgb4(uint8_t first, const uint16_t *rgb4, uint8_t count, uint8_t pct)
{
    palette_dirty = 1;
    nextreg(NR_PAL_CTRL, 0x10);         /* write Layer 2 first palette */
    nextreg(NR_PAL_INDEX, first);
    while (count--) {
        uint16_t c = *rgb4++;
        uint8_t r = chan3((c >> 8) & 15, pct);
        uint8_t g = chan3((c >> 4) & 15, pct);
        uint8_t b = chan3(c & 15, pct);
        nextreg(NR_PAL_VALUE9, (r << 5) | (g << 2) | (b >> 1));
        nextreg(NR_PAL_VALUE9, b & 1);
    }
}
