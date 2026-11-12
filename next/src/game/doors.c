/* Banked module (PAGE_DOORS): doors between the outside world and the interiors,
   locked doors, beds. Ported from fmain.c (doorlist, open_list, doorfind, door check
   in main(), xfer). */

#include <stdint.h>
#include "world.h"
#include "view.h"
#include "state.h"
#include "actors.h"
#include "banked.h"

/* door types: horizontals all have lsb set */
#define HWOOD   1
#define VWOOD   2
#define HSTONE  3
#define VSTONE  4
#define HCITY   5
#define VCITY   6
#define CRYST   7
#define SECRET  8
#define BLACK   9
#define MARBLE  10
#define LOG     11
#define HSTON2  13
#define VSTON2  14
#define STAIR   15
#define DESERT  17
#define CAVE    18
#define VLOG    18      /* same as cave */

#define DOORCOUNT 86

struct door {
    uint16_t xc1, yc1;  /* outside image coords relative to F1 */
    uint16_t xc2, yc2;  /* inside image coords relative to F9 */
    uint8_t type;       /* wood, stone */
    uint8_t secs;       /* what sectors are joined by this */
};

/* fmain.c:239 */
static const struct door doorlist[DOORCOUNT] = {
    { 0x1170,0x5060, 0x2870,0x8b60, HWOOD,1 }, /* desert fort */
    { 0x1170,0x5060, 0x2870,0x8b60, HWOOD,1 }, /* desert fort */
    { 0x1170,0x5060, 0x2870,0x8b60, HWOOD,1 }, /* desert fort */
    { 0x1170,0x5060, 0x2870,0x8b60, HWOOD,1 }, /* desert fort */
    { 0x1390,0x1b60, 0x1980,0x8c60, CAVE,  2 }, /* dragon cave */
    { 0x1770,0x6aa0, 0x2270,0x96a0, BLACK, 1 }, /* pass fort */
    { 0x1970,0x62a0, 0x1f70,0x96a0, BLACK, 1 }, /* gate fort */
    { 0x1aa0,0x4ba0, 0x13a0,0x95a0, DESERT,1 }, /* oasis #1 */
    { 0x1aa0,0x4c60, 0x13a0,0x9760, DESERT,1 }, /* oasis #4 */
    { 0x1b20,0x4b60, 0x1720,0x9560, DESERT,1 }, /* oasis #2 */
    { 0x1b80,0x4b80, 0x1580,0x9580, DESERT,1 }, /* oasis #3 */
    { 0x1b80,0x4c40, 0x1580,0x9740, DESERT,1 }, /* oasis #5 */
    { 0x1e70,0x3b60, 0x2880,0x9c60, HSTONE,1 }, /* west keep */
    { 0x2480,0x33a0, 0x2e80,0x8da0, HWOOD ,1 }, /* swamp shack */
    { 0x2960,0x8760, 0x2b00,0x92c0, STAIR ,1 }, /* stargate forwards */
    { 0x2b00,0x92c0, 0x2960,0x8780, STAIR ,2 }, /* stargate backwards */
    { 0x2c00,0x7160, 0x2af0,0x9360, BLACK ,1 }, /* doom tower */
    { 0x2f70,0x2e60, 0x3180,0x9a60, HSTONE,1 }, /* lakeside keep */
    { 0x2f70,0x63a0, 0x1c70,0x96a0, BLACK ,1 }, /* plain fort */
    { 0x3180,0x38c0, 0x2780,0x98c0, HWOOD ,1 }, /* road's end inn */
    { 0x3470,0x4b60, 0x0470,0x8ee0, STAIR ,2 }, /* tombs */
    { 0x3DE0,0x1BC0, 0x2EE0,0x93C0, CRYST ,1 }, /* crystal palace */
    { 0x3E00,0x1BC0, 0x2F00,0x93C0, CRYST ,1 }, /* crystal palace */
    { 0x4270,0x2560, 0x2e80,0x9a60, HSTONE,1 }, /* coast keepDB */
    { 0x4280,0x3bc0, 0x2980,0x98c0, HWOOD ,1 }, /* friendly inn */
    { 0x45e0,0x5380, 0x25d0,0x9680, MARBLE,1 }, /* mountain keep */
    { 0x4780,0x2fc0, 0x2580,0x98c0, HWOOD ,1 }, /* forest inn */
    { 0x4860,0x6640, 0x1c60,0x9a40, VLOG  ,1 }, /* cabin yard #7 */
    { 0x4890,0x66a0, 0x1c90,0x9aa0, LOG   ,1 }, /* cabin #7 */
    { 0x4960,0x5b40, 0x2260,0x9a40, VLOG  ,1 }, /* cabin yard #6 */
    { 0x4990,0x5ba0, 0x2290,0x9aa0, LOG   ,1 }, /* cabin #6 */
    { 0x49a0,0x3cc0, 0x0ba0,0x82c0, VWOOD ,1 }, /* village #2 */
    { 0x49d0,0x3dc0, 0x0bd0,0x84c0, VWOOD ,1 }, /* village #1.a */
    { 0x49d0,0x3e00, 0x0bd0,0x8500, VWOOD ,1 }, /* village #1.b */
    { 0x4a10,0x3c80, 0x0d10,0x8280, HWOOD ,1 }, /* village #3 */
    { 0x4a10,0x3d40, 0x0f10,0x8340, HWOOD ,1 }, /* village #5 */
    { 0x4a30,0x3dc0, 0x0e30,0x85c0, HWOOD ,1 }, /* village #7 */
    { 0x4a60,0x3e80, 0x1060,0x8580, HWOOD ,1 }, /* village #8 */
    { 0x4a70,0x3c80, 0x1370,0x8280, HWOOD ,1 }, /* village #4 */
    { 0x4a80,0x3d40, 0x1190,0x8340, HWOOD ,1 }, /* village #6 */
    { 0x4c70,0x3260, 0x2580,0x9c60, HSTONE,1 }, /* crag keep */
    { 0x4d60,0x5440, 0x1f60,0x9c40, VLOG  ,1 }, /* cabin #2 */
    { 0x4d90,0x4380, 0x3080,0x8d80, HSTON2,1 }, /* crypt */
    { 0x4d90,0x54a0, 0x1f90,0x9ca0, LOG   ,1 }, /* cabin yard #2 */
    { 0x4de0,0x6b80, 0x29d0,0x9680, MARBLE,1 }, /* river keep */
    { 0x5360,0x5840, 0x2260,0x9840, VLOG  ,1 }, /* cabin yard #3 */
    { 0x5390,0x58a0, 0x2290,0x98a0, LOG   ,1 }, /* cabin #3 */
    { 0x5460,0x4540, 0x1c60,0x9840, VLOG  ,1 }, /* cabin yard #1 */
    { 0x5470,0x6480, 0x2c80,0x8d80, HSTONE,1 }, /* elf glade */
    { 0x5490,0x45a0, 0x1c90,0x98a0, LOG   ,1 }, /* cabin #1 */
    { 0x55f0,0x52e0, 0x16e0,0x83e0, MARBLE,1 }, /* main castle */
    { 0x56c0,0x53c0, 0x1bc0,0x84c0, HSTON2,1 }, /* city #15.a */
    { 0x56c0,0x5440, 0x19c0,0x8540, HSTON2,1 }, /* city #17 */
    { 0x56f0,0x51a0, 0x19f0,0x82a0, HSTON2,1 }, /* city #10 */
    { 0x5700,0x5240, 0x1df0,0x8340, VSTON2,1 }, /* city #12 */
    { 0x5710,0x5440, 0x1c10,0x8640, HSTON2,1 }, /* city #18 */
    { 0x5730,0x5300, 0x1a50,0x8400, HSTON2,1 }, /* city #14 */
    { 0x5730,0x5380, 0x1c30,0x8480, VSTON2,1 }, /* city #15.b */
    { 0x5750,0x51a0, 0x1c60,0x82a0, HSTON2,1 }, /* city #11 */
    { 0x5750,0x5260, 0x2050,0x8360, HSTON2,1 }, /* city #13 */
    { 0x5760,0x53c0, 0x2060,0x84c0, HSTON2,1 }, /* city #16 */
    { 0x5760,0x5440, 0x1e60,0x8540, HSTON2,1 }, /* city #19 */
    { 0x5860,0x5d40, 0x1c60,0x9a40, VLOG  ,1 }, /* cabin yard #4 */
    { 0x5890,0x5da0, 0x1c90,0x9ca0, LOG   ,1 }, /* cabin #4 */
    { 0x58c0,0x2e60, 0x0ac0,0x8860, CAVE,  2 }, /* troll cave */
    { 0x5960,0x6f40, 0x2260,0x9a40, VLOG  ,1 }, /* cabin yard #9 */
    { 0x5990,0x6fa0, 0x2290,0x9ca0, LOG   ,1 }, /* cabin #9 */
    { 0x59a0,0x6760, 0x2aa0,0x8b60, STAIR ,1 }, /* unreachable castle */
    { 0x59e0,0x5880, 0x27d0,0x9680, MARBLE,1 }, /* farm keep */
    { 0x5e70,0x1a60, 0x2580,0x9a60, HSTONE,1 }, /* north keep */
    { 0x5ec0,0x2960, 0x11c0,0x8b60, CAVE  ,2 }, /* spider exit */
    { 0x6060,0x7240, 0x1960,0x9c40, VLOG  ,1 }, /* cabin yard #10 */
    { 0x6090,0x72a0, 0x1990,0x9ca0, LOG   ,1 }, /* cabin #10 */
    { 0x60f0,0x32c0, 0x25f0,0x8bc0, HSTONE,1 }, /* mammoth manor */
    { 0x64c0,0x1860, 0x03c0,0x8660, CAVE  ,2 }, /* maze cave 2 */
    { 0x6560,0x5d40, 0x1f60,0x9a40, VLOG  ,1 }, /* cabin yard #5 */
    { 0x6590,0x5da0, 0x1f90,0x98a0, LOG   ,1 }, /* cabin #5 */
    { 0x65c0,0x1a20, 0x04b0,0x8840, BLACK ,2 }, /* maze cave 1 */
    { 0x6670,0x2a60, 0x2b80,0x9a60, HSTONE,1 }, /* glade keep */
    { 0x6800,0x1b60, 0x2af0,0x9060, BLACK ,1 }, /* witch's castle */
    { 0x6b50,0x4380, 0x2850,0x8d80, HSTON2,1 }, /* light house */
    { 0x6be0,0x7c80, 0x2bd0,0x9680, MARBLE,1 }, /* lonely keep */
    { 0x6c70,0x2e60, 0x2880,0x9a60, HSTONE,1 }, /* sea keep */
    { 0x6d60,0x6840, 0x1f60,0x9a40, VLOG  ,1 }, /* cabin yard #8 */
    { 0x6d90,0x68a0, 0x1f90,0x9aa0, LOG   ,1 }, /* cabin #8 */
    { 0x6ee0,0x5280, 0x31d0,0x9680, MARBLE,1 }  /* point keep */
};

