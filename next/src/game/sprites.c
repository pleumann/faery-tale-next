#include <stdint.h>
#include <string.h>
#include <arch/zxn.h>
#include "../platform/hw.h"
#include "../platform/files.h"
#include "assets.h"
#include "world.h"
#include "view.h"
#include "actors.h"
#include "sprites.h"

#define NR_SPRITE_CTRL      0x15
#define NR_CLIP_SPRITE      0x19
#define NR_SPRITE_TRANSP    0x4B

#define TRANSPARENT         SHAPE_TRANSPARENT   /* colour 31 */

#define SLOT2 ((uint8_t *)0x4000)

struct set_info {
    uint32_t offset;
    uint8_t frames, wcells, hcells, height;
};

static struct set_info sets[SHAPE_SETS];
static uint8_t set_page[7];             /* page holding each seq_list slot (PHIL, ...) */

static uint8_t pattern[512];            /* one 16x32 figure being composed */
static uint8_t shadow[64];              /* one shadow mask */
static uint8_t next_sprite;             /* hardware sprite slot allocation per frame */
static uint8_t last_sprites;
static uint8_t pat_base;                /* 0 or 32: patterns are double buffered */

struct attr { uint8_t x, y, a2, a3; };
static struct attr attrs[64];

void sprites_init(void)
{
    uint8_t h;

    /* all 128 sprites invisible - attribute RAM is not cleared on start */
    IO_SPRITE_SLOT = 0;
    for (h = 0; h < 128; h++) {
        IO_SPRITE_ATTRIBUTE = 0;
        IO_SPRITE_ATTRIBUTE = 0;
        IO_SPRITE_ATTRIBUTE = 0;
        IO_SPRITE_ATTRIBUTE = 0;
    }
    nextreg(NR_SPRITE_TRANSP, TRANSPARENT);
    nextreg(NR_SPRITE_CTRL, 0x23);      /* visible, over border, clip in over-border mode, SLU */
    nextreg(NR_CLIP_CTRL, 0x02);        /* reset sprite clip index */
    nextreg(NR_CLIP_SPRITE, VIEW_X / 2);
    nextreg(NR_CLIP_SPRITE, (VIEW_X + VIEW_W - 1) / 2);
    nextreg(NR_CLIP_SPRITE, (uint8_t)VIEW_Y);
    nextreg(NR_CLIP_SPRITE, (uint8_t)(VIEW_Y + VIEW_H - 1));

    h = file_open("FTA/SHAPES.DAT");
    if (h != FILE_ERROR) {
        file_seek(h, 2);
        file_read_at(h, SHAPE_PAGE_PHIL, 0, sizeof(sets));
        mmu_set(2, SHAPE_PAGE_PHIL);
        memcpy(sets, SLOT2, sizeof(sets));
        mmu_set(2, 10);
        file_close(h);
    }
    h = file_open("FTA/SHADOW.DAT");
    if (h != FILE_ERROR) {
        file_read_pages(h, SHADOW_PAGE, 192 * 64);
        file_close(h);
    }
}

uint8_t sprites_load_set(uint8_t set, uint8_t page)
{
    uint8_t h = file_open("FTA/SHAPES.DAT");
    uint32_t len = (uint32_t)sets[set].frames * sets[set].wcells * sets[set].hcells * 256;
    if (h == FILE_ERROR)
        return 0;
    file_seek(h, sets[set].offset);
    file_read_pages(h, page, len);
    file_close(h);
    set_page[set < 3 ? PHIL : 0] = page;    /* M3: only the brothers */
    return 1;
}

void sprites_palette(const uint16_t *rgb4, uint8_t pct)
{
    /* same conversion as palette_load_rgb4, but into the first sprite palette */
    nextreg(NR_PAL_CTRL, 0x20);
    nextreg(NR_PAL_INDEX, 0);
    for (uint8_t i = 0; i < 32; i++) {
        uint16_t c = rgb4[i];
        uint8_t r = (uint8_t)((((c >> 8) & 15) * pct * 7 + 750) / 1500);
        uint8_t g = (uint8_t)((((c >> 4) & 15) * pct * 7 + 750) / 1500);
        uint8_t b = (uint8_t)(((c & 15) * pct * 7 + 750) / 1500);
        nextreg(NR_PAL_VALUE9, (r << 5) | (g << 2) | (b >> 1));
        nextreg(NR_PAL_VALUE9, b & 1);
    }
}

/* Copy frame `frame` (16x32 = two 16x16 cells) of a set into pattern[]. */
static void fetch_frame(uint8_t page, uint8_t frame)
{
    uint16_t off = (uint16_t)frame * 512;
    mmu_set(2, page + (off >> 13));
    memcpy(pattern, SLOT2 + (off & 0x1FFF), 512);   /* 512 never crosses a page */
    mmu_set(2, 10);
}

static void fetch_shadow(uint8_t n)
{
    uint16_t off = (uint16_t)n * 64;
    mmu_set(2, SHADOW_PAGE + (off >> 13));
    memcpy(shadow, SLOT2 + (off & 0x1FFF), 64);
    mmu_set(2, 10);
}

/* Hide the figure's pixels where shadow mask n covers tile (tx, ty).
   (wx, wy) is the world position of pattern pixel (0, 0). */
