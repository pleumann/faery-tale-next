/* Banked module (PAGE_PLACARD): full-screen texts on the playfield in the Amber font -
   placards (fsubs.asm _placard, _ssp, narr.asm placard_text), map_message/message_off
   (fmain2.c), and the Layer 2 drawing primitives they need. While a placard is shown the
   Layer 2 scroll is reset, so buffer coordinates are playfield coordinates. */

#include <stdint.h>
#include <string.h>
#include "../platform/hw.h"
#include "../platform/layer2.h"
#include "../platform/files.h"
#include "narr.h"
#include "world.h"
#include "view.h"
#include "state.h"
#include "actors.h"
#include "banked.h"
#include "ui.h"
#include "placard.h"
#include "sprites.h"

#define FONT_PAGE   78
#define NARR_PAGE   74
#define SLOT1       ((uint8_t *)0x2000)
#define L2W         ((uint8_t *)0x6000)

static int16_t pen_x, pen_y;            /* Move() position: baseline */
static uint8_t pen = 24;
static uint8_t center;                  /* centre each text run on the 320 pixel screen */
static char line[64];

/* ---- Layer 2 primitives (playfield coordinates, scroll at 0) ---- */

void l2_pixel(int16_t x, int16_t y, uint8_t c)
{
    uint16_t bx, by;
    if (x < -VIEW_X || y < -VIEW_Y || x >= 320 - VIEW_X || y >= 256 - VIEW_Y) return;
    bx = (uint16_t)(x + VIEW_X) % 320;   /* screen x = buffer x when scroll is at -VIEW_X */
    by = (uint16_t)(y + VIEW_Y) & 255;
    mmu_set(3, L2_PAGE_A + (bx >> 5));
    L2W[((bx & 31) << 8) + by] = c;
}

