/* Actors as Next hardware sprites. Replaces save_blit/mask_blit/shape_blit/rest_blit
   from fsubs.asm and the drawing loop in fmain.c (lines 2381-2609).

   Every shape is built in a buffer of 16x16 cells, the background occlusion (shadow
   masks) is applied by turning covered pixels transparent, and each cell becomes one
   hardware sprite. Patterns are double buffered (0-31 / 32-63). */

#include <stdint.h>
#include <string.h>
#include <arch/zxn.h>
#include "../platform/hw.h"
#include "../platform/files.h"
#include "assets.h"
#include "world.h"
#include "view.h"
#include "state.h"
#include "actors.h"
#include "shapes.h"
#include "sprites.h"

#define NR_SPRITE_CTRL      0x15
#define NR_CLIP_SPRITE      0x19
#define NR_SPRITE_TRANSP    0x4B

#define TRANSPARENT         SHAPE_TRANSPARENT   /* colour 31 */
#define MAX_PATTERNS        32                  /* per half of the pattern memory */

#define SLOT2 ((uint8_t *)0x4000)

static uint8_t buf[16 * 256];           /* up to 4x4 cells (the bird) */
static uint8_t shadow[64];
static uint8_t bw, bh;                  /* buffer size in cells */
static uint8_t next_sprite, last_sprites, npat;
static uint8_t pat_base;

struct attr { uint8_t x, y, a2, a3; };
static struct attr attrs[64];

void sprites_init(void)
{
    uint8_t h;

    IO_SPRITE_SLOT = 0;                 /* all 128 sprites invisible */
    for (h = 0; h < 128; h++) {
        IO_SPRITE_ATTRIBUTE = 0;
        IO_SPRITE_ATTRIBUTE = 0;
        IO_SPRITE_ATTRIBUTE = 0;
        IO_SPRITE_ATTRIBUTE = 0;
    }
    nextreg(NR_SPRITE_TRANSP, TRANSPARENT);
    nextreg(NR_SPRITE_CTRL, 0x23);      /* visible, over border, clip in over-border mode, SLU */
    nextreg(NR_CLIP_CTRL, 0x02);
    nextreg(NR_CLIP_SPRITE, VIEW_X / 2);
    nextreg(NR_CLIP_SPRITE, (VIEW_X + VIEW_W - 1) / 2);
    nextreg(NR_CLIP_SPRITE, (uint8_t)VIEW_Y);
    nextreg(NR_CLIP_SPRITE, (uint8_t)(VIEW_Y + VIEW_H - 1));

    h = file_open("FTA/SHADOW.DAT");
    if (h != FILE_ERROR) {
        file_read_pages(h, SHADOW_PAGE, 192 * 64);
        file_close(h);
    }
}

