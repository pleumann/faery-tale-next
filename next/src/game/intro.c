/* Banked module (PAGE_INTRO): title and intro book (fmain.c main() 1155-1215,
   fmain2.c copypage), the princess rescue (rescue) and the victory (win_colors).
   The animated page turn (flipscan) is replaced by a fade. */

#include <stdint.h>
#include <string.h>
#include "../platform/hw.h"
#include "../platform/layer2.h"
#include "../platform/files.h"
#include "../platform/input.h"
#include "world.h"
#include "view.h"
#include "state.h"
#include "actors.h"
#include "banked.h"
#include "ui.h"
#include "audio.h"
#include "placard.h"
#include "sprites.h"
#include "intro.h"

#define L2W         ((uint8_t *)0x6000)
#define SRC         ((uint8_t *)0x4000)
#define BRUSH_PAGE  56                  /* sector/map copies: loaded only after the intro */
#define PIC_Y       28                  /* 320x200 pictures are centred in 256 lines */

/* fmain.c:484 */
static const uint16_t introcolors[32] = {
    0x000, 0xFFF, 0xE00, 0xA00, 0xD80, 0xEC0, 0x390, 0x021,
    0xEEB, 0xEDA, 0xEEA, 0xCB8, 0xA95, 0x973, 0x840, 0x620,
    0xA52, 0xC74, 0xD96, 0xFCA, 0x449, 0x444, 0xDC9, 0x668,
    0x33F, 0x888, 0xA60, 0xAAF, 0xBBB, 0xCCF, 0xDDD, 0xEEE };

/* fmain2.c:1569 */
static const uint16_t sun_colors[53] = {
    0x000, 0x000, 0x001, 0x002, 0x002, 0x012, 0x112, 0x113,
    0x113, 0x214, 0x224, 0x225, 0x326, 0x326, 0x337, 0x338,
    0x438, 0x439, 0x449, 0x54A, 0x54B, 0x54B, 0x55C, 0x65C,
    0x65D, 0x66E, 0x76E, 0x76F, 0x86F, 0x97E, 0xA7E, 0xB7E,
    0xB8D, 0xC8D, 0xD8D, 0xD8D, 0xE9C, 0xE9C, 0xE9B, 0xE9B,
    0xFAA, 0xFAA, 0xF99, 0xF98, 0xF98, 0xF97, 0xF86, 0xF85,
    0xF84, 0xF84, 0xF93, 0xF92, 0x76F };

/* fsubs.asm _titletext (XY = 128, x/2, y) */
static const char titletext[] = {
    (char)128, (160 - 26 * 4) / 2, 33, '"', 'T','h','e',' ','F','a','e','r','y',' ','T','a','l','e',' ',
        'A','d','v','e','n','t','u','r','e', '"',
    (char)128, (160 - 30 * 4) / 2, 79, 'A','n','i','m','a','t','i','o','n',',',' ','P','r','o','g','r','a',
        'm','m','i','n','g',' ','&',' ','M','u','s','i','c',
    (char)128, (160 - 2 * 4) / 2, 90, 'b','y',
    (char)128, (160 - 12 * 4) / 2, 101, 'D','a','v','i','d',' ','J','o','i','n','e','r',
    (char)128, (160 - 30 * 4) / 2, 160, 'C','o','p','y','r','i','g','h','t',' ','1','9','8','6',' ',
        'M','i','c','r','o','I','l','l','u','s','i','o','n','s',
    0 };

static uint8_t skipp;

/* wait, but stop early on SPACE (skipint) */
static uint8_t wait_skip(uint16_t ticks)
{
    uint16_t end = frames + ticks;
    while ((int16_t)(frames - end) < 0) {
        audio_update();
        if (input_key() == ' ') return skipp = 1;
    }
    return 0;
}

static void fade(const uint16_t *pal, uint8_t up)
{
    int8_t p;
    for (p = up ? 0 : 100; up ? p <= 100 : p >= 0; p += up ? 5 : -5) {
        palette_load_rgb4(0, pal, 32, (uint8_t)p);
        delay(1);
    }
}

/* full-screen picture: 320 wide, centred */
static void fullscreen(void)
{
    layer2_clip(0, PIC_Y, 319, PIC_Y + 199);
    nextreg(NR_L2_XSCROLL_LSB, 0);
    nextreg(NR_L2_XSCROLL_MSB, 0);
    nextreg(NR_L2_YSCROLL, 0);
}

/* unpackbrush(name, page, x bytes, y) onto the Layer 2 picture. The brush is row-major,
   Layer 2 column-major: map the source page per row, the target page per 32 columns. */
static uint8_t l2_base = L2_PAGE_A;     /* buffer the brushes are drawn into */

