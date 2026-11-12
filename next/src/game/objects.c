/* Banked module (PAGE_OBJECTS): objects lying around in the world and set figures
   (fmain2.c:967-1302). The lists stay resident (they change and will be saved). */

#include <stdint.h>
#include "world.h"
#include "state.h"
#include "actors.h"
#include "shapes.h"
#include "banked.h"
#include "../platform/files.h"

enum obytes {
    QUIVER = 11,
    MONEY = 13, URN, CHEST, SACKS, G_RING, B_STONE, G_JEWEL,
    SCRAP, C_ORB, VIAL, B_TOTEM, J_SKULL,
    GOLD_KEY = 25, GREY_KEY = 26,
    FOOTSTOOL = 31,
    TURTLE = 102,
    BLUE_KEY = 114,
    M_WAND = 145, MEAL, ROSE, FRUIT, STATUE, BOOK, SHELL,
    GREEN_KEY = 153, WHITE_KEY = 154, RED_KEY = 242
};

static const uint8_t rand_treasure[16] = {
    SACKS, SACKS, SACKS, SACKS,
    CHEST, MONEY, GOLD_KEY, QUIVER,
    GREY_KEY, GREY_KEY, GREY_KEY, RED_KEY,
    B_TOTEM, VIAL, WHITE_KEY, CHEST
};

#define TENBLANKS {0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0}

/* 0=nonexist, 1=ground, 2=inventory, 3=setfig, 4=dead setfig, 5=hidden object, 6=cabinet item */
static struct object ob_listg[] = {    /* global objects */
    {     0,    0,0,0},            /* special item (for 'give') */
    {     0,    0,28,0},        /* dead brother 1 - to be filled in later */
    {     0,    0,28,0},        /* dead brother 2 - to be filled in later */
    { 19316,15747,11,0},        /* ghost brother 1 - to be filled in later */
    { 18196,15735,11,0},        /* ghost brother 2 - to be filled in later */
    { 12439,36202,10,3},        /* spectre - can be disabled */
    { 11092,38526,STATUE,1},    /* gold statues (6 = seahold) */
    { 25737,10662,STATUE,1},    /* (7 = ogre den) */
    {  2910,39023,STATUE,1},    /* (8 = octal room) */
    { 12025,37639,STATUE,0},    /* (9 = sorceress) */
    {  6700,33766,STATUE,0},    /* (10 = priest) */
};
static struct object ob_list0[] = {    /* snow land objects */
    { 3340,6735,12,3},        /* ranger west */
    { 9678,7035,12,3},        /* ranger east */
    { 4981,6306,12,3},        /* ranger north */
    TENBLANKS
};
static struct object ob_list1[] = {    /* maze forest (north) objects */
/*    {23297,5797,TURTLE,1}, */
    {23087,5667,TURTLE,1},
    TENBLANKS
};
static struct object ob_list2[] = {    /* swamp land objects */
    {13668,15000,0,3},        /* wizard */
    {10627,13154,0,3},        /* wizard */
    { 4981,10056,12,3},        /* ranger */
    {13950,11087,SACKS,1},
    { 10344,36171,SHELL,1},    /* shell */
    TENBLANKS
};
static struct object ob_list3[] = {    /* maze forest (south) and manor objects (and tambry) */
    {19298,16128,CHEST,1},
    {18310,15969,13,3}, /* beggar */
    {20033,14401,0,3},    /* wizard */
    {24794,13102,13,3}, /* beggar */
    { 21626,15446,B_STONE,1},    /* 3 items at stone ring */

    { 21616,15456,MONEY,1},
    { 21636,15456,G_RING,1},
    { 20117,14222,G_JEWEL,1},
    { 24185,9840,SACKS,1},
    { 25769,10617,MONEY,1},

    { 25678,10703,B_STONE,1},
    { 17177,10599,SCRAP,1},
    TENBLANKS
};
static struct object ob_list4[] = {    /* desert objects */
    { 0,0,0,0},    /* dummy */
    { 0,0,0,0}, /* dummy */
    {6817,19693,13,3}, /* beggar - must be 3rd object */
    TENBLANKS
};
static struct object ob_list5[] = {    /* farm and city objects */
    {22184,21156,13,3}, /* beggar */
    {18734,17595,G_RING,1},
    {21294,22648,CHEST,1},
    {22956,19955,0,3},    /* wizard */
    {28342,22613,0,3},        /* wizard */
    TENBLANKS
};
static struct object ob_list6[] = {    /* lava plain objects */
    {24794,13102,13,3}, /* DUMMY OBJECT */
    TENBLANKS
};
static struct object ob_list7[] = {    /* southern mountain land objects */
    {23297,5797,TURTLE,1},    /* dummy object */
    TENBLANKS
};
static struct object ob_list8[] = {            /* interiors of buildings */
    { 6700,33756,1,3},    /* priest in chapel */
    { 5491,33780,5,3},    /* king on throne - never mask */
    { 5592,33764,6,3},    /* noble */
    { 5514,33668,2,3},    /* guard */
    { 5574,33668,2,3},    /* guard */
    { 8878,38995,0,3},    /* wizard */
    { 7776,34084,0,3},    /* wizard */
    { 5514,33881,3,3},    /* guard */
    { 5574,33881,3,3},    /* guard */
    { 10853,35656,4,3},    /* princess */
    { 12037,37614,7,3},    /* sorceress */