enum ky { NOKEY = 0, GOLD, GREEN, KBLUE, RED, GREY, WHITE };

struct door_open {
    uint8_t door_id;
    uint16_t map_id;    /* what number door and map */
    uint8_t new1, new2; /* new identities */
    uint8_t above;      /* 0 = none 1=above, 2=side, 3=back */
    uint8_t keytype;    /* key type needed to unlock */
};

/* fmain.c:1059 */
static const struct door_open open_list[17] = {
    { 64, 360, 123,124, 2, GREEN }, /* HSTONE */
    {120, 360, 125,126, 2, NOKEY }, /* HWOOD */
    {122, 360, 127,  0, 0, NOKEY }, /* VWOOD */
    { 64, 280, 124,125, 2, GREY }, /* HSTONE2 */
    { 77, 280, 126,  0, 0, GREY }, /* VSTONE2 */
    { 82, 480,  84, 85, 2, KBLUE }, /* CRYST */
    { 64, 480, 105,106, 2, GREEN }, /* OASIS */
    {128, 240, 154,155, 1, WHITE }, /* MARBLE */
    { 39, 680,  41, 42, 2, GOLD }, /* HGATE */
    { 25, 680,  27, 26, 3, GOLD }, /* VGATE */
    {114, 760, 116,117, 1, RED }, /* SECRET */
    {118, 760, 116,117, 1, GREY }, /* TUNNEL */
    {136, 800, 133,134,135,GOLD }, /* GOLDEN */
    {187, 800,  76, 77, 2, NOKEY }, /* HSTON3 */
    { 73, 720,  75,  0, 0, NOKEY }, /* VSTON3 */
    {165, 800,  85, 86, 4, GREEN }, /* CABINET */
    {210, 840, 208,209, 2, NOKEY }  /* BLUE */
};

