#include <stdint.h>
#include <string.h>
#include "../platform/hw.h"
#include "../platform/files.h"
#include "assets.h"
#include "narr.h"
#include "state.h"
#include "actors.h"
#include "ui.h"

/* Bank 5 (pages 10/11) holds the tilemap; it is mapped at 0x4000-0x7FFF by default. */
#define TM_MAP          ((uint8_t *)0x4000)     /* 80 x 8 entries of 2 bytes */
#define TM_TILES_OFF    0x0600
#define TM_TILES        ((uint8_t *)(0x4000 + TM_TILES_OFF))
#define GLYPH_BASE      HISCR_TILES             /* tiles for characters 32..127 */

#define NR_TM_CTRL      0x6B
#define NR_TM_ATTR      0x6C
#define NR_TM_MAP       0x6E
#define NR_TM_TILES     0x6F
#define NR_TM_TRANSP    0x4C
#define NR_CLIP_TM      0x1B
#define NR_TM_XSCROLL_H 0x2F
#define NR_TM_XSCROLL_L 0x30
#define NR_TM_YSCROLL   0x31

#define PANEL_Y         168                     /* screen line of panel row 0 */

#define NARR_PAGE       74                      /* NARR.DAT, 2 pages */
#define SLOT1           ((uint8_t *)0x2000)

/* message area (Amiga: x 16..400, 4 lines) and status line */
#define MSG_COL         2
#define MSG_COLS        48
#define MSG_TOP         1
#define MSG_BOTTOM      4
#define STAT_ROW        6

#define COMBO_TEXT      1                       /* sub-palette 1: pen 10 on pen 11 */

const char *const datanames[3] = { "Julian", "Phillip", "Kevin" };

static uint8_t cursor;                          /* column on the bottom message line */
static uint8_t combos[16];                      /* sub-palette k: (bg << 4) | fg, 0xFF free */
static uint8_t cmp_entries[9 * CMP_W * CMP_H * 2];
static char mesbuf[200];
static char tmpbuf[256];

/* fmain.c:476 */
static const uint16_t textcolors[16] = {
    0x000, 0xFFF, 0xC00, 0xF60, 0x00f, 0xc0f, 0x090, 0xFF0,
    0xf90, 0xf0c, 0xA50, 0xFDB, 0xEB7, 0xCCC, 0x888, 0x444 };

/* ---- tilemap basics ---- */

static void ui_slots(void)
{
    mmu_set(2, 10);
    mmu_set(3, 11);
}

static void put_entry(uint8_t col, uint8_t row, uint16_t tile, uint8_t pal)
{
    uint8_t *e = TM_MAP + ((uint16_t)row * 80 + col) * 2;
    e[0] = (uint8_t)tile;
    e[1] = (pal << 4) | (uint8_t)(tile >> 8);
}

static void put_char(uint8_t col, uint8_t row, char c, uint8_t pal)
{
    if (c < 32 || c > 127) c = '?';
    put_entry(col, row, GLYPH_BASE + (uint8_t)(c - 32), pal);
}

/* 9-bit colour into palette entry `index` of the first tilemap palette */
static void set_pal(uint8_t index, uint16_t rgb4)
{
    uint8_t r = ((rgb4 >> 8) & 15) * 7 / 15;
    uint8_t g = ((rgb4 >> 4) & 15) * 7 / 15;
    uint8_t b = (rgb4 & 15) * 7 / 15;
    nextreg(NR_PAL_CTRL, 0x30);
    nextreg(NR_PAL_INDEX, index);
    nextreg(NR_PAL_VALUE9, (r << 5) | (g << 2) | (b >> 1));
    nextreg(NR_PAL_VALUE9, b & 1);
}

static uint8_t combo(uint8_t bg, uint8_t fg)
{
    uint8_t k, key = (bg << 4) | fg;
    for (k = 1; k < 16; k++)
        if (combos[k] == key)
            return k;
    for (k = 2; k < 16; k++)
        if (combos[k] == 0xFF) {
            combos[k] = key;
            set_pal(k * 16, textcolors[bg]);
            set_pal(k * 16 + 1, textcolors[fg]);
            return k;
        }
    return COMBO_TEXT;                          /* out of palettes: plain text colours */
}

void ui_menu_reset(void)
{
    uint8_t k;
    for (k = 2; k < 16; k++)
        combos[k] = 0xFF;
}

/* The Spectrum ROM font at 0x3D00 (chars 32..127) as 4bpp tiles: pixel 1 = ink. */
void ui_font_from_rom(void)
{
    const uint8_t *font = (const uint8_t *)0x3D00;
    uint8_t *t;
    uint8_t c, y, bits, x;

    ui_slots();
    for (c = 0; c < 96; c++) {
        t = TM_TILES + (uint16_t)(GLYPH_BASE + c) * 32;
        for (y = 0; y < 8; y++) {
            bits = font[c * 8 + y];
            for (x = 0; x < 4; x++) {
                *t++ = ((bits & 0x80) ? 0x10 : 0) | ((bits & 0x40) ? 0x01 : 0);
                bits <<= 2;
            }
        }
    }
}

