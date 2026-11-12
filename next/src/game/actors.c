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





/* fmain2.c:877 */
const int8_t bow_x[32] = {
    1,2,3,4,3,2,1,0, 3,2,0,-2,-3,-2,0,2,
    -3,-3,-3,-3,-3,-3,-3,-2, 0,1,1,1,0,-2,-3,-2 };
const int8_t bow_y[32] = {
    8,8,8,7,8,8,8,8, 11,12,13,13,13,13,13,12,
    8,7,6,5,6,7,8,9, 12,12,12,12,12,12,11,12 };

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
    if (x + y > 6000) return x + y;             /* far away anyway; keeps the math 16 bit */
    return (int16_t)((uint16_t)(x + y) * 5 / 7);
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

