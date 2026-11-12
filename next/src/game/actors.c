/* Actors: control, movement, combat and sorting, ported from the main loop in fmain.c
   (lines 1384-1865, 2238-2359) and fmain2.c (set_course, dohit, proxcheck, ...). */

#include <stdint.h>
#include "world.h"
#include "state.h"
#include "actors.h"
#include "banked.h"

struct shape anim_list[MAX_ACTORS];
uint8_t anix;
uint8_t cycle;
uint16_t hero_x, hero_y;

/* fmain.c:1010 */
const int8_t diroffs[16] = { 16,16,24,24,0,0,8,8,56,56,68,68,32,32,44,44 };

/* fmain.c:150 */
const struct state statelist[87] = {
    /* 0 = southwalk sequence */
    {  0,11,-2,11 },{  1,11,-3,11 },{  2,11,-3,10 },{  3,11,-3, 9 },
    {  4,11,-3,10 },{  5,11,-3,11 },{  6,11,-2,11 },{  7,11,-1,11 },
    /* 8 = westwalk sequence */
    {  8,9,-12,11 },{  9,9,-11,12 },{ 10,9,-8,13 },{ 11,9,-4,13 },
    { 12,9,0,13 },{ 13,9,-4,13 },{ 14,9,-8,13 },{ 15,9,-11,12 },
    /* 16 = northwalk sequence */
    { 16,14,-1,1 },{ 17,14,-1,2 },{ 18,14,-1,3 },{ 19,14,-1,4 },
    { 20,14,-1,3 },{ 21,14,-1,2 },{ 22,14,-1,1 },{ 23,14,-1,1 },
    /* 24 = eastwalk sequence */
    { 24,10,5,12 },{ 25,10,3,12 },{ 26,10,2,12 },{ 27,10,3,12 },
    { 28,10,5,12 },{ 29,10,6,12 },{ 30,10,6,11 },{ 31,10,6,12 },
    /* 32 = south fight = 32 + transition state */
    { 32,11,-2,12 },{ 32,10,0,12 },{ 33,0,2,10 },
    { 34,1,4,6 },{ 34,2,1,4 },{ 34,3,0,4 },
    { 36,4,-5,0 },{ 36,5,-10,1 },{ 35,12,-5,5 },
    { 36,0,0,+6 },{ 38,85,-6,5 },{ 37,81,-6,5 },
    /* 44 = west fight = 44 + transition state */
    { 40,9,-7,12 },{ 40,8,-9,9 },{ 41,7,-10,5 },
    { 42,7,-12,4 },{ 42,6,-12,3 },{ 42,5,-12,3 },
    { 44,5,-8,3 },{ 44,14,-7,6 },{ 43,13,-7,8 },
    { 42,5,-12,3 },{ 46,86,-3,0 },{ 45,82,-3,0 },
    /* 56 = north fight = 56 + transition state */
    { 48,14,-3,0 },{ 48,6,-3,-1 },{ 49,5,-2,-3 },
    { 50,5,-3,-4 },{ 50,4,0,0 },{ 50,3,3,0 },
    { 52,4,6,1 },{ 52,15,7,3 },{ 51,14,1,6 },
    { 50,4,0,0 },{ 54,87,3,0 },{ 53,83,3,0 },
    /* 68 = east fight = 68 + transition state */
    { 56,10,5,11 },{ 56,0,6,9 },{ 57,1,10,6 },
    { 58,1,10,5 },{ 58,2,7,3 },{ 58,3,6,3 },
    { 60,4,1,0 },{ 60,3,3,2 },{ 59,15,4,1 },
    { 58,4,5,1 },{ 62,84,3,0 },{ 61,80,3,0 },
    /* 80 = death sequence 1 */
    { 47,0,5,11 },{ 63,0,6,9 },{ 39,0,6,9 },
    /* 83 = sinking sequence 1 */
    { 55,10,5,11 },
    /* 84, 85 - oscillations */
    { 64,10,5,11 },{ 65,10,5,11 },
    /* 86 - asleep */
    { 66,10,5,11 }
};

