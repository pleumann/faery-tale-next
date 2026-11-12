/* Banked module (PAGE_MENU): the option menus of the status panel and what they do -
   fmain.c menus/letter_list/print_options/propt/do_option/gomenu/set_options,
   fmain2.c nearest_fig/eat. */

#include <stdint.h>
#include "world.h"
#include "state.h"
#include "actors.h"
#include "banked.h"
#include "ui.h"
#include "menus.h"
#include "audio.h"
#include "intro.h"
#include "view.h"

enum cmodes { ITEMS = 0, MAGIC, TALK, BUY, GAME, SAVEX, KEYS, GIVE, USE, FILE_ };

static const char label1[] = "ItemsMagicTalk Buy  Game ";
static const char label2[] = "List Take Look Use  Give ";
static const char label3[] = "Yell Say  Ask  ";
static const char label4[] = "PauseMusicSoundQuit Load ";
static const char label5[] = "Food ArrowVial Mace SwordBow  Totem";
static const char label6[] = "StoneJewelVial Orb  TotemRing Skull";
static const char label7[] = "Dirk Mace SwordBow  Wand LassoShellKey  Sun  Book ";
static const char label8[] = "Save Exit ";
static const char label9[] = "Gold GreenBlue Red  Grey White";
static const char labelA[] = "Gold Book Writ Bone ";
static const char labelB[] = "  A    B    C    D    E    F    G    H  ";

static const uint8_t keycolors[6] = { 8, 6, 4, 2, 14, 1 };

/* bit0 = selected, bit1 = displayed, else = type:
   4 = toggle, 8 = immediate, 12 = radio buttons, 0 = not changeable */
struct menu {
    const char *label_list;
    uint8_t num, color;
    uint8_t enabled[12];
};

static struct menu menus[10] = {
    { label2, 10, 6, { 3,2,2,2,2,10,10,10,10,10,0,0 } },          /* items */
    { label6, 12, 5, { 2,3,2,2,2,8,8,8,8,8,8,8 } },               /* magic */
    { label3,  8, 9, { 2,2,3,2,2,10,10,10,0,0,0,0 } },            /* talk */
    { label5, 12,10, { 2,2,2,3,2,10,10,10,10,10,10,10 } },        /* buy */
    { label4, 10, 2, { 2,2,2,2,3,6,7,7,10,10,0,0 } },             /* game */
    { label8,  7, 0, { 2,2,2,2,2,10,10,0,0,0,0,0 } },             /* save/exit */
    { label9, 11, 8, { 2,2,2,2,2,10,10,10,10,10,10,0 } },         /* keys */
    { labelA,  9,10, { 2,2,2,2,2,10,0,0,0,0,0,0 } },              /* give */
    { label7, 10, 8, { 10,10,10,10,10,10,10,10,10,0,10,10 } },    /* use */
    { labelB, 10, 5, { 10,10,10,10,10,10,10,10,0,0,0,0 } },       /* files */
};

struct letters { uint8_t letter, menu, choice; };

static const struct letters letter_list[] = {
    {'I',ITEMS,5}, {'T',ITEMS,6}, {'?',ITEMS,7}, {'U',ITEMS,8}, {'G',ITEMS,9},
    {'Y',TALK,5}, {'S',TALK,6}, {'A',TALK,7},
    {' ',GAME,5}, {'M',GAME,6}, {'F',GAME,7}, {'Q',GAME,8}, {'L',GAME,9},
    {'O',BUY,5}, {'R',BUY,6}, {'8',BUY,7}, {'C',BUY,8}, {'W',BUY,9}, {'B',BUY,10}, {'E',BUY,11},
    {'V',SAVEX,5}, {'X',SAVEX,6},
    {10,MAGIC,5}, {11,MAGIC,6}, {12,MAGIC,7}, {13,MAGIC,8}, {14,MAGIC,9}, {15,MAGIC,10}, {16,MAGIC,11},
    {'1',USE,0}, {'2',USE,1}, {'3',USE,2}, {'4',USE,3}, {'5',USE,4}, {'6',USE,5}, {'7',USE,6}, {'K',USE,7},
};
#define LMENUS (sizeof(letter_list) / sizeof(letter_list[0]))