/* fmain.c:1081 - open a door the hero bumped into (keytype 0 = no key) */
uint8_t doorfind_b(uint16_t x, uint16_t y, uint8_t keytype)
{
    uint8_t sec_id, j, k, l;
    uint16_t reg_id;

    if (px_to_im(x, y) == 15) goto found;
    x += 4; if (px_to_im(x, y) == 15) goto found;
    x -= 8; if (px_to_im(x, y) == 15) goto found;
    return FALSE;
found:
    if (px_to_im(x - 16, y) == 15) x -= 16;
    if (px_to_im(x - 16, y) == 15) x -= 16;
    if (px_to_im(x, y + 32) == 15) y += 32;
    x >>= 4;
    y >>= 5;
    sec_id = mapxy_get(x, y);
    reg_id = world_image_block(sec_id >> 6);
    for (j = 0; j < 17; j++) {
        if (open_list[j].map_id == reg_id && open_list[j].door_id == sec_id) {
            if (open_list[j].keytype == 0 || open_list[j].keytype == keytype) {
                mapxy_set(x, y, open_list[j].new1);
                k = open_list[j].new2;
                if (k) {
                    l = open_list[j].above;
                    if (l == 1) mapxy_set(x, y - 1, k);
                    else if (l == 3) mapxy_set(x - 1, y, k);
                    else if (l == 4) {
                        mapxy_set(x, y - 1, 87);
                        mapxy_set(x + 1, y, 86);
                        mapxy_set(x + 1, y - 1, 88);
                    }
                    else {
                        mapxy_set(x + 1, y, k);
                        if (l != 2) mapxy_set(x + 2, y, open_list[j].above);
                    }
                }
                viewstatus = 99;
                view_invalidate();
                print("It opened.");
                return TRUE;
            }
        }
    }
    if (!bumped && !keytype) print("It's locked.");
    bumped = 1;
    return FALSE;
}