    { 11013,36804,9,3},    /* witch */
    { 9631,38953,8,3},    /* bartender */
    { 10191,38953,8,3},    /* bartender */
    { 10649,38953,8,3},    /* bartender */
    {  2966,33964,8,3},    /* bartender */
    { 9532,40002,FOOTSTOOL,1},
    { 6747,33751,FOOTSTOOL,1},
    { 11410,36169,27+128,1},    /* sunstone */
    { 9550,39964,B_TOTEM,1}, /* cabinet item */
    { 9552,39964,B_TOTEM,1},

    { 9682,39964,B_TOTEM,1}, /* cabinet item */
    { 9684,39964,B_TOTEM,1},
    { 9532,40119,B_TOTEM,1}, /* on table */
    { 9575,39459,URN,1},
    { 9590,39459,URN,1},
    { 9605,39459,URN,1},
    { 9680,39453,VIAL,1},
    { 9682,39453,VIAL,1},
    { 9784,39453,VIAL,1},
    { 9668,39554,CHEST,1},

    { 11090,39462,MONEY,1},
    { 11108,39458,B_TOTEM,1},
    { 11118,39459,B_TOTEM,1},
    { 11128,39459,B_TOTEM,1},
    { 11138,39458,B_TOTEM,1},
    { 11148,39459,B_TOTEM,1},
    { 11158,39459,B_TOTEM,1},
    { 11855,36206,FOOTSTOOL,1},
    { 11909,36198,CHEST,1},
    { 11918,36246,B_TOTEM,1},    /* cabinet items? */

    { 11928,36246,B_TOTEM,1},
    { 11938,36246,B_TOTEM,1},
    { 12212,38481,CHEST,1},
    { 11652,38481,RED_KEY,1},
    { 10427,39977,FOOTSTOOL,1},
    { 10323,40071,URN,1},
    { 10059,38472,SACKS,1},
    { 10344,36171,SHELL,1},    /* shell */
    { 11936,36207,SCRAP,1},        /* spectre note */
    {  9674,35687,URN,1},
    {  5473,38699,ROSE,1},
    {  7185,34342,FRUIT,1},
     {  7190,34342,FRUIT,1},
    {  7195,34342,FRUIT,1},
    {  7185,34347,FRUIT,1},
     {  7190,34347,FRUIT,1},
    {  7195,34347,FRUIT,1},
    {  6593,34085,FRUIT,1},
    {  6598,34085,FRUIT,1},
    {  6593,34090,FRUIT,1},
    {  6598,34090,FRUIT,1},

    /* 'look' items */
    { 3872,33546,GOLD_KEY,5 },
    { 3887,33510,B_TOTEM,5 },
    { 4495,33510,VIAL,5 },
    { 3327,33383,J_SKULL,5 },
    { 4221,34119,QUIVER,5 },
    { 7610,33604,VIAL,5 },
    { 7616,33522,MONEY,5 },
    { 9570,35768,B_STONE,5 },
    { 9668,35769,QUIVER,5 },
    { 9553,38951,G_RING,5 },
    { 10062,39005,J_SKULL,5 },
    { 10577,38951,VIAL,5 },
    { 11062,39514,MONEY,5 },
    { 8845,39494,WHITE_KEY,5 },
    { 6542,39494,G_JEWEL,5 },
    { 7313,38992,RED_KEY,5 }
};
static struct object ob_list9[] = {            /* underground areas */
    { 7540,38528,M_WAND,1},
    { 9624,36559,M_WAND,1},
    { 9624,37459,M_WAND,1},
    { 8337,36719,M_WAND,1},
    { 8154,34890,CHEST,1},
    { 7826,35741,CHEST,1},
    { 3460,37260,0,3},        /* wizard */
    { 8485,35725,MONEY,1},
    { 3723,39340,(128+10),1},    /* king's bone */
};

static struct object *const ob_table[10] = {
    ob_list0, ob_list1, ob_list2, ob_list3, ob_list4,
    ob_list5, ob_list6, ob_list7, ob_list8, ob_list9 };

static int16_t mapobs[10] = { 3, 1, 5, 12, 3, 5, 1, 1, 61 + 16, 9 };   /* count */
static uint8_t dstobs[10] = { 0, 0, 0, 0, 0, 0, 0, 0, 1, 1 };         /* distributed */
#define GLBOBS 11

static uint8_t j1;