/* fmain.c:137 - fight animation transitions */
static const int8_t trans_list[9][4] = {
    {  1, 8, 0, 1 },    /* 0 - arm down, weapon low */
    {  2, 0, 1, 0 },    /* 1 - arm down, weapon diagonal down */
    {  3, 1, 2, 8 },    /* 2 - arm swing1, weapon horizontal */
    {  4, 2, 3, 7 },    /* 3 - arm swing2, weapon raised */
    {  5, 3, 4, 6 },    /* 4 - arm swing2, weapon diag up */
    {  6, 4, 5, 5 },    /* 5 - arm swing2, weapon high */
    {  8, 5, 6, 4 },    /* 6 - arm high, weapon up */
    {  8, 6, 7, 3 },    /* 7 - arm high, weapon horizontal */
    {  0, 6, 8, 2 },    /* 8 - arm middle, weapon raise fwd */
};

/* fmain2.c:871 */
static const uint8_t fallstates[24] = {
    0,0,0,0,0,0,
    0x20,0x22,0x3a,0x6f,0x70,0x71,
    0x24,0x27,0x3c,0x6f,0x70,0x71,
    0x37,0x38,0x3d,0x6f,0x70,0x71 };

/* fmain2.c:877 */
const int8_t bow_x[32] = {
    1,2,3,4,3,2,1,0, 3,2,0,-2,-3,-2,0,2,
    -3,-3,-3,-3,-3,-3,-3,-2, 0,1,1,1,0,-2,-3,-2 };
const int8_t bow_y[32] = {
    8,8,8,7,8,8,8,8, 11,12,13,13,13,13,13,12,
    8,7,6,5,6,7,8,9, 12,12,12,12,12,12,11,12 };
static const int8_t bowshotx[8] = {  0, 0, 3, 6,-3,-3,-3,-6 };
static const int8_t bowshoty[8] = { -6,-6,-1, 0, 6, 8, 0,-1 };
static const int8_t gunshoty[8] = {  2, 0, 4, 7, 9, 4, 7, 8 };

/* fsubs.asm:1276 */
static const int8_t xdir[10] = { -2,0,2,3,2,0,-2,-3,0,0 };
static const int8_t ydir[10] = { -2,-3,-2,0,2,3,2,0,0,0 };

/* fsubs.asm _newx: x + xdir*speed/2 (logical shift of the 16-bit product), 15 bits */
uint16_t newx(uint16_t x, uint8_t dir, int16_t speed)
{
    if (dir > 7)
        return x;
    return (x + ((uint16_t)(xdir[dir] * speed) >> 1)) & 0x7FFF;
}

/* fsubs.asm _newy: same, but keeps bit 15 (indoor maps) */
uint16_t newy(uint16_t y, uint8_t dir, int16_t speed)
{
    if (dir > 7)
        return y;
    return ((y + ((uint16_t)(ydir[dir] * speed) >> 1)) & 0x7FFF) | (y & 0x8000);
}

/* fsubs.asm _prox: terrain at the two feet */
static uint8_t prox(uint16_t x, uint16_t y)
{
    uint8_t t = px_to_im(x + 4, y + 2);
    if (t == 1 || t >= 10)
        return t;
    t = px_to_im(x - 4, y + 2);
    if (t == 1 || t >= 8)
        return t;
    return 0;
}

/* fmain2.c:277 */
uint8_t proxcheck(uint16_t x, uint16_t y, uint8_t i)
{
    uint8_t j, r;
    int16_t x1, y1;

    if (anim_list[i].type != ENEMY || anim_list[i].race != 2) {     /* wraith */
        r = prox(x, y);
        if (i == 0 && (r == 8 || r == 9))
            r = 0;
        if (r)
            return r;
    }
    for (j = 0; j < anix; j++) {
        if (i != j && j != 1 && anim_list[j].type != 5 && anim_list[j].state != DEAD) {
            x1 = (int16_t)(x - anim_list[j].abs_x);
            y1 = (int16_t)(y - anim_list[j].abs_y);
            if (x1 < 11 && x1 > -11 && y1 < 9 && y1 > -9)
                return 16;
        }
    }
    return 0;
}