void ui_init(void)
{
    uint8_t h, i;

    h = file_open("FTA/HISCR.TIL");
    if (h != FILE_ERROR) {
        file_read_at(h, 10, TM_TILES_OFF, (uint16_t)HISCR_TILES * 32);   /* fits in page 10 */
        file_close(h);
    }
    h = file_open("FTA/HISCR.MAP");
    if (h != FILE_ERROR) {
        file_read_at(h, 10, 0, 80 * 8 * 2);
        file_close(h);
    }
    file_load("FTA/HISCR.CMP", cmp_entries, sizeof(cmp_entries));
    h = file_open("FTA/NARR.DAT");
    if (h != FILE_ERROR) {
        file_read_pages(h, NARR_PAGE, NARR_SIZE);
        file_close(h);
    }
    ui_slots();

    for (i = 0; i < 16; i++)
        set_pal(i, textcolors[i]);
    combos[0] = 0;
    combos[COMBO_TEXT] = (11 << 4) | 10;
    set_pal(16, textcolors[11]);
    set_pal(17, textcolors[10]);
    ui_menu_reset();

    for (i = 1; i < 50; i++)                    /* status line: plain parchment */
        put_char(i, STAT_ROW, ' ', COMBO_TEXT);

    nextreg(NR_TM_MAP, 0x00);                   /* map at bank 5 offset 0 */
    nextreg(NR_TM_TILES, TM_TILES_OFF >> 8);
    nextreg(NR_TM_ATTR, 0x00);
    nextreg(NR_TM_TRANSP, HISCR_TRANSP);
    nextreg(NR_TM_XSCROLL_H, 0);
    nextreg(NR_TM_XSCROLL_L, 0);

    /* The tilemap only covers the border while the ULA border is transparent: switch the
       ULA on again, but make every ULA colour the global transparent colour 0xE3. */
    nextreg(NR_PAL_CTRL, 0x00);                 /* ULA first palette */
    nextreg(NR_PAL_INDEX, 0);
    i = 0;
    do { nextreg(NR_PAL_VALUE8, 0xE3); } while (++i);
    nextreg(NR_ULA_CTRL, 0x00);
    nextreg(NR_TM_YSCROLL, (uint8_t)(256 - PANEL_Y));
    nextreg(NR_CLIP_CTRL, 0x08);                /* reset tilemap clip index */
    nextreg(NR_CLIP_TM, 0);
    nextreg(NR_CLIP_TM, 159);
    nextreg(NR_CLIP_TM, PANEL_Y);
    nextreg(NR_CLIP_TM, PANEL_Y + 8 * 8 - 1);
    nextreg(NR_TM_CTRL, 0x80 | 0x40 | 0x02);    /* on, 80x32, attributes, 512 tiles */
}

/* ---- text output (fmain2.c:495) ---- */

void print_cont(const char *s)
{
    ui_slots();
    while (*s) {
        if (cursor < MSG_COL + MSG_COLS)
            put_char(cursor, MSG_BOTTOM, *s, COMBO_TEXT);
        cursor++;
        s++;
    }
}

void print(const char *s)
{
    uint8_t r, c;
    ui_slots();
    for (r = MSG_TOP; r < MSG_BOTTOM; r++)
        memcpy(TM_MAP + ((uint16_t)r * 80 + MSG_COL) * 2,
               TM_MAP + ((uint16_t)(r + 1) * 80 + MSG_COL) * 2, MSG_COLS * 2);
    for (c = 0; c < MSG_COLS; c++)
        put_char(MSG_COL + c, MSG_BOTTOM, ' ', COMBO_TEXT);
    cursor = MSG_COL;
    print_cont(s);
}

/* fsubs.asm _prdec: right aligned in len digits plus one space */
static void format_dec(char *buf, uint16_t n, uint8_t len)
{
    char d[6];
    uint8_t i = 0;
    do { d[i++] = '0' + n % 10; n /= 10; } while (n && i < 5);
    while (len > i) { *buf++ = ' '; len--; }
    while (i) *buf++ = d[--i];
    *buf++ = ' ';
    *buf = 0;
}

void prdec(uint16_t n, uint8_t len)
{
    char buf[12];
    format_dec(buf, n, len);
    print_cont(buf);
}

static void text_at(uint8_t col, uint8_t row, const char *s)
{
    ui_slots();
    while (*s && col < 80)
        put_char(col++, row, *s++, COMBO_TEXT);
}