static void l2_line(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint8_t c)
{
    int16_t dx = x1 > x0 ? x1 - x0 : x0 - x1, sx = x0 < x1 ? 1 : -1;
    int16_t dy = y1 > y0 ? y0 - y1 : y1 - y0, sy = y0 < y1 ? 1 : -1;
    int16_t err = dx + dy, e2;
    for (;;) {
        l2_pixel(x0, y0, c);
        if (x0 == x1 && y0 == y1) break;
        e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
    mmu_set(3, 11);
}

/* Layer 2 shows buffer (0,0) at the top-left of the playfield */
static void still_scroll(void)
{
    nextreg(NR_L2_XSCROLL_LSB, 0);
    nextreg(NR_L2_XSCROLL_MSB, 0);
    nextreg(NR_L2_YSCROLL, 0);
}

/* ---- Amber font (FONT.DAT, see tools/convert.py) ---- */

static uint8_t fb(uint16_t off)
{
    mmu_set(1, FONT_PAGE);
    return SLOT1[off];
}

void font_load_b(void)
{
    uint8_t h = file_open("FTA/FONT.DAT");
    if (h != FILE_ERROR) {
        file_read_pages(h, FONT_PAGE, 2048);
        file_close(h);
    }
}

static int16_t text_width(const char *s, uint8_t len)
{
    uint8_t lo = fb(2), hi = fb(3), ch, idx;
    int16_t w = 0;
    while (len--) {
        ch = (uint8_t)*s++;
        idx = (ch >= lo && ch <= hi) ? ch - lo : hi - lo + 1;
        w += (int8_t)fb(6 + (uint16_t)idx * 5 + 3);
    }
    return w;
}

/* Text() in JAM1 at the pen position, the pen moves on */
static void text(const char *s, uint8_t len)
{
    uint8_t ysize = fb(0), baseline = fb(1), lo = fb(2), hi = fb(3);
    uint16_t modulo = fb(4) | ((uint16_t)fb(5) << 8);
    uint16_t bitmap = 6 + (uint16_t)(hi - lo + 2) * 5;
    while (len--) {
        uint8_t ch = (uint8_t)*s++, idx, w, y, b;
        uint16_t e, off;
        int8_t space, kern;
        idx = (ch >= lo && ch <= hi) ? ch - lo : hi - lo + 1;
        e = 6 + (uint16_t)idx * 5;
        off = fb(e) | ((uint16_t)fb(e + 1) << 8);
        w = fb(e + 2);
        space = (int8_t)fb(e + 3);
        kern = (int8_t)fb(e + 4);
        for (y = 0; y < ysize; y++)
            for (b = 0; b < w; b++) {
                uint16_t bit = off + b;
                if (fb(bitmap + y * modulo + (bit >> 3)) & (0x80 >> (bit & 7)))
                    l2_pixel(pen_x + kern + b, pen_y - baseline + y, pen);
            }
        pen_x += space;
    }
    mmu_set(1, 0xFF);
    mmu_set(3, 11);
}

/* ---- placards ---- */

/* fsubs.asm _ssp: XY codes (128, x/2, y) and text runs */
static void ssp(uint16_t off)
{
    uint8_t c, n;
    for (;;) {
        mmu_set(1, NARR_PAGE + (off >> 13));
        c = SLOT1[off & 0x1FFF];
        if (c == 0) break;
        if (c == NARR_XY) {
            mmu_set(1, NARR_PAGE + ((off + 1) >> 13));
            pen_x = SLOT1[(off + 1) & 0x1FFF] * 2;
            mmu_set(1, NARR_PAGE + ((off + 2) >> 13));
            pen_y = SLOT1[(off + 2) & 0x1FFF];
            off += 3;
            continue;
        }
        n = 0;
        for (;;) {
            mmu_set(1, NARR_PAGE + (off >> 13));
            c = SLOT1[off & 0x1FFF];
            if (c == 0 || (c & 0x80) || n == sizeof(line)) break;
            line[n++] = c;
            off++;
        }
        mmu_set(1, 0xFF);
        text(line, n);
    }
    mmu_set(1, 0xFF);
}

void placard_text_b(uint8_t n)
{
    static const uint16_t mst[20] = NARR_MST_TABLE;
    ssp(mst[n]);
}

void placard_name_b(void)
{
    const char *s = datanames[brother - 1];
    text(s, (uint8_t)strlen(s));
}

void placard_ssp_b(const char *s)               /* resident XY string (titletext) */
{
    while (*s) {
        uint8_t n = 0;
        if ((uint8_t)*s == NARR_XY) { pen_x = (uint8_t)s[1] * 2; pen_y = (uint8_t)s[2]; s += 3; continue; }
        while (s[n] && !((uint8_t)s[n] & 0x80)) n++;
        if (center) pen_x = (320 - text_width(s, n)) / 2 - VIEW_X;
        text(s, n);
        s += n;
    }
}

void placard_pen_b(uint8_t c) { pen = c; }
void placard_center_b(uint8_t on) { center = on; }

/* fsubs.asm _placard: the meander border, finally in colour 24 */
static const int8_t xmod[16] = { -4,-4,-4,0,0,0,4,4,0,-4,0,4,4,0,0,0 };
static const int8_t ymod[16] = { 0,0,0,4,4,4,0,0,-4,0,-4,0,0,4,4,4 };

void placard_b(void)
{
    int16_t xorg = 12, yorg = 0, dx, dy;
    int8_t i, j;
    for (i = 16; i >= 0; i--) {
        for (j = 0; j < 16; j++) {
            dx = xorg + xmod[j];
            dy = yorg + ymod[j];
            if (i > 9) {
                l2_line(xorg, yorg, dx, dy, 24);
                l2_line(284 - xorg, 124 - yorg, 284 - dx, 124 - dy, 24);
            }
            l2_line(16 + yorg, 12 - xorg, 16 + dy, 12 - dx, 24);
            l2_line(268 - yorg, 112 + xorg, 268 - dy, 112 + dx, 24);
            xorg = dx;
            yorg = dy;
        }
    }
}

/* fmain2.c:600 - switch to a still playfield for messages */
void map_message_b(void)
{
    fade_down();
    sprites_hide();
    view_clear();
    still_scroll();
    ui_panel(0);
    fade_page(100, 100, 100, FALSE);
    pen = 24;
    viewstatus = 2;
}

/* fmain2.c:615 */
void message_off_b(void)
{
    fade_down();
    view_clear();
    ui_panel(1);
    viewstatus = 3;
}

/* RectFill(rp, 13,13,271,107) in colour 0 */
void placard_clear_b(void)
{
    int16_t x, y;
    for (x = 13; x <= 271; x++)
        for (y = 13; y <= 107; y++)
            l2_pixel(x, y, 0);
    mmu_set(3, 11);
}

/* fsubs.asm _bigdraw + fmain.c:3311 - the magic map: 18 x 9 sectors around the hero,
   one pixel per tile in the tile's map colour (terrain byte 3), every line doubled */
void bigdraw_b(void)
{
    int16_t secx, secy, px, py;
    uint8_t si, sj, tx, ty, sec;

    sprites_hide();
    view_clear();
    still_scroll();

    secx = (int16_t)(map_x >> 8) - 9 - (int16_t)xreg;
    secy = (int16_t)(map_y >> 8) - 4 - (int16_t)yreg;
    if (secx < 32) secx += 128;
    if (secx > 96) secx -= 128;
    if (secy < 32) secy += 128;
    if (secy > 96) secy -= 128;
    if (secx < 0) secx = 0;
    if (secx > 64 - 18) secx = 64 - 18;
    if (secy < 0) secy = 0;
    if (secy > 32 - 9) secy = 32 - 9;

    {
        static uint8_t colors[256];         /* map colour per tile (terrain byte 3) */
        static uint8_t secs[9 * 18];
        uint8_t dpage = 0xFF, spage = 0xFF;
        uint16_t i;
        mmu_set(1, MAP_PAGE);
        for (i = 0; i < 256; i++)
            colors[i] = SLOT1[MAP_TERRA_OFF + (i << 2) + 3] & 31;
        for (sj = 0; sj < 9; sj++)
            for (si = 0; si < 18; si++)
                secs[sj * 18 + si] = SLOT1[(uint16_t)(secy + sj) * 128 + xreg + secx + si];
        for (sj = 0; sj < 9; sj++) {
            for (si = 0; si < 18; si++) {
                uint16_t soff;
                sec = secs[sj * 18 + si];
                soff = (uint16_t)sec << 7;
                if (spage != SECTOR_PAGE + (soff >> 13)) {
                    spage = SECTOR_PAGE + (soff >> 13);
                    mmu_set(1, spage);
                }
                for (tx = 0; tx < 16; tx++) {
                    /* one Layer 2 column per tile column: playfield x -> buffer x */
                    uint16_t bx = VIEW_X + si * 16 + tx;
                    uint8_t *col;
                    if (dpage != L2_PAGE_A + (bx >> 5)) {
                        dpage = L2_PAGE_A + (bx >> 5);
                        mmu_set(3, dpage);
                    }
                    col = L2W + ((bx & 31) << 8) + VIEW_Y + sj * 16;
                    for (ty = 0; ty < 8; ty++) {
                        col[0] = col[1] = colors[SLOT1[(soff & 0x1FFF) + (ty << 4) + tx]];
                        col += 2;
                    }
                }
            }
        }
        mmu_set(1, 0xFF);
        mmu_set(3, 11);
    }

    /* the hero's position */
    pen_x = (int16_t)(hero_x >> 4) - ((secx + xreg) << 4) - 4;
    pen_y = (int16_t)(hero_y >> 4) - ((secy + yreg) << 4) + 3;
    if (pen_x > 0 && pen_x < 320 && pen_y > 0 && pen_y < 143) {
        uint8_t old = pen;
        pen = 31;
        text("+", 1);
        pen = old;
    }
}