/* fmain.c:380 inv_list: image, xoff, yoff, ydelta, img_off, img_height, maxshown, name */
const struct inv_item inv_list[36] = {
    {  12, 10, 0, 0, 0, 8, 1, "Dirk" },
    {   9, 10,10, 0, 0, 8, 1, "Mace" },
    {   8, 10,20, 0, 0, 8, 1, "Sword" },
    {  10, 10,30, 0, 0, 8, 1, "Bow" },
    {  17, 10,40, 0, 8, 8, 1, "Magic Wand" },
    {  27, 10,50, 0, 0, 8, 1, "Golden Lasso" },
    {  23, 10,60, 0, 8, 8, 1, "Sea Shell" },
    {  27, 10,70, 0, 8, 8, 1, "Sun Stone" },
    {   3, 30, 0, 3, 7, 1,45, "Arrows" },
    {  18, 50, 0, 9, 0, 8,15, "Blue Stone" },
    {  19, 65, 0, 6, 0, 5,23, "Green Jewel" },
    {  22, 80, 0, 8, 0, 7,17, "Glass Vial" },
    {  21, 95, 0, 7, 0, 6,20, "Crystal Orb" },
    {  23,110, 0,10, 0, 9,14, "Bird Totem" },
    {  17,125, 0, 6, 0, 5,23, "Gold Ring" },
    {  24,140, 0,10, 0, 9,14, "Jade Skull" },
    {  25,160, 0, 5, 0, 5,25, "Gold Key" },
    {  25,172, 0, 5, 8, 5,25, "Green Key" },
    { 114,184, 0, 5, 0, 5,25, "Blue Key" },
    { 114,196, 0, 5, 8, 5,25, "Red Key" },
    {  26,208, 0, 5, 0, 5,25, "Grey Key" },
    {  26,220, 0, 5, 8, 5,25, "White Key" },
    {  11,  0, 80,0, 8, 8, 1, "Talisman" },
    {  19,  0, 90,0, 8, 8, 1, "Rose" },
    {  20,  0,100,0, 8, 8, 1, "Fruit" },
    {  21,232, 0,10, 8, 8, 5, "Gold Statue" },
    {  22,  0,110,0, 8, 8, 1, "Book" },
    {   8, 14, 80,0, 8, 8, 1, "Herb" },
    {   9, 14, 90,0, 8, 8, 1, "Writ" },
    {  10, 14,100,0, 8, 8, 1, "Bone" },
    {  12, 14,110,0, 8, 8, 1, "Shard" },
    {   0,  0, 0, 0, 0, 0, 2, "2 Gold Pieces" },
    {   0,  0, 0, 0, 0, 0, 5, "5 Gold Pieces" },
    {   0,  0, 0, 0, 0, 0,10, "10 Gold Pieces" },
    {   0,  0, 0, 0, 0, 0,100,"100 Gold Pieces" },
    {   0,  0, 0, 0, 0, 0, 0, "quiver of arrows" }
};

/* fmain2.c:850, 852 */
static const int8_t jtrans[14] = { 0,3, 8,10, 11,15, 1,30, 2,45, 3,75, 13,20 };
static const int8_t treasure_probs[40] = {
     0, 0, 0, 0, 0, 0, 0, 0,    /* no treasure */
     9,11,13,31,31,17,17,32,    /* stone,vial,totem,gold,keys */
    12,14,20,20,20,31,33,31,    /* keys,skull,gold,nothing */
    10,10,16,16,11,17,18,19,    /* magic and keys */
    15,21,0,0,0,0,0,0           /* jade skull and white key */
};
/* encounter_chart[].treasure (fmain.c:52) */
static const uint8_t enc_treasure[11] = { 2,1,4,3,0,0,0,0,0,0,0 };