/* fmain2.c:514 - word wrap at 37 columns, % = the brother's name */
void extract(const char *start)
{
    char *mbuf = mesbuf, *lbreak, *lstart = mesbuf, c;
    const char *ss;
    int16_t i = 0;

    for (;;) {
        lbreak = 0;
        for (; i < 37; i++) {
            c = *start++;
            if (c == ' ') lbreak = mbuf;
            if (c == 0) { lbreak = mbuf; *lbreak = 0; i = 1000; }
            if (c == 13) { lbreak = mbuf; *lbreak = 0; break; }
            if (c == '%') {
                ss = datanames[brother - 1];
                while (*ss) { *mbuf++ = *ss++; i++; }
            }
            else *mbuf++ = c;
        }
        if (lbreak) {
            *lbreak++ = 0;
            print(lstart);
            if (i > 38) break;
            i = mbuf - lbreak;
            lstart = lbreak;
        } else {
            *mbuf++ = 0;
            if (i > 38) break;
            print(lstart);
            i = 0;
            lstart = mbuf;
        }
    }
}

/* copy the n-th NUL-terminated string after `base` from NARR.DAT into tmpbuf */
static const char *narr_string(uint16_t base, uint8_t n)
{
    uint16_t off = base;
    uint8_t c, i = 0;
    for (;;) {
        mmu_set(1, NARR_PAGE + (off >> 13));
        c = SLOT1[off & 0x1FFF];
        off++;
        if (n == 0) {
            tmpbuf[i++] = c;
            if (!c || i == 255) break;
        }
        else if (!c) n--;
    }
    tmpbuf[255] = 0;
    mmu_set(1, 0xFF);
    return tmpbuf;
}

void event(uint8_t n) { extract(narr_string(NARR_event_msg, n)); }
void speak(uint8_t n) { extract(narr_string(NARR_speeches, n)); }
void msg_place(uint8_t inside, uint8_t n) { extract(narr_string(inside ? NARR_inside_msg : NARR_place_msg, n)); }
void question(uint8_t n)
{
    static const uint16_t qq[8] = NARR_QQ_TABLE;
    print_cont(narr_string(qq[n], 0));
}

void name(void) { print_cont(datanames[brother - 1]); }

void announce_container(const char *s)
{
    print(datanames[brother - 1]);
    print_cont(" found ");
    print_cont(s);
    print_cont(" containing ");
}

void announce_treasure(const char *s)
{
    print(datanames[brother - 1]);
    print_cont(" found ");
    print_cont(s);
}

/* place name table lookup (fmain.c find_place): returns the message number for a sector */
uint8_t place_lookup(uint8_t sector, uint8_t inside)
{
    uint16_t off = inside ? NARR_inside_tbl : NARR_place_tbl;
    uint8_t lo, hi, m = 0, i;
    for (i = 0; i < 255; i++, off += 3) {
        mmu_set(1, NARR_PAGE + (off >> 13));
        lo = SLOT1[off & 0x1FFF];
        hi = SLOT1[(off + 1) & 0x1FFF];
        m = SLOT1[(off + 2) & 0x1FFF];
        if (sector >= lo && sector <= hi) break;
    }
    mmu_set(1, 0xFF);
    return m;
}

/* ---- status line and compass ---- */

static void stat(uint8_t col, const char *label, int16_t v)
{
    char buf[12];
    text_at(col, STAT_ROW, label);
    format_dec(buf, v < 0 ? 0 : (uint16_t)v, 3);
    text_at(col + strlen(label), STAT_ROW, buf);
}

void print_options(void);

/* fmain2.c:442 ppick - done immediately instead of queued */
void prq(uint8_t n)
{
    switch (n) {
    case 4: stat(31, "Vit:", anim_list[0].vitality); break;
    case 5: print_options(); break;
    case 7:
        if (luck < 0) luck = 0;
        stat(2, "Brv:", brave);
        stat(11, "Lck:", luck);
        stat(21, "Knd:", kind);
        stat(40, "Wlth:", wealth);
        break;
    case 10: print("Take What?"); break;
    }
}

void drawcompass(uint8_t dir)
{
    const uint8_t *e = cmp_entries + (uint16_t)(dir < 8 ? dir : 8) * CMP_W * CMP_H * 2;
    uint8_t r;
    ui_slots();
    for (r = 0; r < CMP_H; r++) {
        memcpy(TM_MAP + ((uint16_t)(CMP_ROW + r) * 80 + CMP_COL) * 2, e, CMP_W * 2);
        e += CMP_W * 2;
    }
}

/* menu slot j: two columns at x 430 / 482, rows 9 px apart on the Amiga */
void ui_menu_text(uint8_t slot, const char *label, uint8_t len, uint8_t fg, uint8_t bg)
{
    uint8_t col = (slot & 1) ? 60 : 53, row = slot >> 1, pal, i;
    ui_slots();
    if (bg == 0 && fg == 0)
        pal = 0;
    else
        pal = combo(bg, fg);
    if (pal == 0) {                             /* empty slot: black */
        for (i = 0; i < 6; i++)
            put_char(col + i, row, ' ', 0);
        return;
    }
    put_char(col, row, ' ', pal);
    for (i = 0; i < 5; i++)
        put_char(col + 1 + i, row, i < len ? label[i] : ' ', pal);
}