/* fmain2.c:1218 */
static void set_objects(struct object *list, int16_t length, uint8_t f)
{
    int16_t xstart, ystart;
    uint8_t i, k, cf, id = 0;
    struct shape *an;

    turtle_eggs = witchflag = FALSE;
    if (dstobs[region_num] == 0 && new_region >= 10) {
        struct object *l2 = ob_table[region_num];
        for (i = 0; i < 10; i++) {
            do {
                xstart = bitrand(0x3fff) + ((region_num & 1) * 0x4000);
                ystart = bitrand(0x1fff) + ((region_num & 6) * 0x1000);
            } while (px_to_im(xstart, ystart));
            k = mapobs[region_num]++;
            l2[k].xc = xstart;
            l2[k].yc = ystart;
            l2[k].ob_id = rand_treasure[bitrand(15)];
            l2[k].ob_stat = 1;
        }
        dstobs[region_num] = 1;                 /* mark this region as distributed */
    }
    i = 0;
    while (length > 0) {
        length--;
        if (anix2 >= 20) return;
        xstart = (int16_t)(list->xc - map_x - 8);
        ystart = (int16_t)(list->yc - map_y - 8);
        if (list->ob_stat == 3 || list->ob_stat == 4) {
            if (xstart > -100 && xstart < 400 && ystart > -100 && ystart < 300) {
                id = list->ob_id;
                cf = setfig_cfile(id);
                if (cf != set_file) {
                    set_file = cf;
                    read_shapes(cf);
                }
                if (list->ob_id == 9 && list->ob_stat == 3) witchflag = TRUE;
            }
            else goto loopend;
        }
        if (list->ob_stat == 2 || list->ob_stat == 0) goto loopend;
        if (xstart > -20 && xstart < 340 && ystart > -20 && ystart < 190) {
            if (list->ob_stat == 3 || list->ob_stat == 4) {
                an = &anim_list[j1++];
                if (an->abs_x == list->xc && an->abs_y == list->yc) {
                    if (an->state == DEAD) list->ob_stat = 4;
                    goto loopend;
                }
                an->type = SETFIG;
                an->index = setfig_image_base(id);
                an->weapon = 0;
                an->vitality = 2 + id + id;
                an->goal = i;
                an->environ = 0;
                an->state = (list->ob_stat == 4) ? DEAD : STILL;
                an->race = id + 0x80;
                ystart -= 18;
                if (j1 > anix2) anix2 = j1;
            } else {
                if (list->ob_id == TURTLE) turtle_eggs = anix2;
                an = &anim_list[anix2++];
                an->type = OBJECTS;
                an->index = list->ob_id;
                an->weapon = 0;
                an->vitality = i + f;
                an->state = STILL;
                an->environ = 0;
                if (list->ob_stat == 6) an->race = 2;
                else if (list->ob_stat == 5) an->race = 0;
                else an->race = 1;
            }
            an->abs_x = list->xc;
            an->abs_y = list->yc;
            an->rel_x = xstart;
            an->rel_y = ystart;
        }
loopend:
        list++;
        i++;
    }
}

/* fmain2.c:1184 */
void do_objects_b(void)
{
    j1 = 2;
    set_objects(ob_listg, GLBOBS, 0x80);
    set_objects(ob_table[region_num], mapobs[region_num], 0);
    if (j1 > 3) anix = j1;
}

/* fmain2.c:1191 */
void leave_item_b(uint8_t i, uint8_t object)
{
    ob_listg[0].xc = anim_list[i].abs_x;
    ob_listg[0].yc = anim_list[i].abs_y + 10;
    ob_listg[0].ob_id = object;
    ob_listg[0].ob_stat = 1;
}

/* fmain2.c:1200 - flag 1=show, 2=take */
void change_object_b(uint8_t id, uint8_t flag)
{
    struct shape *an = &anim_list[id];
    struct object *ob;
    uint8_t n = an->vitality & 0x7f;
    if (an->vitality & 0x80) ob = &ob_listg[n];
    else ob = &ob_table[region_num][n];
    if (ob->ob_id == CHEST) ob->ob_id = 0x1d; else ob->ob_stat = flag;
}

/* generic field access for other modules: field 0 xc, 1 yc, 2 ob_id, 3 ob_stat */
uint8_t ob_field_b(uint8_t list, uint8_t idx, uint8_t field, uint8_t write, uint16_t value)
{
    struct object *ob = (list == OBLIST_GLOBAL ? ob_listg : ob_table[list]) + idx;
    if (write) {
        switch (field) {
        case 0: ob->xc = value; break;
        case 1: ob->yc = value; break;
        case 2: ob->ob_id = (uint8_t)value; break;
        default: ob->ob_stat = (uint8_t)value; break;
        }
        return 0;
    }
    return field == 2 ? ob->ob_id : ob->ob_stat;
}

/* savegame: all object lists, counts and distribution flags (they are resident data) */
uint8_t objects_io_b(uint8_t h, uint8_t save)
{
    uint8_t i, ok = 1;
    uint16_t (*io)(uint8_t, void *, uint16_t) = save ? (uint16_t (*)(uint8_t, void *, uint16_t))file_write : file_read;
    ok &= io(h, ob_listg, sizeof(ob_listg)) == sizeof(ob_listg);
    ok &= io(h, mapobs, sizeof(mapobs)) == sizeof(mapobs);
    ok &= io(h, dstobs, sizeof(dstobs)) == sizeof(dstobs);
    for (i = 0; i < 10; i++) {
        uint16_t n = (uint16_t)mapobs[i] * sizeof(struct object);
        ok &= io(h, ob_table[i], n) == n;
    }
    return ok;
}