/* fmain2.c:311 */
int16_t calc_dist(uint8_t a, uint8_t b)
{
    int16_t x = (int16_t)(anim_list[a].abs_x - anim_list[b].abs_x);
    int16_t y = (int16_t)(anim_list[a].abs_y - anim_list[b].abs_y);
    if (x < 0) x = -x;
    if (y < 0) y = -y;
    if (x > y + y) return x;
    if (y > x + x) return y;
    return (int16_t)(((int32_t)(x + y) * 5) / 7);
}

/* fmain2.c:322 */
uint8_t move_figure(uint8_t fig, uint8_t dir, int8_t dist)
{
    uint16_t xtest = newx(anim_list[fig].abs_x, dir, dist);
    uint16_t ytest = newy(anim_list[fig].abs_y, dir, dist);
    if (proxcheck(xtest, ytest, fig))
        return FALSE;
    anim_list[fig].abs_x = xtest;
    anim_list[fig].abs_y = ytest;
    return TRUE;
}

/* fmain2.c:63 (asm) */
static const uint8_t com2[9] = { 0, 1, 2, 7, 9, 3, 6, 5, 4 };

void set_course(uint8_t object, uint16_t target_x, uint16_t target_y, uint8_t mode)
{
    struct shape *an = &anim_list[object];
    int16_t xdif, ydif, xabs, yabs;
    int8_t xd = 0, yd = 0, deviation = 0;
    uint8_t j;

    if (mode == 6) {
        xdif = (int16_t)target_x;
        ydif = (int16_t)target_y;
    } else {
        xdif = (int16_t)(an->abs_x - target_x);
        ydif = (int16_t)(an->abs_y - target_y);
    }
    xabs = xdif; yabs = ydif;
    if (xdif > 0) xd = 1;
    else if (xdif < 0) { xabs = -xdif; xd = -1; }
    if (ydif > 0) yd = 1;
    else if (ydif < 0) { yabs = -ydif; yd = -1; }
    if (mode != 4) {
        if ((uint16_t)xabs >> 1 > (uint16_t)yabs) yd = 0;
        if ((uint16_t)yabs >> 1 > (uint16_t)xabs) xd = 0;
    }
    if (mode == 1 && xabs + yabs < 40) deviation = 1;
    else if (mode == 2 && xabs + yabs < 30) deviation = 1;   /* sic: asm sets 1 */
    if (mode == 3) { xd = -xd; yd = -yd; }
    j = com2[4 - yd - yd - yd - xd];
    if (j == 9) {
        an->state = STILL;
        return;
    }
    if (rand16() & 2) j += deviation; else j -= deviation;
    an->facing = j & 7;
    if (mode != 5)
        an->state = WALKING;
}

/* fmain.c:2769 */
void checkdead(uint8_t i, uint8_t dtype)
{
    struct shape *an = &anim_list[i];
    if (an->vitality < 1 && an->state != DYING && an->state != DEAD) {
        an->vitality = 0;
        an->tactic = 7;
        an->goal = DEATH;
        an->state = DYING;
        if (an->race == 7) speak(42);
        else if (an->type == SETFIG && an->race != 0x89) kind -= 3;
        if (i) brave++;
        else { event(dtype); luck -= 5; setmood(TRUE); }
        if (kind < 0) kind = 0;
        prq(7);
    }
    if (i == 0) prq(4);
}

