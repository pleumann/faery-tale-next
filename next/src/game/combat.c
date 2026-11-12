/* Banked module (PAGE_COMBAT): melee, missiles and the depth sort of the main loop -
   fmain.c lines 2238-2359. */

#include <stdint.h>
#include "world.h"
#include "state.h"
#include "actors.h"
#include "banked.h"
#include "../platform/hw.h"

/* fmain.c:2238-2301 - melee and missiles */
void combat_update_b(void)
{
    uint8_t i, j;
    int16_t xd, yd;

    for (i = 0; i < anix; i++) {
        int8_t w, fc;
        int16_t bv;
        uint16_t xs, ys;
        if (i > 0 && freeze_timer) break;
        if (i == 1 || anim_list[i].state >= WALKING) continue;
        w = anim_list[i].weapon;
        fc = anim_list[i].facing;
        if (!(w & 4)) {
            if (w >= 8) w = 5;                          /* tone down touch attack! */
            w += bitrand(2);                            /* sometimes extended */
            xs = newx(anim_list[i].abs_x, fc, w + w) + rand8() - 3;
            ys = newy(anim_list[i].abs_y, fc, w + w) + rand8() - 3;
            if (i == 0) bv = (brave / 20) + 5; else bv = 2 + rand4();
            if (bv > 14) bv = 15;                       /* bravery limit */
            for (j = 0; j < anix; j++) {
                if (j == 1 || j == i || anim_list[j].state == DEAD ||
                    anim_list[i].type == CARRIER) continue;
                xd = (int16_t)(anim_list[j].abs_x - xs);
                yd = (int16_t)(anim_list[j].abs_y - ys);
                if (xd < 0) xd = -xd;
                if (yd < 0) yd = -yd;
                if (xd > yd) yd = xd;
                if ((i == 0 || (int16_t)rand256() > brave) && yd < bv && !freeze_timer) {
                    dohit(i, j, fc, w);
                    break;
                }
                else if (yd < bv + 2 && w != 5) effect(1, 150 + rand256());
            }
        }
    }

    if (!freeze_timer) for (i = 0; i < 6; i++) {
        struct missile *ms = &missile_list[i];
        int8_t s = ms->speed * 2, fc, mt;
        int16_t bv;
        uint16_t xs, ys;
        if (ms->missile_type == 0 || ms->missile_type == 3 || s == 0 || ms->time_of_flight++ > 40) {
            ms->missile_type = 0;
            continue;
        }
        j = px_to_im(xs = ms->abs_x, ys = ms->abs_y);
        if (j == 1 || j == 15) { ms->missile_type = 0; s = 0; }
        fc = ms->direction;
        mt = (ms->missile_type == 2) ? 9 : 6;
        for (j = 0; j < anix; j++) {
            bv = (j == 0) ? brave : 20;
            if (j == 1 || ms->archer == j || anim_list[j].state == DEAD ||
                anim_list[j].type == CARRIER) continue;
            xd = (int16_t)(anim_list[j].abs_x - xs);
            yd = (int16_t)(anim_list[j].abs_y - ys);
            if (xd < 0) xd = -xd;
            if (yd < 0) yd = -yd;
            if (xd > yd) yd = xd;
            /* original tests the missile index i here, not the target j (kept) */
            if ((i != 0 || (int16_t)bitrand(512) > bv) && yd < mt) {
                if (ms->missile_type == 2) dohit(-2, j, fc, rand8() + 4);
                else dohit(-1, j, fc, rand8() + 4);
                ms->speed = 0;
                if (ms->missile_type == 2) ms->missile_type = 3;
                break;
            }
        }
        ms->abs_x = newx(ms->abs_x, fc, s);
        ms->abs_y = newy(ms->abs_y, fc, s);
    }
}

/* fmain.c:2303-2359 - objects, missiles as sprites, depth sort, nearest person */
void actors_sort_b(void)
{
    uint8_t i, j;

    anix2 = anix;
    do_objects();

    for (i = 0; i < 6; i++) {
        struct missile *ms = &missile_list[i];
        int8_t t = ms->missile_type;
        int16_t xs, ys;
        struct shape *an;
        if (!t) continue;
        xs = (int16_t)(ms->abs_x - map_x - 9);
        ys = (int16_t)(ms->abs_y - map_y - 26);
        if (xs > -20 && xs < 340 && ys > -20 && ys < 190) {
            an = &anim_list[anix2];
            an->abs_x = ms->abs_x; an->abs_y = ms->abs_y;
            an->rel_x = xs; an->rel_y = ys;
            an->type = OBJECTS; an->weapon = 0;
            an->index = ms->direction;
            an->race = 0xff;
            an->state = STILL;
            an->environ = 0;
            if (t == 3) an->index = 0x58;
            else if (t == 2) an->index += 0x59;
            if (++anix2 >= 20) break;
        }
    }

    nearest_person = 0;
    perdist = 50;
    for (i = 0; i < anix2; i++) anim_index[i] = i;
    for (i = 0; i < anix2; i++) {
        struct shape *an = &anim_list[i];
        if (i && an->type != OBJECTS && an->state != DEAD && riding != 11) {
            int16_t d = calc_dist(i, 0);
            if (d < perdist) { perdist = d; nearest_person = i; }
        }
        for (j = 1; j < anix2; j++) {
            uint8_t k1 = anim_index[j - 1], k2 = anim_index[j];
            /* abs_y has bit 15 set indoors for everybody: compare unsigned */
            uint16_t u1 = anim_list[k1].abs_y, u2 = anim_list[k2].abs_y;
            if (anim_list[k1].state == DEAD || (k2 == 0 && riding) || k1 == 1) u1 -= 32;
            if (anim_list[k2].state == DEAD || (k1 == 0 && riding) || k2 == 1) u2 -= 32;
            if (anim_list[k1].environ > 25) u1 += 32;
            if (anim_list[k2].environ > 25) u2 += 32;
            if (u2 < u1) { anim_index[j - 1] = k2; anim_index[j] = k1; }
        }
    }
}