/* fmain2.c:979 - object id -> inventory index */
static const uint8_t itrans[] = {
    11,35, 18,9, 19,10, 22,11, 21,12, 23,13, 17,14, 24,15,
    145,4, 27,5, 8,2, 9,1, 12,0, 10,3, 147,23, 148,24, 149,25,
    150,26, 151,6, 155,7, 136,27, 137,28, 138,29, 139,22, 140,30,
    25,16, 153,17, 114,18, 242,19, 26,20, 154,21,
    0,0 };

/* fmain.c:374 */
static const uint8_t stone_list[22] = {
    54,43, 71,77, 78,102, 66,121, 12,85, 79,40, 107,38, 73,21, 12,26, 26,53, 84,60 };

uint8_t cmode;
static int8_t real_options[12];
static uint8_t hit, hitgo, svflag;

static uint8_t stuff_flag(uint8_t i) { return stuff[i] ? 10 : 8; }

/* fmain.c:3113 - Items/List: the inventory as a picture on the playfield */
void show_inventory_b(void)
{
    uint8_t j, i, num, x, y;
    view_clear();
    for (j = 0; j < GOLDBASE; j++) {
        num = stuff[j];
        if (num > inv_list[j].maxshown) num = inv_list[j].maxshown;
        x = inv_list[j].xoff + 20;
        y = inv_list[j].yoff;
        for (i = 0; i < num; i++) {
            view_blit_object(inv_list[j].image_number, inv_list[j].img_off,
                             inv_list[j].img_height, x, y);
            y += inv_list[j].ydelta;
        }
    }
}

/* fmain.c:3527 */
void set_options_b(void)
{
    uint8_t i, j;
    for (i = 0; i < 7; i++) {
        menus[MAGIC].enabled[i + 5] = stuff_flag(i + 9);
        menus[USE].enabled[i] = stuff_flag(i);
    }
    j = 8;
    for (i = 0; i < 6; i++)
        if ((menus[KEYS].enabled[i + 5] = stuff_flag(i + 16)) == 10) j = 10;
    menus[USE].enabled[7] = j;
    menus[USE].enabled[8] = stuff_flag(7);          /* sunstone */
    menus[GIVE].enabled[5] = wealth > 2 ? 10 : 8;   /* gold */
    menus[GIVE].enabled[6] = 8;                     /* book */
    menus[GIVE].enabled[7] = stuff_flag(28);        /* writ */
    menus[GIVE].enabled[8] = stuff_flag(29);        /* bone */
}

/* fmain.c:3070 */
static void propt(uint8_t j, uint8_t pena)
{
    int8_t k = real_options[j];
    uint8_t penb;
    const char *label;

    if (cmode == USE) penb = 14;
    else if (cmode == FILE_) penb = 13;
    else if (k < 5) penb = 4;
    else if (cmode == KEYS) penb = keycolors[k - 5];
    else if (cmode == SAVEX) penb = k;
    else penb = menus[cmode].color;

    k = real_options[j] * 5;
    if (cmode >= USE) label = menus[cmode].label_list + k;
    else if (k < 25) label = label1 + k;
    else label = menus[cmode].label_list + (k - 25);
    ui_menu_text(j, label, 5, pena, penb);
}

/* fmain.c:3048 */
void print_options_b(void)
{
    uint8_t i, j = 0, x;
    ui_menu_reset();
    for (i = 0; i < menus[cmode].num; i++) {
        x = menus[cmode].enabled[i];
        if ((x & 2) == 0) continue;
        real_options[j] = i;
        propt(j, x & 1);
        j++;
        if (j > 11) break;
    }
    for (; j < 12; j++) {
        ui_menu_text(j, "", 0, 0, 0);
        real_options[j] = -1;
    }
}

/* fmain.c:3520 */
static void gomenu(uint8_t mode)
{
    if (menus[GAME].enabled[5] & 1) return;
    cmode = mode;
    print_options_b();
}