/* fmain.c:2625 - move the hero to (x, y); flag: compute the region from the position */
void xfer_b(uint16_t xtest, uint16_t ytest, uint8_t flag)
{
    map_x += xtest - hero_x;
    map_y += ytest - hero_y;
    hero_x = anim_list[0].abs_x = xtest;
    hero_y = anim_list[0].abs_y = ytest;

    encounter_number = 0;
    if (flag) {
        uint16_t xr = ((map_x + 151) >> 8 >> 6) & 1;
        uint16_t yr = ((map_y + 64) >> 8 >> 5) & 7;
        new_region = xr + yr + yr;
    }
    load_all();
    gen_mini();                                 /* to set xreg and yreg */
    viewstatus = 99;
    setmood(TRUE);
    while (proxcheck(hero_x, hero_y, 0)) hero_y++;
    anim_list[0].abs_y = hero_y;
}

/* fmain.c:1871-1956 - beds inside buildings, then doors */
void door_check_b(void)
{
    uint8_t t;
    int16_t i, k, j;
    uint16_t xtest, ytest;
    const struct door *d;

    if (region_num == 8) {                      /* inside buildings */
        t = mapxy_get(hero_x >> 4, hero_y >> 5);
        if (t == 161 || t == 52 || t == 162 || t == 53) {  /* sleeping spot */
            sleepwait++;
            if (sleepwait == 30) {
                if (fatigue < 50) event(25);
                else {
                    event(26);
                    hero_y = (anim_list[0].abs_y |= 0x1f);
                    anim_list[0].state = SLEEP;
                }
            }
        }
        else sleepwait = 0;
    }

    i = 0;
    k = DOORCOUNT - 1;
    xtest = hero_x & 0xfff0;
    ytest = hero_y & 0xffe0;

    if (riding) return;
    if (region_num < 8) {
        while (k >= i) {
            j = (k + i) / 2;
            d = &doorlist[j];
            if (d->xc1 > xtest) k = j - 1;
            else if (d->xc1 + 16 < xtest) i = j + 1;
            else if (d->xc1 < xtest && (d->type & 1) == 0) i = j + 1;   /* door not horizontal */
            else if (d->yc1 > ytest) k = j - 1;
            else if (d->yc1 < ytest) i = j + 1;
            else {
                if (d->type & 1) { if (hero_y & 0x10) break; }
                else if ((hero_x & 15) > 6) break;

                if (d->type == DESERT && stuff[STATBASE] < 5) break;
                if (d->type == CAVE) { xtest = d->xc2 + 24; ytest = d->yc2 + 16; }
                else if (d->type & 1) { xtest = d->xc2 + 16; ytest = d->yc2; }
                else { xtest = d->xc2 - 1; ytest = d->yc2 + 16; }

                new_region = (d->secs == 1) ? 8 : 9;
                xfer_b(xtest, ytest, FALSE);
                find_place(2);
                fade_page(100, 100, 100, TRUE);
                break;
            }
            if (i >= DOORCOUNT || k < 0) break;
        }
    } else {
        for (j = 0; j < DOORCOUNT; j++) {
            d = &doorlist[j];
            if (d->yc2 == ytest && (d->xc2 == xtest || (d->xc2 == xtest - 16 && (d->type & 1)))) {
                if (d->type & 1) { if ((hero_y & 0x10) == 0) break; }
                else if ((hero_x & 15) < 2) break;

                if (d->type == CAVE) { xtest = d->xc1 - 4; ytest = d->yc1 + 16; }
                else if (d->type & 1) { xtest = d->xc1 + 16; ytest = d->yc1 + 34; }
                else { xtest = d->xc1 + 20; ytest = d->yc1 + 16; }

                xfer_b(xtest, ytest, TRUE);
                find_place(FALSE);
                break;
            }
        }
    }
    hero_x = anim_list[0].abs_x;
    hero_y = anim_list[0].abs_y;
}
