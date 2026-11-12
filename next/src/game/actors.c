/* Actor movement, ported from the actor loop in fmain.c main() (lines 1466-1864).
   M3 covers the WALKING / STILL / blocked paths and the terrain ("sinker") logic;
   fighting, shooting, carriers, set figures and dragons follow in M4. */

#include <stdint.h>
#include "world.h"
#include "actors.h"

struct shape anim_list[MAX_ACTORS];
uint8_t anix;
uint8_t cycle;
uint16_t hero_x, hero_y;

static uint8_t frustflag;

/* state not yet ported, kept with the original names (M4) */
static uint8_t riding, raftprox, xtype;
static uint8_t stuff30;          /* stuff[30] - crystal shard, walk through walls of type 12 */

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

/* fsubs.asm:1276 */
static const int8_t xdir[10] = { -2,0,2,3,2,0,-2,-3,0,0 };
static const int8_t ydir[10] = { -2,-3,-2,0,2,3,2,0,0,0 };

/* fsubs.asm _newx: x + xdir*speed/2 (logical shift of the 16-bit product), 15 bits */
uint16_t newx(uint16_t x, uint8_t dir, int8_t speed)
{
    if (dir > 7)
        return x;
    return (x + ((uint16_t)(xdir[dir] * speed) >> 1)) & 0x7FFF;
}

/* fsubs.asm _newy: same, but keeps bit 15 (indoor maps) */
uint16_t newy(uint16_t y, uint8_t dir, int8_t speed)
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

void actors_init_hero(uint16_t x, uint16_t y)
{
    struct shape *an = &anim_list[0];
    an->abs_x = hero_x = x;
    an->abs_y = hero_y = y;
    an->type = PHIL;
    an->race = 0;
    an->index = diroffs[5] + 1;
    an->weapon = 1;
    an->environ = 0;
    an->goal = 0;
    an->tactic = 0;
    an->state = STILL;
    an->facing = 5;
    an->vitality = 10;
    an->vel_x = an->vel_y = 0;
    anix = 1;
}

/* fmain.c:1440 - hero state from the joystick/keyboard direction */
void hero_input(uint8_t dir)
{
    if (dir < 9) {
        anim_list[0].facing = dir;
        anim_list[0].state = WALKING;
    } else {
        anim_list[0].state = STILL;
    }
}

void actors_update(void)
{
    uint8_t i, j, d, s;
    int8_t k, e, dex;
    uint16_t xtest, ytest;
    struct shape *an;

    for (i = 0; i < anix; i++) {
        an = &anim_list[i];
        k = an->environ;
        d = an->facing;
        s = an->state;
        dex = an->index;
        j = 0;

        if (s == SINK) {
            if (an->vitality > 0)
                dex = 83;
            frustflag = 0;
            goto cpx;
        } else if (s == WALKING) {
            if (k == -2) {
                /* ice: velocity based sliding (fmain.c:1583) */
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
                if (proxcheck(xtest, ytest, i) == 0)
                    goto newloc;
                k = 0;
            }
            if (i == 0 && riding == 5) e = 3;
            else if (k == -3) e = -2;               /* walk backwards */
            else if (k == -1) e = 4;
            else if (k == 2 || k > 6) e = 1;
            else e = 2;

            xtest = newx(an->abs_x, d, e);
            ytest = newy(an->abs_y, d, e);
            j = proxcheck(xtest, ytest, i);
            if (i == 0) {
                /* TODO (M4): doors - if (j == 15) doorfind(xtest, ytest, 0) */
                if (stuff30 && j == 12)
                    goto newloc;
            }
            if (j) {
                d = (d + 1) & 7;                    /* checkdev1 */
                xtest = newx(an->abs_x, d, e);
                ytest = newy(an->abs_y, d, e);
                if (proxcheck(xtest, ytest, i)) {
                    d = (d - 2) & 7;                /* checkdev2 */
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
            if (k == -2)
                goto cpx;
            if (k == -3)
                dex ^= 7;
            j = px_to_im(xtest, ytest);
            if (an->race == 2 || an->race == 4)     /* wraiths and snakes immune to water */
                j = 0;
            if (an->race == 4)
                dex = (cycle & 2) + ((d & 0xFE) << 2) + 0x24;
            if (k > 2) {
                if (j == 0 || (j == 3 && k > 5) || (j == 4 && k > 10)) {
                    if (hero_sector != 181)
                        k--;
                    goto raise;
                }
            }
            an->vel_x = (int8_t)((int16_t)(xtest - an->abs_x) * 4);
            an->vel_y = (int8_t)((int16_t)(ytest - an->abs_y) * 4);
            an->abs_x = xtest;
            an->abs_y = ytest;
            frustflag = 0;
            goto sinker;
        } else if (s == STILL) {
            dex = diroffs[d] + 1;
            if (k == -2)
                goto cpx;
            j = px_to_im(an->abs_x, an->abs_y);
            goto sinker;
        }
        goto raise;

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

sinker:
        /* j is the terrain type (fmain.c:1759) */
        if (i == 0 && raftprox) k = 0;
        else if (j == 0) k = 0;
        else if (j == 6) k = -1;
        else if (j == 7) k = -2;
        else if (j == 8) k = -3;
        else if (j == 9 && i == 0 && xtype == 52) {
            /* TODO (M4): pit fall (fallstates, luck) */
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
                    /* TODO (M4): hero_sector 181 drowns into the dungeon (xfer) */
                    an->state = STILL;
                }
                else if (k > 15)
                    an->state = SINK;
            }
        }
        if (k == 0 && s == SINK)
            an->state = STILL;
raise:
        if (new_region >= NO_REGION)
            an->environ = k;
        an->index = dex;

        if (i == 0) {
            if (region_num < 8) {
                if (an->abs_x < 300) an->abs_x = 32565;
                else if (an->abs_x > 32565) an->abs_x = 300;
                else if (an->abs_y < 300) an->abs_y = 32565;
                else if (an->abs_y > 32565) an->abs_y = 300;
            }
            hero_x = an->abs_x;
            hero_y = an->abs_y;
            map_adjust(hero_x, hero_y);
            hero_sector = world_hero_sector(hero_x, hero_y);
        }
    }
}