/* fmain2.c:297 */
static int16_t nearest_fig(uint8_t constraint, int16_t dist)
{
    uint8_t i;
    int16_t d;
    nearest = 0;
    for (i = 1; i < anix2; i++) {
        if (anim_list[i].type == OBJECTS && (constraint || anim_list[i].index == 0x1d))
            continue;
        d = calc_dist(i, 0);
        if (d < dist) { nearest = i; dist = d; }
    }
    return dist;
}

/* fmain2.c:1704 */
static void eat(int16_t amt)
{
    hunger -= amt;
    if (hunger < 0) { hunger = 0; event(13); }
    else print("Yum!");
}

/* fmain.c:3102 */
static void do_option(uint8_t h)
{
    uint16_t x, y;
    uint8_t i, j, k;
    struct shape *an;

    hit = h;
    switch (cmode) {
    case ITEMS:
        if (hit == 8) gomenu(USE);
        else if (hit == 5) {
            show_inventory_b();
            viewstatus = 4;
            prq(5);
        }
        else if (hit == 6) {                        /* find nearest object */
            prq(7);
            nearest_fig(0, 30);
            stuff[ARROWBASE] = 0;
            if (nearest) {
                if (anim_list[nearest].type == OBJECTS) {
                    j = anim_list[nearest].index & 0xff;
                    x = anim_list[nearest].vitality & 0x7f;
                    if (j == 0x0d) {
                        announce_treasure("50 gold pieces");
                        wealth += 50;
                    }
                    else if (j == 0x14) {           /* scrap of paper */
                        event(17);
                        if (region_num > 7) event(19); else event(18);
                        goto pickup;
                    }
                    else if (j == 148) {
                        if (hunger < 15) { stuff[24]++; event(36); }
                        else eat(30);
                        goto pickup;
                    }
                    else if (j == 102) break;       /* don't take turtle eggs */
                    else if (j == 28) {
                        announce_treasure("his brother's bones.");
                        ob_stat_set(OBLIST_GLOBAL, 3, 0);
                        ob_stat_set(OBLIST_GLOBAL, 4, 0);
                        for (k = 0; k < GOLDBASE; k++) {
                            if (x == 1) stuff[k] += julstuff[k];
                            else stuff[k] += philstuff[k];
                        }
                    }
                    else {                          /* containers */
                        if (j == 0x0f) announce_container("a chest");
                        else if (j == 0x0e) announce_container("a brass urn");
                        else if (j == 0x10) announce_container("some sacks");
                        else if (j == 0x1d) break;  /* empty chest */
                        else {
                            if (j == 31) break;
                            for (k = 0; itrans[k]; k += 2) {
                                if (j == itrans[k]) {
                                    i = itrans[k + 1];
                                    stuff[i]++;
                                    announce_treasure("a ");
                                    print_cont(inv_list[i].name);
                                    print_cont(".");
                                    goto pickup;
                                }
                            }
                            break;
                        }
                        k = rand4();
                        if (k == 0) print("nothing.");
                        else if (k == 1) {
                            i = rand8() + 8;
                            if (i == 8) i = ARROWBASE;
                            print("a ");
                            print_cont(inv_list[i].name); print_cont(".");
                            stuff[i]++;
                        }
                        else if (k == 2) {
                            i = rand8() + 8;
                            if (i == 8) { i = GOLDBASE + 3; wealth += 100; }
                            else print_cont(" a");
                            print(inv_list[i].name);
                            print_cont(" and a ");
                            do { k = rand8() + 8; } while (k == i);
                            if (k == 8) k = ARROWBASE;
                            print_cont(inv_list[k].name);
                            if (i < GOLDBASE) stuff[i]++;
                            stuff[k]++;
                        }
                        else {
                            i = rand8() + 8;
                            if (i == 8) {
                                print("3 keys.");
                                for (k = 0; k < 3; k++) {
                                    i = rand8() + KEYBASE;
                                    if (i == 22) i = 16;
                                    if (i == 23) i = 20;
                                    stuff[i]++;
                                }
                                goto pickup;
                            }
                            print("3 ");
                            print_cont(inv_list[i].name); print_cont("s.");
                            stuff[i] += 3;
                        }
                    }
pickup:
                    change_object(nearest, 2);
                    stuff[8] += stuff[ARROWBASE] * 10;
                    if (stuff[22]) {
                        quitflag = TRUE; viewstatus = 2;
                        win();
                    }
                }
                else if (anim_list[nearest].weapon < 0) ;
                else if (anim_list[nearest].vitality == 0 || freeze_timer) {
                    extract("% searched the body and found");
                    print("");
                    i = anim_list[nearest].weapon; if (i > 5) i = 0;
                    if (i) {
                        print_cont("a ");
                        print_cont(inv_list[i - 1].name);
                        stuff[i - 1]++;             /* weapon */
                        if (i > anim_list[0].weapon) anim_list[0].weapon = i;
                        if (i == 4) {
                            print_cont(" and ");
                            j = rand8() + 2; prdec(j, 1);
                            print_cont(" Arrows.");
                            stuff[8] += j;
                            anim_list[nearest].weapon = -1;
                            break;
                        }
                    }
                    anim_list[nearest].weapon = -1;
                    j = anim_list[nearest].race;
                    if (j & 0x80) j = 0;            /* setfigs have no treasure */
                    else j = treasure_probs[(enc_treasure[j] * 8) + rand8()];
                    if (j) {
                        if (i) print_cont(" and ");
                        if (j < GOLDBASE) print_cont("a ");
                        print_cont(inv_list[j].name);
                        if (j >= GOLDBASE) wealth += inv_list[j].maxshown;
                        else stuff[j]++;
                    }
                    else if (!i) print_cont("nothing");
                    print_cont(".");
                }
                else event(35);
            }
            else prq(10);
        }
        else if (hit == 7) {                        /* Look */
            uint8_t flag = 0;
            for (i = 0; i < anix2; i++) {
                an = &anim_list[i];
                if (an->type == OBJECTS && an->race == 0 && calc_dist(i, 0) < 40) {
                    change_object(i, 1);
                    flag = 1;
                }
            }
            if (flag) event(38); else event(20);
        }
        else if (hit == 9) gomenu(GIVE);
        break;
    case MAGIC:
        if (hit < 5 || stuff[4 + hit] == 0) { event(21); break; }
        if (extn->v3 == 9) { speak(59); break; }
        switch (hit) {
        case 6: light_timer += 760; break;
        case 8: secret_timer += 360; break;
        case 10: if (riding > 1) return; freeze_timer += 100; break;
        case 9:
            if (region_num > 7) return;
            /* TODO (M7): the magic map (bigdraw) */
            print("(map not yet available)");
            break;
        case 5:
            if (hero_sector == 144) {
                if ((hero_x & 255) / 85 == 1 && (hero_y & 255) / 64 == 1) {
                    x = hero_x >> 8; y = hero_y >> 8;
                    for (i = 0; i < 11; i++) {
                        if (stone_list[i + i] == x && stone_list[i + i + 1] == y) {
                            i += anim_list[0].facing + 1;
                            if (i > 10) i -= 11;
                            x = (stone_list[i + i] << 8) + (hero_x & 255);
                            y = (stone_list[i + i + 1] << 8) + (hero_y & 255);
                            xfer(x, y, TRUE);
                            if (riding) {
                                anim_list[wcarry].abs_x = anim_list[0].abs_x;
                                anim_list[wcarry].abs_y = anim_list[0].abs_y;
                            }
                            break;
                        }
                    }
                } else return;
            }
            else return;                            /* didn't work: keep the stone */
            /* fall through - as in the original */
        case 7:                                     /* HEAL */
            anim_list[0].vitality += rand8() + 4;
            if (anim_list[0].vitality > 15 + brave / 4)
                anim_list[0].vitality = 15 + brave / 4;
            else print("That feels a lot better!");
            prq(4);
            break;
        case 11:
            an = &anim_list[1];
            for (i = 1; i < anix; i++, an++) {
                if (an->vitality && an->type == ENEMY && an->race < 7) {
                    an->vitality = 0; checkdead(i, 0); brave--;
                }
            }
            if (battleflag) event(34);
            break;
        }
        if (!--stuff[4 + hit]) set_options_b();
        break;
    case TALK: {
        int16_t d;
        if (hit == 5) d = nearest_fig(1, 100); else d = nearest_fig(1, 50);
        if (nearest == 0) break;
        an = &anim_list[nearest];
        if (an->state == DEAD) break;
        if (an->type == SETFIG) {
            if (hit == 5 && d < 35) { speak(8); break; }
            k = an->race & 0x7f;
            if (setfig_can_talk(k)) { an->state = TALKING; an->tactic = 15; }
            switch (k) {
            case 0: if (kind < 10) speak(35); else speak(27 + an->goal); break;      /* wizard */
            case 1:                                                                 /* priest */
                if (stuff[28]) {
                    if (ob_stat_get(OBLIST_GLOBAL, 10) == 0) { speak(39); ob_stat_set(OBLIST_GLOBAL, 10, 1); }
                    else speak(19);
                }
                else if (kind < 10) speak(40);
                else {
                    speak(36 + (daynight % 3));
                    anim_list[0].vitality = 15 + brave / 4;
                    prq(4);
                }
                break;
            case 2:
            case 3: speak(15); break;                                               /* guard */
            case 4: if (ob_stat_get(8, 9)) speak(16); break;                        /* princess */
            case 5: if (ob_stat_get(8, 9)) speak(17); break;                        /* king */
            case 6: speak(20); break;                                               /* noble */
            case 7:                                                                 /* sorceress */
                if (ob_stat_get(OBLIST_GLOBAL, 9)) { if (luck < (int16_t)rand64()) luck += 5; }
                else { speak(45); ob_stat_set(OBLIST_GLOBAL, 9, 1); }
                prq(7);
                break;
            case 8: if (fatigue < 5) speak(13); else if (dayperiod > 7) speak(12); else speak(14); break;
            case 9: speak(46); break;                                               /* witch */
            case 10: speak(47); break;                                              /* spectre */
            case 11: speak(49); break;                                              /* ghost */
            case 12: if (region_num == 2) speak(22); else speak(53 + an->goal); break;  /* ranger */
            case 13: speak(23); break;                                              /* beggar */
            }
        }
        else if (an->type == CARRIER && active_carrier == 5) {
            if (stuff[6]) speak(57);
            else { stuff[6] = 1; speak(56); }
        }
        else if (an->type == ENEMY) speak(an->race);
        break;
    }
    case BUY:
        if ((nearest = nearest_person) == 0) break;
        if (anim_list[nearest].race == 0x88) {
            if (hit > 11) return;
            h = (hit - 5) * 2;
            i = jtrans[h++]; j = jtrans[h];
            if (wealth > j) {
                wealth -= j;
                prq(7);
                if (i == 0) { event(22); eat(50); }
                else if (i == 8) { stuff[i] += 10; event(23); }
                else {
                    stuff[i]++; extract("% bought a ");
                    print_cont(inv_list[i].name); print_cont(".");
                }
            }
            else print("Not enough money!");
        }
        break;
    case GAME:
        audio_options(menus[GAME].enabled[6] & 1, menus[GAME].enabled[7] & 1);
        if (hit == 6) setmood(TRUE);
        if (hit == 8) gomenu(SAVEX);
        if (hit == 9) { svflag = 0; gomenu(FILE_); }
        break;
    case USE:
        if (hit == 7) {
            if (hitgo) gomenu(KEYS); else extract("% has no keys!");
            return;
        }
        if (hit < 5) {
            if (hitgo) anim_list[0].weapon = hit + 1;
            else extract("% doesn't have one.");
        }
        if (hit == 6 && hitgo) {
            if (hero_x < 21373 && hero_x > 11194 && hero_y < 16208 && hero_y > 10205) break;
            get_turtle();
        }
        if (hit == 8 && witchflag) speak(60);
        gomenu(ITEMS);
        break;
    case SAVEX:
        if (hit == 6) quitflag = TRUE;
        if (hit == 5) { svflag = 1; gomenu(FILE_); }
        break;
    case FILE_:
        savegame(hit, svflag);
        gomenu(GAME);
        break;
    case KEYS:
        hit -= 5;
        bumped = 0;
        if (stuff[hit + KEYBASE]) {
            for (i = 0; i < 9; i++) {
                x = newx(hero_x, i, 16);
                y = newy(hero_y, i, 16);
                if (doorfind(x, y, hit + 1)) { stuff[hit + KEYBASE]--; break; }
            }
            if (i > 8) {
                extract("% tried a ");
                print_cont(inv_list[KEYBASE + hit].name);
                print_cont(" but it didn't"); print("fit.");
            }
        }
        gomenu(ITEMS);
        break;
    case GIVE:
        if (nearest_person == 0) break;
        k = anim_list[nearest_person].race;
        if (hit == 5 && wealth > 2) {                /* give gold to beggar */
            wealth -= 2;
            if ((int16_t)rand64() > kind) kind++;
            prq(4); prq(7);
            if (k == 0x8d) speak(24 + anim_list[nearest_person].goal);
            else speak(50);
        }
        else if (hit == 8 && stuff[29]) {            /* spectre */
            if (k != 0x8a) speak(21);
            else { speak(48); stuff[29] = 0; leave_item(nearest_person, 140); }
        }
        gomenu(ITEMS);
    }
    set_options_b();
}

