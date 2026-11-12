/* Banked module (PAGE_COMBAT): melee, missiles and the depth sort of the main loop -
   fmain.c lines 2238-2359. */

#include <stdint.h>
#include "world.h"
#include "state.h"
#include "actors.h"
#include "banked.h"

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
