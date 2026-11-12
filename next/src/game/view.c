#include <stdint.h>
#include <string.h>
#include "../platform/hw.h"
#include "../platform/layer2.h"
#include "world.h"
#include "view.h"
#include "shapes.h"
#include "actors.h"

#define BUF_COLS    20          /* 320 / 16 */
#define BUF_ROWS    8           /* 256 / 32 */

#define SRC ((uint8_t *)0x4000) /* slot 2: tile page */
#define DST ((uint8_t *)0x6000) /* slot 3: Layer 2 page */

static uint16_t ox, oy;         /* tile coordinates of the buffered 20 x 8 window */
static uint8_t valid;
static uint16_t scroll_x, scroll_y;

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
    scroll_x = (map_x - VIEW_X) % 320;
    scroll_y = (map_y - VIEW_Y) & 255;
}

void view_scroll(void)
{
    nextreg(NR_L2_XSCROLL_LSB, (uint8_t)scroll_x);
    nextreg(NR_L2_XSCROLL_MSB, (uint8_t)(scroll_x >> 8));
    nextreg(NR_L2_YSCROLL, (uint8_t)scroll_y);
}

/* Clear the whole Layer 2 buffer (the playfield then needs a full redraw). */
void view_clear(void)
{
    uint8_t p;
    for (p = 0; p < 10; p++) {
        mmu_set(3, L2_PAGE_A + p);
        memset(DST, 0, 8192);
    }
    restore_slots();
    valid = 0;
}

/* Draw rows [off, off+h) of object frame `frame` at playfield position (px, py),
   colour 31 left out (fmain.c Items/List). */
void view_blit_object(uint8_t frame, uint8_t off, uint8_t h, uint16_t px, uint8_t py)
{
    const struct seq_info *q = &seq_list[OBJECTS];
    uint16_t src = (uint16_t)frame * 256;
    uint8_t row, col, v;
    uint16_t bx, by;
    for (row = 0; row < h; row++) {
        for (col = 0; col < 16; col++) {
            mmu_set(2, q->page + (src >> 13));
            v = SRC[(src & 0x1FFF) + (uint16_t)(off + row) * 16 + col];
            if (v == 31) continue;
            bx = (map_x + px + col) % 320;
            by = (map_y + py + row) & 255;
            mmu_set(3, L2_PAGE_A + (bx >> 5));
            DST[((bx & 31) << 8) + by] = v;
        }
    }
    restore_slots();
}