/* fmain.c:1283-1363 - a key press (uppercase ASCII, digits, ' ', 10..16 = magic) */
void menu_key_b(uint8_t key)
{
    uint8_t i, atype, notpause = !(menus[GAME].enabled[5] & 1);

    if (viewstatus && notpause) { viewstatus = 99; return; }
    if (anim_list[0].state == DEAD) return;
    if (cmode == KEYS) {
        if (key >= '1' && key <= '6') do_option(key - '1' + 5);
        else gomenu(ITEMS);
        return;
    }
    /* Port addition: submenus whose entries have no key on the Amiga (mouse only) -
       digits select the n-th entry of Give and Files, 8/9 the sunstone and book in Use. */
    if (key >= '1' && key <= '9' && notpause) {
        uint8_t n = key - '1', first = (cmode >= USE) ? 0 : 5;
        if (cmode == GIVE || cmode == FILE_) {
            for (i = first; i < menus[cmode].num; i++) {
                if ((menus[cmode].enabled[i] & 2) == 0) continue;
                if (n-- == 0) {
                    hit = i;
                    hitgo = menus[cmode].enabled[hit] & 2;
                    do_option(hit);
                    print_options_b();
                    break;
                }
            }
            return;
        }
        if (cmode == USE && (key == '8' || key == '9')) {
            hit = key == '8' ? 8 : 9;
            hitgo = menus[cmode].enabled[hit] & 2;
            do_option(hit);
            print_options_b();
            return;
        }
    }
    if (key == ' ' || notpause) {
        for (i = 0; i < LMENUS; i++) {
            if (letter_list[i].letter == key) {
                uint8_t menu = letter_list[i].menu;
                if (menu == SAVEX && cmode != SAVEX) break;
                cmode = menu;
                hit = letter_list[i].choice;
                hitgo = menus[cmode].enabled[hit] & 2;
                atype = menus[cmode].enabled[hit] & 0xfc;
                if (atype == 4) menus[cmode].enabled[hit] ^= 1;
                do_option(hit);
                print_options_b();
                break;
            }
        }
    }
}

uint8_t paused_b(void)
{
    return menus[GAME].enabled[5] & 1;
}

void menus_start_b(void)
{
    cmode = ITEMS;
    set_options_b();
    print_options_b();
}
