#include <stdint.h>
#include <string.h>
#include "../platform/hw.h"
#include "../platform/layer2.h"
#include "world.h"
#include "view.h"

#define BUF_COLS    20          /* 320 / 16 */
#define BUF_ROWS    8           /* 256 / 32 */

#define SRC ((uint8_t *)0x4000) /* slot 2: tile page */
#define DST ((uint8_t *)0x6000) /* slot 3: Layer 2 page */

static uint16_t ox, oy;         /* tile coordinates of the buffered 20 x 8 window */
static uint8_t valid;

void view_init(void)
{
    layer2_clip(VIEW_X, (uint8_t)VIEW_Y, VIEW_X + VIEW_W - 1, (uint8_t)(VIEW_Y + VIEW_H - 1));
    valid = 0;
}

void view_invalidate(void)
{
    valid = 0;
}

/* Copy tile (imx, imy) into its torus position. */
static void draw_tile(uint16_t imx, uint16_t imy)
{
    uint8_t t = world_tile(imx, imy);
    uint16_t bx = (imx % BUF_COLS) * 16;
    uint8_t by = (uint8_t)((imy & (BUF_ROWS - 1)) * 32);
    const uint8_t *s;
    uint8_t *d;
    uint8_t c;

    mmu_set(2, TILE_PAGE + (t >> 4));
    mmu_set(3, L2_PAGE_A + (bx >> 5));
    s = SRC + ((uint16_t)(t & 15) << 9);
    d = DST + ((bx & 31) << 8) + by;
    for (c = 0; c < 16; c++) {
        memcpy(d, s, 32);
        s += 32;
        d += 256;
    }
}

static void draw_column(uint16_t imx)
{
    uint8_t r;
    for (r = 0; r < BUF_ROWS; r++)
        draw_tile(imx, oy + r);
}

static void draw_row(uint16_t imy)
{
    uint8_t c;
    for (c = 0; c < BUF_COLS; c++)
        draw_tile(ox + c, imy);
}

static void restore_slots(void)
{
    mmu_set(2, 10);
    mmu_set(3, 11);
}

void view_update(void)
{
    uint16_t vx0 = map_x >> 4, vx1 = (map_x + VIEW_W - 1) >> 4;
    uint16_t vy0 = map_y >> 5, vy1 = (map_y + VIEW_H - 1) >> 5;
    uint16_t sx, sy;

    if (!valid || vx0 + 2 < ox || vx0 > ox + 2 || vy0 + 2 < oy || vy0 > oy + 3) {
        uint8_t c;
        ox = vx0;
        oy = vy0;
        for (c = 0; c < BUF_COLS; c++)
            draw_column(ox + c);
        valid = 1;
    } else {
        while (vx0 < ox) { ox--; draw_column(ox); }
        while (vx1 > ox + BUF_COLS - 1) { ox++; draw_column(ox + BUF_COLS - 1); }
        while (vy0 < oy) { oy--; draw_row(oy); }
        while (vy1 > oy + BUF_ROWS - 1) { oy++; draw_row(oy + BUF_ROWS - 1); }
    }
    restore_slots();

    /* screen pixel VIEW_X shows world pixel map_x */
    sx = (map_x - VIEW_X) % 320;
    sy = (map_y - VIEW_Y) & 255;
    wait_frame();
    nextreg(NR_L2_XSCROLL_LSB, (uint8_t)sx);
    nextreg(NR_L2_XSCROLL_MSB, (uint8_t)(sx >> 8));
    nextreg(NR_L2_YSCROLL, (uint8_t)sy);
}