/* fmain2.c:230 - a hit, a palpable hit! */
void dohit(int8_t i, uint8_t j, uint8_t fc, int16_t wt)
{
    if (anim_list[0].weapon < 4 &&
        (anim_list[j].race == 9 || (anim_list[j].race == 0x89 && stuff[7] == 0))) {
        speak(58);
        return;
    }
    if (anim_list[j].race == 0x8a || anim_list[j].race == 0x8b)
        return;
    anim_list[j].vitality -= wt;
    if (anim_list[j].vitality < 0) anim_list[j].vitality = 0;
    if (i == -1) effect(2, 500 + rand64());
    else if (i == -2) effect(5, 3200 + bitrand(511));
    else if (j == 0) effect(0, 800 + bitrand(511));
    else effect(3, 400 + rand256());
    /* push back the foe!! Yar! */
    if (anim_list[j].type != DRAGON && anim_list[j].type != SETFIG
        && move_figure(j, fc, 2) && i >= 0)
        move_figure((uint8_t)i, fc, 2);
    checkdead(j, 5);
}

/* fmain.c:1384-1462 - the hero's state from the controls; dir 0..7 or 9, fire = button */
void hero_control(uint8_t dir, uint8_t fire)
{
    int8_t inum;
    struct shape *an;

    if (light_timer) light_timer--;
    if (secret_timer) secret_timer--;
    if (freeze_timer) freeze_timer--;

    fiery_death = (map_x > 8802 && map_x < 13562 && map_y > 24744 && map_y < 29544);

    inum = anim_list[0].state;
    if (inum == DEAD || inum == FALL) {
        if (goodfairy == 1) { revive(FALSE); inum = STILL; }
        else if (--goodfairy < 20) ;                    /* resurrection glow */
        else if (luck < 1 && goodfairy < 200) { revive(TRUE); inum = STILL; }
        else if (anim_list[0].state == FALL && goodfairy < 200) { revive(FALSE); inum = STILL; }
        else if (goodfairy < 120) {
            an = &anim_list[3];
            anix = 4;
            an->abs_x = hero_x + goodfairy * 2 - 20;
            an->abs_y = hero_y;
            an->type = OBJECTS;
            an->index = 100 + (cycle & 1);
            an->state = STILL;
            an->weapon = an->environ = 0;
            an->race = 0xff;
            actors_on_screen = TRUE;
            battleflag = FALSE;
        }
    }
    else if (inum == DYING || inum == SINK || inum == SLEEP)
        ;
    else if (fire) {
        int8_t dif_x = anim_list[0].vel_x, dif_y = anim_list[0].vel_y;
        if (xtype > 80) {
            if (inum != SHOOT1 && xtype == 81) event(15);
            if (inum != SHOOT1 && xtype == 82) event(16);
            inum = SHOOT1;
        }
        else if (riding == 11) {
            if (fiery_death) event(32);
            else if (dif_x > -15 && dif_x < 15 && dif_y > -15 && dif_y < 15) {
                uint16_t ytest = anim_list[0].abs_y - 14;
                if (proxcheck(anim_list[0].abs_x, ytest, 0) == 0 &&
                    proxcheck(anim_list[0].abs_x, ytest + 10, 0) == 0) {
                    riding = 0;                         /* dismount */
                    anim_list[0].abs_y = ytest;
                }
            }
            else event(33);
        }
        else {
            if (dir < 9) anim_list[0].facing = dir;
            if (inum >= WALKING) {
                if (anim_list[0].weapon == 4) inum = SHOOT1;
                else if (anim_list[0].weapon == 5) { if (inum < SHOOT1) inum = SHOOT1; }
                else inum = FIGHTING;
            }
        }
    }
    else if (inum == SHOOT1) inum = SHOOT3;            /* release */
    else {
        if (dir < 9) {
            if (hunger > 120 && !rand4()) {
                if (rand16() & 1) dir = (dir + 1) & 7;
                else dir = (dir - 1) & 7;
            }
            anim_list[0].facing = dir;
            inum = WALKING;
        }
        else inum = STILL;
    }
    anim_list[0].state = inum;
}