static void brush(const char *name, uint16_t x, uint8_t y)
{
    uint16_t w, h, r, c, off = 4;
    uint8_t spage = 0xFF, dpage, by;
    const uint8_t *src;
    if (!file_load_pages(name, BRUSH_PAGE)) return;
    mmu_set(2, BRUSH_PAGE);
    w = SRC[0] | (SRC[1] << 8);
    h = SRC[2] | (SRC[3] << 8);
    for (r = 0; r < h; r++) {
        by = PIC_Y + y + r;
        dpage = 0xFF;
        for (c = 0; c < w; c++, off++) {
            uint16_t bx = x + c;
            if ((uint8_t)(off >> 13) != spage) {
                spage = (uint8_t)(off >> 13);
                mmu_set(2, BRUSH_PAGE + spage);
            }
            if ((uint8_t)(bx >> 5) != dpage) {
                dpage = (uint8_t)(bx >> 5);
                mmu_set(3, l2_base + dpage);
            }
            src = SRC + (off & 0x1FFF);
            L2W[((bx & 31) << 8) + by] = *src;
        }
    }
    mmu_set(2, 10);
    mmu_set(3, 11);
}

/* Book pages: copy the visible picture into the hidden Layer 2 buffer, draw the new
   brushes there, then show it at once (the Amiga used its two pages the same way). */
static uint8_t shown_bank = L2_BANK_A;

static void book_page(const char *a, const char *b, uint16_t bx, uint8_t by)
{
    uint8_t from = shown_bank * 2, to, i;
    uint8_t back = (shown_bank == L2_BANK_A) ? L2_BANK_B : L2_BANK_A;
    to = back * 2;
    for (i = 0; i < 10; i++) {
        mmu_set(2, from + i);
        mmu_set(3, to + i);
        memcpy(L2W, SRC, 8192);
    }
    mmu_set(2, 10);
    mmu_set(3, 11);
    l2_base = to;
    brush(a, 32, 24);
    brush(b, bx, by);
    l2_base = L2_PAGE_A;
    shown_bank = back;
    layer2_show(back);
}

/* fmain.c:1155-1215 */
void intro_b(void)
{
    static uint16_t black[32];
    skipp = 0;
    world_init();                       /* the brushes use cached tile pages */
    region_num = NO_REGION;
    ui_panel(0);
    sprites_hide();
    view_clear();
    fullscreen();
    palette_load_rgb4(0, introcolors, 32, 100);
    placard_pen(1);
    placard_center(1);                  /* Amber is wider than the Amiga's hires topaz */
    placard_ssp(titletext);
    placard_center(0);
    music_play(12);
    if (wait_skip(150)) goto end;

    fade(introcolors, 0);
    file_load_pages("FTA/PAGE0.L2", L2_PAGE_A);
    fade(introcolors, 1);
    if (wait_skip(100)) goto end;
    if (wait_skip(350)) goto end;
    book_page("FTA/P1A.BIN", "FTA/P1B.BIN", 21 * 8, 29);
    if (wait_skip(350)) goto end;
    book_page("FTA/P2A.BIN", "FTA/P2B.BIN", 20 * 8, 29);
    if (wait_skip(350)) goto end;
    book_page("FTA/P3A.BIN", "FTA/P3B.BIN", 20 * 8, 33);
    wait_skip(190);
end:
    fade(introcolors, 0);
    layer2_show(L2_BANK_A);             /* the game draws into buffer A */
    shown_bank = L2_BANK_A;
    palette_load_rgb4(0, black, 32, 0);
    seed1 ^= frames;                    /* the original had no seed at all */
    view_clear();
    view_init();
}

/* fmain2.c:1584 - the princess is rescued */
void rescue_b(void)
{
    uint8_t i = princess * 3;
    map_message();
    placard_text(8 + i); placard_name(); placard_text(9 + i); placard_name(); placard_text(10 + i);
    placard();
    delay(380);
    placard_clear();
    delay(10);
    placard_text(17); placard_name(); placard_text(18);
    delay(380);
    message_off();

    princess++;
    xfer(5511, 33780, 0);               /* do princess transfer */
    move_extent(0, 22205, 21231);
    ob_id_set(8, 2, 4);
    stuff[28] = 1;
    speak(18);
    wealth += 100;
    ob_stat_set(8, 9, 0);
    for (i = 16; i < 22; i++) stuff[i] += 3;
}

/* fmain2.c:1605 - the talisman is found */
void win_b(void)
{
    static uint16_t fader[32];
    int8_t i, j;
    map_message();
    placard_text(6); placard_name(); placard_text(7);
    placard();
    delay(80);
    fade(introcolors, 0);
    fullscreen();
    file_load_pages("FTA/WINPIC.L2", L2_PAGE_A);
    for (i = 25; i > -30; i--) {
        fader[0] = fader[31] = 0;
        fader[1] = fader[28] = 0xfff;
        for (j = 2; j < 28; j++)
            fader[j] = (i + j > 0) ? sun_colors[i + j] : 0;
        if (i > -14) { fader[29] = 0x800; fader[30] = 0x400; }
        else {
            j = (i + 30) / 2;
            fader[29] = 0x100 * j;
            fader[30] = 0x100 * (j / 2);
        }
        palette_load_rgb4(0, fader, 32, 100);
        if (i == 25) delay(60);
        delay(9);
    }
    delay(30);
    while (!input_key()) audio_update();
    palette_load_rgb4(0, fader, 32, 0);
    view_init();
}