static void apply_shadow(uint8_t n, uint16_t tx, uint16_t ty, uint16_t wx, uint16_t wy)
{
    int16_t x0 = (int16_t)(tx * 16 - wx);       /* tile left edge in pattern coordinates */
    int16_t y0 = (int16_t)(ty * 32 - wy);       /* tile top edge in pattern coordinates */
    int16_t px, py;
    uint8_t row;

    fetch_shadow(n);
    for (row = 0; row < 32; row++) {
        uint16_t bits;
        py = y0 + row;
        if (py < 0 || py > 31)
            continue;
        bits = ((uint16_t)shadow[row * 2] << 8) | shadow[row * 2 + 1];
        if (!bits)
            continue;
        for (px = 0; px < 16; px++) {
            int16_t x = x0 + px;
            if (x >= 0 && x < 16 && (bits & (0x8000 >> px)))
                pattern[(py >> 4) * 256 + (py & 15) * 16 + x] = TRANSPARENT;
        }
    }
}

/* Occlusion rules of fmain.c:2571-2598 for a 16x32 figure at world (wx, wy). */
static void occlude(struct shape *an, uint16_t wx, uint16_t wy, uint16_t ground)
{
    uint16_t tx0 = wx >> 4, ty0 = (wy & 0x7FFF) >> 5;
    uint8_t cols = (wx & 15) ? 2 : 1;
    uint8_t rows = (uint8_t)((((wy & 0x7FFF) + 31) >> 5) - ty0 + 1);
    uint8_t xm, ym, tile, k, mask;
    int16_t ystop;

    for (xm = 0; xm < cols; xm++) {
        for (ym = 0; ym < rows; ym++) {
            uint16_t ty = ty0 + ym;
            ystop = (int16_t)(ground - (ty << 5));
            tile = world_tile(tx0 + xm, ty | (wy & 0x8000 ? 0x400 : 0));
            k = terra_byte(tile, 1) & 15;
            mask = terra_byte(tile, 0);
            switch (k) {
            case 0: continue;
            case 1: if (xm == 0) continue; break;
            case 2: if (ystop > 35) continue; break;
            case 3: if (hero_sector == 48 && an != &anim_list[1]) continue; break;
            case 4: if (xm == 0 || ystop > 35) continue; break;
            case 5: if (xm == 0 && ystop > 35) continue; break;
            case 6: if (ym != 0) mask = terra_byte(64, 0); break;
            case 7: if (ystop > 20) continue; break;
            }
            apply_shadow(mask, tx0 + xm, ty, wx, wy & 0x7FFF);
        }
    }
}

static void upload_pattern(uint8_t slot, const uint8_t *src)
{
    uint16_t n;
    IO_SPRITE_SLOT = slot;
    for (n = 0; n < 256; n++)
        IO_SPRITE_PATTERN = src[n];
}

static void set_sprite(uint8_t slot, uint16_t x, uint8_t y, uint8_t pat, uint8_t visible)
{
    struct attr *a = &attrs[slot];
    a->x = (uint8_t)x;
    a->y = y;
    a->a2 = (uint8_t)(x >> 8) & 1;
    a->a3 = (visible ? 0x80 : 0) | (pat & 0x3F);
}

/* Draw one 16x32 actor (the brothers and humanoid enemies). */
static void draw_figure(struct shape *an)
{
    int16_t sx = (int16_t)(an->abs_x - map_x - 8);
    int16_t sy = (int16_t)(an->abs_y - map_y - 26);
    uint16_t wx = an->abs_x - 8;
    uint16_t wy = an->abs_y - 26;
    uint16_t ground = (an->abs_y & 0x7FFF) - 26 + 32;   /* feet, world y */
    int8_t env = an->environ;
    uint8_t frame, cut_top = 32;            /* rows of the image to show */
    uint8_t slot = next_sprite;

    if (sx < -16 || sx > VIEW_W || sy < -VIEW_Y || sy > VIEW_H)
        return;
    if (env > 29)
        return;                             /* TODO (M4): bubbles (objects 97/98) */

    frame = statelist[an->index].figure;
    fetch_frame(set_page[PHIL], frame);

    if (env == 2)
        cut_top = 22;                       /* wading: ystop -= 10 */
    else if (env > 2) {
        sy += env;                          /* sinking: image moves down, bottom stays */
        wy += env;
        cut_top = 32 - env;
    }
    if (cut_top < 32)
        memset(pattern + (cut_top >> 4) * 256 + (cut_top & 15) * 16, TRANSPARENT,
               512 - ((cut_top >> 4) * 256 + (cut_top & 15) * 16));

    occlude(an, wx, wy, ground);

    upload_pattern(pat_base + slot, pattern);
    upload_pattern(pat_base + slot + 1, pattern + 256);
    set_sprite(slot, VIEW_X + sx, (uint8_t)(VIEW_Y + sy), pat_base + slot, 1);
    set_sprite(slot + 1, VIEW_X + sx, (uint8_t)(VIEW_Y + sy + 16), pat_base + slot + 1, 1);
    next_sprite += 2;
}

void sprites_draw(void)
{
    uint8_t i;
    pat_base ^= 32;
    next_sprite = 0;
    for (i = 0; i < anix; i++)
        if (anim_list[i].type == PHIL)
            draw_figure(&anim_list[i]);
    for (i = next_sprite; i < last_sprites; i++)
        set_sprite(i, 0, 0, 0, 0);
}

/* Write the attributes prepared by sprites_draw() - call right after the vblank. */
void sprites_flush(void)
{
    uint8_t i, n = next_sprite > last_sprites ? next_sprite : last_sprites;
    IO_SPRITE_SLOT = 0;
    for (i = 0; i < n; i++) {
        IO_SPRITE_ATTRIBUTE = attrs[i].x;
        IO_SPRITE_ATTRIBUTE = attrs[i].y;
        IO_SPRITE_ATTRIBUTE = attrs[i].a2;
        IO_SPRITE_ATTRIBUTE = attrs[i].a3;
    }
    last_sprites = next_sprite;
}