void actors_update(void)
{
    uint8_t i, j, d, s, wcarry_i;
    int8_t k, e, dex;
    int16_t xstart, ystart;
    uint16_t xtest, ytest;
    struct shape *an;
    struct missile *ms;

    /* determine the monster and character motion effects (fmain.c:1455) */
    raftprox = turtleprox = FALSE;
    wcarry_i = active_carrier ? 3 : 1;
    wcarry = wcarry_i;
    xstart = (int16_t)(anim_list[0].abs_x - anim_list[wcarry_i].abs_x - 4);
    ystart = (int16_t)(anim_list[0].abs_y - anim_list[wcarry_i].abs_y - 4);
    if (xstart < 16 && xstart > -16 && ystart < 16 && ystart > -16) raftprox = 1;
    if (xstart < 9 && xstart > -9 && ystart < 9 && ystart > -9) raftprox = 2;
    if (riding == 11) anim_list[0].environ = -2;

    for (i = 0; i < anix; i++) {
        an = &anim_list[i];
        if (freeze_timer && i > 0) goto statc;
        k = an->environ;
        d = an->facing;
        s = an->state;
        dex = an->index;
        ms = &missile_list[mdex];
        j = 0;
        xtest = an->abs_x;
        ytest = an->abs_y;

        if (an->type == OBJECTS) { j = 0; goto raise; }
        if (an->type == DRAGON) {
            dex = 0;
            if (s == DYING) dex = 3;
            else if (s == DEAD) dex = 4;
            else if (rand4() == 0) {
                ms->speed = 5;
                mdex++;
                dex = rand2() + 1;
                effect(5, 1800 + rand256());
                an->facing = 5;
                ms->missile_type = 2;
                d = an->facing;
                goto dragshoot;
            }
            else dex = 0;
            goto after_chain;
        }
        else if (an->type == CARRIER) {
            k = j = 0;
            if (actor_file == 11) {                     /* swan */
                if (raftprox && wcarry_i == 3 && stuff[5]) {
                    d = anim_list[0].facing;
                    xtest = an->abs_x = anim_list[0].abs_x;
                    ytest = an->abs_y = anim_list[0].abs_y;
                    riding = 11;
                } else {
                    xtest = an->abs_x;
                    ytest = an->abs_y;
                }
                dex = d;
                e = 0;
            } else {                                    /* turtle */
                if (raftprox && wcarry_i == 3) {
                    d = anim_list[0].facing;
                    xtest = anim_list[0].abs_x;
                    ytest = anim_list[0].abs_y;
                    riding = 5;
                    dex = d + d;
                    if (anim_list[0].state == WALKING) dex += (cycle & 1);
                } else {
                    xtest = newx(an->abs_x, d, 3);
                    ytest = newy(an->abs_y, d, 3);
                    if (px_to_im(xtest, ytest) != 5) {
                        d = (d + 1) & 7;
                        xtest = newx(an->abs_x, d, 3);
                        ytest = newy(an->abs_y, d, 3);
                        if (px_to_im(xtest, ytest) != 5) {
                            d = (d - 2) & 7;
                            xtest = newx(an->abs_x, d, 3);
                            ytest = newy(an->abs_y, d, 3);
                            if (px_to_im(xtest, ytest) != 5) {
                                d = (d - 1) & 7;
                                xtest = newx(an->abs_x, d, 3);
                                ytest = newy(an->abs_y, d, 3);
                            }
                        }
                    }
                    riding = FALSE;
                    dex = d + d + (cycle & 1);
                }
                j = px_to_im(xtest, ytest);
                if (j == 5) { an->abs_x = xtest; an->abs_y = ytest; }
                e = 1;
            }
            move_extent(e, xtest, ytest);
            goto raise;
        }
        else if (an->type == SETFIG) {
            j = an->race & 0x7f;                        /* id# of setfig */
            dex = setfig_image_base(j);
            if (s == DYING) { dex += 2; if (j == 9) dex += 2; }
            else if (s == DEAD) { dex += 3; if (j == 9) dex += 2; }
            else if (j == 9) {
                set_course(i, hero_x, hero_y, 0);
                dex = an->facing / 2;
                witchflag = TRUE;
            }
            else if (s == TALKING) {
                dex += rand2();
                if (!(--(an->tactic))) an->state = STILL;
            }
            else goto statc;
            k = j = 0;
            goto after_chain;
        }
        else if (an->type == RAFT) {
            riding = FALSE;
            if (wcarry_i != 1 || raftprox != 2) goto statc;
            xtest = anim_list[0].abs_x;                 /* raft follows character */
            ytest = anim_list[0].abs_y;
            j = px_to_im(xtest, ytest);
            if (j < 3 || j > 5) goto statc;
            an->abs_x = xtest;
            an->abs_y = ytest;
            riding = 1;
            goto statc;
        }
        else if (s == SINK) {
            if (an->vitality > 0) dex = 83;
            frustflag = 0;
            goto cpx;
        }
        else if (s == WALKING) {
            if (k == -2) {
                int8_t nvx, nvy, nvx1, nvy1;
                e = (riding == 11) ? 40 : 42;
                nvx1 = nvx = an->vel_x + (int8_t)(newx(20, d, 2) - 20);
                nvy1 = nvy = an->vel_y + (int8_t)(newy(20, d, 2) - 20);
                if (nvx1 < 0) nvx1 = -nvx;
                if (nvy1 < 0) nvy1 = -nvy;
                if (nvx1 < e - 8) an->vel_x = nvx;
                if (nvy1 < e) an->vel_y = nvy;
                xtest = an->abs_x + an->vel_x / 4;
                ytest = an->abs_y + an->vel_y / 4;
                if (riding == 11) {
                    set_course(0, (uint16_t)(-nvx), (uint16_t)(-nvy), 6);
                    d = an->facing;
                    goto newloc;
                }
                if (proxcheck(xtest, ytest, i) == 0) goto newloc;
                k = 0;
            }
            if (i == 0 && riding == 5) e = 3;
            else if (k == -3) e = -2;                   /* walk backwards */
            else if (k == -1) e = 4;
            else if (k == 2 || k > 6) e = 1;
            else e = 2;

            xtest = newx(an->abs_x, d, e);
            ytest = newy(an->abs_y, d, e);
            j = proxcheck(xtest, ytest, i);
            if (i == 0) {
                if (j == 15) doorfind(xtest, ytest, 0);
                else bumped = 0;
                if (stuff[30] && j == 12) goto newloc;
            }
            if (j) {
                d = (d + 1) & 7;                        /* checkdev1 */
                xtest = newx(an->abs_x, d, e);
                ytest = newy(an->abs_y, d, e);
                if (proxcheck(xtest, ytest, i)) {
                    d = (d - 2) & 7;                    /* checkdev2 */
                    xtest = newx(an->abs_x, d, e);
                    ytest = newy(an->abs_y, d, e);
                    if (proxcheck(xtest, ytest, i))
                        goto blocked;
                }
            }
newloc:
            an->facing = d;
            dex = diroffs[d];
            if (!(riding && i == 0) && an->race != 2)
                dex += (cycle + i) & 7;
            if (k == -2) goto cpx;
            if (k == -3) dex ^= 7;
            j = px_to_im(xtest, ytest);
            if (an->race == 2 || an->race == 4) j = 0;  /* wraiths and snakes immune to water */
            if (an->race == 4) dex = (cycle & 2) + ((d & 0xFE) << 2) + 0x24;
            if (k > 2) {
                if (j == 0 || (j == 3 && k > 5) || (j == 4 && k > 10)) {
                    if (hero_sector != 181) k--;
                    goto raise;
                }
            }
            an->vel_x = (int8_t)((int16_t)(xtest - an->abs_x) * 4);
            an->vel_y = (int8_t)((int16_t)(ytest - an->abs_y) * 4);
            an->abs_x = xtest;
            an->abs_y = ytest;
            frustflag = 0;
            goto after_chain;
        }
        else if (s == STILL) {
            dex = diroffs[d] + 1;
            if (k == -2) goto cpx;
            j = px_to_im(an->abs_x, an->abs_y);
            goto after_chain;
        }
        else if (s >= SHOOT1) {
            /* no shooting underwater or in pax areas */
            if (k > 15 || xtype > 80) goto cpx;
            if (s == SHOOT3) {
                if (an->weapon == 5) { dex = diroffs[d + 8]; goto cpx; }
                dex = diroffs[d + 8] + 11;
                an->state = STILL;
                if (i == 0) { if (stuff[8] == 0) goto cpx; else stuff[8]--; }
                ms->speed = 3;
                mdex++;
                effect(4, 400 + rand256());
            }
            else if (s == SHOOT1) {
                if (an->type == ENEMY) dex = diroffs[d + 8] + 11;
                else dex = diroffs[d + 8] + 10;
                if (an->weapon == 5) {
                    ms->speed = 5;
                    mdex++;
                    an->state = SHOOT3;
                    dex = diroffs[d + 8];
                    effect(5, 1800 + rand256());
                }
                else if (i == 0 && stuff[8] == 0) { print("No Arrows!"); goto cpx; }
                else ms->speed = 0;
            }
            ms->missile_type = an->weapon - 3;
dragshoot:
            ms->abs_x = an->abs_x + bowshotx[d];
            ms->abs_y = an->abs_y;
            if (an->weapon == 4) ms->abs_y += bowshoty[d];
            else ms->abs_y += gunshoty[d];
            ms->time_of_flight = 0;
            ms->direction = an->facing;
            ms->archer = i;
            if (mdex > 5) mdex = 0;
            frustflag = 0;
            goto cpx;
        }
        else if (s < 9) {
            int8_t inum = diroffs[d + 8];
            s = an->state = trans_list[s][rand4()];
            if ((i > 2 && s == 6) || s == 7) s = 8;     /* original precedence: i>2&&s==6 || s==7 */
            dex = s + inum;
            frustflag = 0;
            goto cpx;
        }
        else {
            if (s == DYING) {
                if (an->tactic > 4) { if (d == 0 || d > 4) dex = 80; else dex = 81; }
                else if (an->tactic > 0) { if (d == 0 || d > 4) dex = 81; else dex = 80; }
                else { an->state = DEAD; dex = 82; }
                frustflag = 0;
            }
            else if (s == DEAD) dex = 82;
            else if (s == FROZEN) dex = 82;
            else if (s == OSCIL) dex = 84 + (cycle & 1);
            else if (s == OSCIL + 1) dex = 84;
            else if (s == SLEEP) dex = 86;
            else if (s == FALL) {
                if (an->tactic >= 30) goto cpx;
                j = (an->tactic / 5) + (brother * 6);
                dex = fallstates[j];
                an->tactic++;
                an->vel_x = (an->vel_x * 3) / 4;
                an->vel_y = (an->vel_y * 3) / 4;
            }
            goto cpx;
        }

blocked:
        if (i == 0) {
            frustflag++;
            if (frustflag > 40) dex = 40;
            else if (frustflag > 20) dex = 84 + ((cycle >> 1) & 1);
        }
cpx:
        j = px_to_im(an->abs_x, an->abs_y);
        if (k == -2) {
            an->abs_x += an->vel_x / 4;
            an->abs_y += an->vel_y / 4;
        }
after_chain:
        if (s == DYING && !(--(an->tactic))) {
            an->state = DEAD;
            if (an->race == 0x09) {
                an->race = 10;
                an->vitality = 10;
                an->state = STILL;
                an->weapon = 0;
                leave_item(i, 139);                     /* leave the talisman */
            }
            if (an->race == 0x89) leave_item(i, 27);    /* leave the lasso */
        }

        /* sinker: j is the terrain type */
        if (i == 0 && raftprox) k = 0;                  /* can't drown on raft */
        else if (j == 0) k = 0;
        else if (j == 6) k = -1;
        else if (j == 7) k = -2;
        else if (j == 8) k = -3;
        else if (j == 9 && i == 0 && xtype == 52) {
            if (an->state != FALL) {
                dex = fallstates[brother * 6];
                an->state = FALL;
                an->tactic = 0;
                luck -= 2;
                setmood(TRUE);
            }
            k = -2;
        }
        else if (j == 2) k = 2;
        else if (j == 3) k = 5;
        else if (j == 4 || j == 5) {
            e = (j == 4) ? 10 : 30;
            if (k > e) k--;
            else if (k < e) {
                k++;
                if (s == DYING || s == DEAD)
                    ;
                else if (k == 30) {
                    if (hero_sector == 181) {
                        if (i == 0) {
                            k = 0;
                            new_region = 9;
                            xfer(0x1080, 34950, FALSE);
                            find_place(1);
                        }
                        else an->vitality = 0;
                    }
                    an->state = STILL;
                }
                else if (k > 15) an->state = SINK;
            }
        }
        if (k == 0 && s == SINK) an->state = STILL;
raise:
        if (new_region >= NO_REGION) an->environ = k;

        k = an->race;
        if (an->type == ENEMY) {
            if (k == 4 && an->state < WALKING) dex = (cycle & 1) + diroffs[d];
            else if (k == 4 && an->state < DYING) dex = ((cycle / 2) & 1) + diroffs[d];
            else if (k == 8) {
                if (an->state == DEAD) an->abs_x = 0;
                else if (an->state == DYING) dex = 0x3f;
                else {
                    dex = (cycle & 3) * 2;
                    if (dex > 4) dex--;
                    switch (i % 3) {
                    case 0: dex = 0x25; break;
                    case 1: dex += 0x28; break;
                    case 2: dex += 0x30; break;
                    }
                }
            }
            else if (k == 7 && an->vitality == 0) {
                /* original: an->state == STILL; (a no-op comparison) */
                dex = 1;
            }
        }
        an->index = dex;

        if (i == 0) {
            if (region_num < 8) {
                if (an->abs_x < 300) an->abs_x = 32565;
                else if (an->abs_x > 32565) an->abs_x = 300;
                else if (an->abs_y < 300) an->abs_y = 32565;
                else if (an->abs_y > 32565) an->abs_y = 300;
                else goto jkl;
                if (riding > 1) {
                    anim_list[3].abs_x = an->abs_x;
                    anim_list[3].abs_y = an->abs_y;
                }
jkl:            ;
            }
            hero_x = an->abs_x;
            hero_y = an->abs_y;
            map_adjust(hero_x, hero_y);
            safe_flag = j;
        }
        if (an->vitality == 0) goto statc;
        if (fiery_death) {
            if (i == 0 && stuff[23]) an->environ = 0;
            else if (an->environ > 15) an->vitality = 0;
            else if (an->environ > 2) an->vitality--;
            checkdead(i, 27);
        }
        if (an->environ == 30 && (cycle & 7) == 0) {
            if (k != 2 && k != 3) { an->vitality--; checkdead(i, 6); }
        }
statc:
        if (an->type == CARRIER && riding == 11) {
            an->rel_x = (int16_t)(an->abs_x - map_x - 32);
            an->rel_y = (int16_t)(an->abs_y - map_y - 40);
        }
        else if (an->type == RAFT || an->type == CARRIER || an->type == DRAGON) {
            an->rel_x = (int16_t)(an->abs_x - map_x - 16);
            an->rel_y = (int16_t)(an->abs_y - map_y - 16);
        }
        else {
            an->rel_x = (int16_t)(an->abs_x - map_x - 8);
            an->rel_y = (int16_t)(an->abs_y - map_y - 26);
        }
    }

    if (anim_list[0].state != STILL) sleepwait = 0;
    hero_x = anim_list[0].abs_x;
    hero_y = anim_list[0].abs_y;
}

/* fmain.c:2238-2301 - melee and missiles */
void combat_update(void)
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
void actors_sort(void)
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