void sprites_palette(const uint16_t *rgb4, uint8_t pct)
{
    nextreg(NR_PAL_CTRL, 0x20);         /* first sprite palette */
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

/* pixel address in buf */
#define PIX(x, y) (buf + ((uint16_t)(((y) >> 4) * bw + ((x) >> 4)) << 8) + (((y) & 15) << 4) + ((x) & 15))

/* Load a frame of a shape set into buf; `skip` source rows are dropped from the top
   (only used for single-cell objects). */
static void fetch_frame(const struct seq_info *q, uint8_t frame, uint8_t skip)
{
    uint32_t off = (uint32_t)frame * q->frame_bytes;
    uint16_t n = q->frame_bytes, done = 0, chunk;

    bw = q->wcells;
    bh = q->hcells;
    while (done < n) {
        uint16_t o = (uint16_t)(off & 0x1FFF);
        mmu_set(2, q->page + (uint8_t)(off >> 13));
        chunk = 8192 - o;
        if (chunk > n - done) chunk = n - done;
        memcpy(buf + done, SLOT2 + o, chunk);
        done += chunk;
        off += chunk;
    }
    mmu_set(2, 10);
    if (skip) {
        memmove(buf, buf + skip * 16, 256 - skip * 16);
        memset(buf + 256 - skip * 16, TRANSPARENT, skip * 16);
    }
}

/* Clear rows from `from` to the bottom of the buffer. */
static void cut_rows(uint8_t from)
{
    uint8_t y, x, h = bh * 16, w = bw * 16;
    for (y = from; y < h; y++)
        for (x = 0; x < w; x += 16)
            memset(PIX(x, y), TRANSPARENT, 16);
}

static void fetch_shadow(uint8_t n)
{
    uint16_t off = (uint16_t)n * 64;
    mmu_set(2, SHADOW_PAGE + (off >> 13));
    memcpy(shadow, SLOT2 + (off & 0x1FFF), 64);
    mmu_set(2, 10);
}

/* Make the buffer transparent where shadow mask n covers tile (tx, ty);
   (wx, wy) is the world position of buffer pixel (0, 0). */
static void apply_shadow(uint8_t n, uint16_t tx, uint16_t ty, uint16_t wx, uint16_t wy)
{
    int16_t x0 = (int16_t)((tx << 4) - wx);
    int16_t y0 = (int16_t)((ty << 5) - wy);
    int16_t w = bw * 16, h = bh * 16;
    uint8_t row, px;

    fetch_shadow(n);
    for (row = 0; row < 32; row++) {
        int16_t y = y0 + row;
        uint16_t bits;
        if (y < 0 || y >= h)
            continue;
        bits = ((uint16_t)shadow[row * 2] << 8) | shadow[row * 2 + 1];
        if (!bits)
            continue;
        for (px = 0; px < 16; px++) {
            int16_t x = x0 + px;
            if (x >= 0 && x < w && (bits & (0x8000 >> px)))
                *PIX(x, y) = TRANSPARENT;
        }
    }
}

/* Occlusion rules of fmain.c:2571-2598. (wx, wy) world position of the buffer,
   rows = visible height, ground = world y of the figure's feet. */
static void occlude(struct shape *an, uint8_t i, uint16_t wx, uint16_t wy, uint8_t rows, uint16_t ground)
{
    uint16_t tx0 = wx >> 4, ty0 = wy >> 5;
    uint8_t cols = (uint8_t)(((wx + bw * 16 - 1) >> 4) - tx0 + 1);
    uint8_t trows = (uint8_t)(((wy + rows - 1) >> 5) - ty0 + 1);
    uint8_t xm, ym, tile, k, mask;
    int16_t ystop;

    for (xm = 0; xm < cols; xm++) {
        for (ym = 0; ym < trows; ym++) {
            uint16_t ty = ty0 + ym;
            ystop = (int16_t)(ground - (ty << 5));
            tile = world_tile(tx0 + xm, ty);
            k = terra_byte(tile, 1) & 15;
            mask = terra_byte(tile, 0);
            if (an->state == FALL) {
                if (tile <= 220) continue;
                k = 3;
            }
            switch (k) {
            case 0: continue;
            case 1: if (xm == 0) continue; break;
            case 2: if (ystop > 35) continue; break;
            case 3: if (hero_sector == 48 && i != 1) continue; break;     /* bridge */
            case 4: if (xm == 0 || ystop > 35) continue; break;
            case 5: if (xm == 0 && ystop > 35) continue; break;
            case 6: if (ym != 0) mask = terra_byte(64, 0); break;        /* full if above */
            case 7: if (ystop > 20) continue; break;
            }
            apply_shadow(mask, tx0 + xm, ty, wx, wy);
        }
    }
}

static void set_sprite(uint8_t slot, uint16_t x, uint8_t y, uint8_t pat, uint8_t visible)
{
    struct attr *a = &attrs[slot];
    a->x = (uint8_t)x;
    a->y = y;
    a->a2 = (uint8_t)(x >> 8) & 1;
    a->a3 = (visible ? 0x80 : 0) | (pat & 0x3F);
}

/* Upload the buffer's cells and place them at playfield position (rx, ry). */
static void emit(int16_t rx, int16_t ry, uint8_t rows)
{
    uint8_t cx, cy, n;
    for (cy = 0; cy < bh; cy++) {
        int16_t sy = VIEW_Y + ry + cy * 16;
        if (cy * 16 >= rows)
            break;
        if (sy < 0 || sy > 255)
            continue;
        for (cx = 0; cx < bw; cx++) {
            int16_t sx = VIEW_X + rx + cx * 16;
            const uint8_t *src;
            if (sx < 0 || sx > 319)
                continue;
            if (npat >= MAX_PATTERNS || next_sprite >= 64)
                return;
            n = pat_base + npat++;
            src = buf + ((uint16_t)(cy * bw + cx) << 8);
            IO_SPRITE_SLOT = n;
            for (uint16_t b = 0; b < 256; b++)
                IO_SPRITE_PATTERN = src[b];
            set_sprite(next_sprite++, (uint16_t)sx, (uint8_t)sy, n, 1);
        }
    }
}

/* fmain.c:2381-2609 for one actor (one or two passes: body and weapon). */
static void draw_actor(uint8_t i)
{
    struct shape *an = &anim_list[i];
    uint8_t pass = 0, passmode, atype, nomask;
    int16_t inum;
    int16_t xstart, ystart, ground, ystop, ysize;
    uint8_t skip;
    const struct seq_info *q;

    an->visible = 0;
newpass:
    passmode = 0;
    inum = an->index;
    if (an->weapon > 0 && an->weapon < 8 &&
        (an->state < DEAD || an->state >= SHOOT1 || xtype > 80)) {
        if ((an->facing - 2) & 4) passmode = pass ^ 1;
        else passmode = pass;
        if (an->weapon == 4 && an->state < SHOOT1) {
            if ((an->facing & 4) == 0) passmode = pass ^ 1;
            else passmode = pass;
        }
        pass++;
    }

    xstart = an->rel_x;
    ystart = an->rel_y;
    ground = ystart + 32;
    skip = 0;

    if (passmode) {
        int8_t k = an->weapon;
        if (k == 4 && inum < 32) { xstart += bow_x[inum]; ystart += bow_y[inum]; }
        else { xstart += statelist[inum].wpn_x; ystart += statelist[inum].wpn_y; }
        if (k == 4 && inum < 32) {                      /* bow */
            inum = inum / 8;
            if (inum & 1) inum = 30;
            else if (inum & 2) inum = 0x53;
            else inum = 0x51;
        }
        else if (k == 5) {
            inum = an->facing + 103;
            if (an->facing == 2) ystart -= 6;
        }
        else {                                          /* hand weapons */
            if (k == 2) k = 32;
            else if (k == 3) k = 48;
            else if (k == 1) k = 64;
            else if (k == 4) k = 0;
            inum = statelist[inum].wpn_no + k;
        }
        atype = OBJECTS;
    } else {
        atype = an->type;
        if ((atype == ENEMY && an->race != 8) || atype == PHIL) {
            if (fiery_death && an->environ > 0) {
                if (an->state == DEAD) goto offscreen;
                else if (an->state == DYING) { atype = OBJECTS; inum = 0x58; goto rte; }
            }
            if (an->state == FALL) {
                if (an->tactic < 16) atype = ENEMY; else atype = OBJECTS;
            }
            else inum = statelist[inum].figure;
            if (an->race == 4) { if (an->state < DYING) inum += 0x24; }
            else if (i > 0) { if (an->race & 1) inum |= 1; else inum &= 0xfffe; }
        }
        if (atype == OBJECTS && an->race == 0) goto offscreen;     /* hidden object */
        if (atype == CARRIER && riding == 0 && actor_file == 11) { atype = RAFT; inum = 1; }
    }
rte:
    q = &seq_list[atype];
    if (!q->frames)
        goto offscreen;
    ysize = q->height;

    if (atype == OBJECTS) {
        if (inum == 0x1b || (inum >= 8 && inum <= 12) || inum == 25 || inum == 26 ||
            (inum > 0x10 && inum < 0x18) || (inum & 128)) ysize = 8;
    }
    ystop = ystart + ysize - 1;

    if (passmode == 0 && atype != OBJECTS) {
        if (i == 0 && riding == 11) ystop -= 16;
        else if (an->environ == 2) ystop -= 10;
        else if (an->environ > 29) {
            if (an->state == DEAD) goto offscreen;
            ystart += 27;
            ystop = ystart + 7;
            atype = OBJECTS;
            q = &seq_list[OBJECTS];
            inum = 97 + ((cycle + i) & 1);
        }
        else if (an->environ > 2) ystart += an->environ;
    } else {
        if (an->environ > 29) goto offscreen;          /* no weapon */
        if (an->environ > 2) {
            ystart += an->environ;
            ystop += an->environ;
            if (ystop > ground) ystop = ground;
            if (ystart >= ground) goto offscreen;
            ground += an->environ;
        }
    }

    if (xstart > VIEW_W || ystart > VIEW_H || xstart + q->wcells * 16 < 0 || ystop < 0)
        goto offscreen;

    if (atype == OBJECTS && (inum & 128)) { inum &= 0x7f; skip = 8; }
    if (inum < 0 || inum >= q->frames)
        goto offscreen;
    fetch_frame(q, (uint8_t)inum, skip);
    if (ystop - ystart + 1 < q->hcells * 16)
        cut_rows((uint8_t)(ystop - ystart + 1));

    /* occlusion unless it is a carrier, a special place, ... (fmain.c:2557) */
    nomask = 0;
    if (atype == CARRIER || (i == 0 && (riding == 11 || xtype == 84 ||
        (hero_x > 22833 && hero_x < 26428 && hero_y > 26425 && hero_y < 26527))))
        nomask = 1;
    if (atype == OBJECTS && inum > 99 && inum < 102) nomask = 1;
    if (an->race == 0x85 || an->race == 0x87) nomask = 1;
    if (!nomask)
        occlude(an, i, map_x + xstart, map_y + ystart, (uint8_t)(ystop - ystart + 1),
                map_y + ground);

    emit(xstart, ystart, (uint8_t)(ystop - ystart + 1));
    an->visible = 1;

offscreen:
    if (pass == 1)
        goto newpass;
}

void sprites_draw(void)
{
    uint8_t j;
    pat_base ^= 32;
    next_sprite = 0;
    npat = 0;
    for (j = 0; j < anix2; j++)
        draw_actor(anim_index[j]);
    for (j = next_sprite; j < last_sprites; j++)
        set_sprite(j, 0, 0, 0, 0);
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