/* ---- the witch's gaze (fmain2.c witch_fx, fmain.c:2367-2376) ---- */

/* fmain2.c:889 - 64 points: (x, y) far and (x, y) near, around a circle */
static const int8_t witchpoints[256] = {
0,100,0,10,     9,99,0,9,         19,98,1,9,        29,95,2,9,
38,92,3,9,        47,88,4,8,        55,83,5,8,        63,77,6,7,
70,70,7,7,        77,63,7,6,        83,55,8,5,        88,47,8,4,
92,38,9,3,        95,29,9,2,        98,19,9,1,        99,9,9,0,
100,0,10,0,        99,-10,9,-1,    98,-20,9,-2,    95,-30,9,-3,
92,-39,9,-4,    88,-48,8,-5,    83,-56,8,-6,    77,-64,7,-7,
70,-71,7,-8,    63,-78,6,-8,    55,-84,5,-9,    47,-89,4,-9,
38,-93,3,-10,    29,-96,2,-10,    19,-99,1,-10,    9,-100,0,-10,
0,-100,0,-10,    -10,-100,-1,-10,-20,-99,-2,-10,    -30,-96,-3,-10,
-39,-93,-4,-10,    -48,-89,-5,-9,    -56,-84,-6,-9,    -64,-78,-7,-8,
-71,-71,-8,-8,    -78,-64,-8,-7,    -84,-56,-9,-6,    -89,-48,-9,-5,
-93,-39,-10,-4,    -96,-30,-10,-3,    -99,-20,-10,-2,    -100,-10,-10,-1,
-100,-1,-10,-1,    -100,9,-10,0,    -99,19,-10,1,    -96,29,-10,2,
-93,38,-10,3,    -89,47,-9,4,    -84,55,-9,5,    -78,63,-8,6,
-71,70,-8,7,    -64,77,-7,7,    -56,83,-6,8,    -48,88,-5,8,
-39,92,-4,9,    -30,95,-3,9,    -20,98,-2,9,    -10,99,-1,9,
};

static uint8_t witchindex;
static int8_t wdir = 1;
static int16_t s1, s2;
static uint8_t drawn;
static uint16_t wq[8];                  /* the drawn outline in world coordinates */

#define L2W ((uint8_t *)0x6000)

static void xor_pixel(uint16_t wx, uint16_t wy)
{
    uint16_t bx = wx % 320;
    uint8_t by = (uint8_t)wy;
    mmu_set(3, L2_PAGE_A + (bx >> 5));
    L2W[((bx & 31) << 8) + by] ^= 31;   /* COMPLEMENT on 5 bitplanes */
}

static void xor_line(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
    int16_t dx = (int16_t)(x1 - x0), dy = (int16_t)(y1 - y0), sx = 1, sy = 1, err, e2;
    if (dx < 0) { dx = -dx; sx = -1; }
    if (dy < 0) { dy = -dy; sy = -1; }
    dy = -dy;
    err = dx + dy;
    for (;;) {
        xor_pixel(x0, y0);
        if (x0 == x1 && y0 == y1) break;
        e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

static void outline(void)
{
    xor_line(wq[0], wq[1], wq[2], wq[3]);
    xor_line(wq[2], wq[3], wq[6], wq[7]);
    xor_line(wq[6], wq[7], wq[4], wq[5]);
    xor_line(wq[4], wq[5], wq[0], wq[1]);
    mmu_set(3, 11);
}

/* before the playfield is updated: remove last frame's outline */
void witch_undo_b(void)
{
    if (drawn) { outline(); drawn = 0; }
}

/* after the update: hit test and new outline */
void witch_draw_b(void)
{
    uint16_t wx, wy;
    const int8_t *w;
    int16_t dx, dy, dx1, dy1;

    if (rand4() == 0) wdir = (s1 > 0) ? 1 : -1;
    witchindex += wdir;
    if (!witchflag) return;

    wx = anim_list[2].abs_x;
    wy = anim_list[2].abs_y - 15;
    w = witchpoints + (uint8_t)((witchindex + 63) * 4);
    wq[0] = wx + w[0]; wq[1] = wy + w[1]; wq[2] = wx + w[2]; wq[3] = wy + w[3];
    w = witchpoints + (uint8_t)((witchindex + 1) * 4);
    wq[4] = wx + w[0]; wq[5] = wy + w[1]; wq[6] = wx + w[2]; wq[7] = wy + w[3];

    dx = (int16_t)(wq[2] - wq[0]); dy = (int16_t)(wq[3] - wq[1]);
    dx1 = (int16_t)(hero_x - wq[0]); dy1 = (int16_t)(hero_y - wq[1]);
    s1 = dx * dy1 - dy * dx1;
    dx = (int16_t)(wq[6] - wq[4]); dy = (int16_t)(wq[7] - wq[5]);
    dx1 = (int16_t)(hero_x - wq[4]); dy1 = (int16_t)(hero_y - wq[5]);
    s2 = dx * dy1 - dy * dx1;

    if (s1 > 0 && s2 < 0 && calc_dist(2, 0) < 100)
        dohit(-1, 0, anim_list[2].facing, rand2() + 1);

    outline();
    drawn = 1;
}
