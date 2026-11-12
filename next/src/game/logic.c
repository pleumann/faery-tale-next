/* Banked module (PAGE_LOGIC): the periodic world logic of the main loop (day/night,
   encounters, enemy AI, hunger and fatigue), encounters and the hero's revival.
   Ported from fmain.c main() lines 2013-2221, load_actors/set_encounter/load_carrier/
   revive, and fmain2.c do_tactic/aftermath/set_loc. */

#include <stdint.h>
#include "world.h"
#include "state.h"
#include "actors.h"
#include "shapes.h"
#include "banked.h"
#include "ui.h"
#include "menus.h"
#include "placard.h"
#include "intro.h"

/* fmain.c:45 */
struct encounter {
    int8_t hitpoints, agressive, arms, cleverness, treasure, file_id;
};

static const struct encounter encounter_chart[11] = {
    { 18, TRUE, 2, 0, 2, 6 },   /* 0 - Ogre */
    { 12, TRUE, 4, 1, 1, 6 },   /* 1 - Orcs */
    { 16, TRUE, 6, 1, 4, 7 },   /* 2 - Wraith */
    {  8, TRUE, 3, 0, 3, 7 },   /* 3 - Skeleton */
    { 16, TRUE, 6, 1, 0, 8 },   /* 4 - Snake - swamp region */
    {  9, TRUE, 3, 0, 0, 7 },   /* 5 - Salamander - lava region */
    { 10, TRUE, 6, 1, 0, 8 },   /* 6 - Spider - spider pits */
    { 40, TRUE, 7, 1, 0, 8 },   /* 7 - DKnight - elf glade */
    { 12, TRUE, 6, 1, 0, 9 },   /* 8 - Loraii - astral plane */
    { 50, TRUE, 5, 0, 0, 9 },   /* 9 - Necromancer - final arena */
    {  4, FALSE, 0, 0, 0, 9 },  /* 10 - Woodcutter */
};

/* fmain2.c:860 */
static const int8_t weapon_probs[32] = {
    0,0,0,0,    /* no weapons */
    1,1,1,1,    /* dirks only */
    1,2,1,2,    /* dirks and maces */
    1,2,3,2,    /* mostly maces */
    4,4,3,2,    /* swords and bows */
    5,5,5,5,    /* magic wand */
    8,8,8,8,    /* touch attack! */
    3,3,3,3,    /* swords only */
};

/* fmain2.c:1714 */
static void set_loc(void)
{
    uint8_t j = rand8();                    /* direction */
    int16_t d = 150 + rand64();             /* distance */
    encounter_x = newx(hero_x, j, d);
    encounter_y = newy(hero_y, j, d);
}

/* fmain.c:2722 */
void load_actors_b(void)
{
    encounter_number = extn->v1 + rnd(extn->v2);
    if (actor_file != encounter_chart[encounter_type].file_id) {
        actor_file = encounter_chart[encounter_type].file_id;
        anix = 3;
        read_shapes(actor_file);
        actors_loading = TRUE;
        active_carrier = 0;
    }
}

#define MAX_TRY 15

/* fmain.c:2736 */
uint8_t set_encounter_b(uint8_t i, uint16_t spread)
{
    struct shape *an = &anim_list[i];
    uint16_t xtest = 0, ytest = 0;
    uint8_t race, w, j;

    if (extn->v3 == 7) { xtest = 21635; ytest = 25762; j = 0; }
    else for (j = 0; j < MAX_TRY; j++) {
        xtest = encounter_x + bitrand(spread) - (spread / 2);
        ytest = encounter_y + bitrand(spread) - (spread / 2);
        if (proxcheck(xtest, ytest, i) == 0) break;
        if (xtype == 52 && px_to_im(xtest, ytest) == 7) break;
    }
    if (j == MAX_TRY) return FALSE;
    an->abs_x = xtest;
    an->abs_y = ytest;
    an->type = ENEMY;
    if ((mixflag & 2) && encounter_type != 4)
        race = (encounter_type & 0xfffe) + rand2();
    else
        race = encounter_type;
    an->race = race;
    if (mixflag & 4) wt = rand4();
    w = (encounter_chart[race].arms * 4) + wt;
    an->weapon = weapon_probs[w];
    an->state = STILL;
    an->environ = an->facing = 0;
    if (an->weapon & 4) an->goal = ARCHER1 + encounter_chart[race].cleverness;
    else an->goal = ATTACK1 + encounter_chart[race].cleverness;
    an->vitality = encounter_chart[race].hitpoints;
    an->rel_x = (int16_t)(an->abs_x - map_x - 8);
    an->rel_y = (int16_t)(an->abs_y - map_y - 26);
    return TRUE;
}

/* fmain.c:2784 */
void load_carrier_b(uint8_t n)
{
    struct shape *an = &anim_list[3];
    uint8_t i;
    an->type = (n == 10) ? DRAGON : CARRIER;
    if (n == 10) i = 2; else if (n == 5) i = 1; else i = 0;
    if (actor_file != n)
        read_shapes(n);
    an->abs_x = extent_list[i].x1 + 250;
    an->abs_y = extent_list[i].y1 + 200;
    an->index = an->weapon = an->environ = 0;
    an->state = STILL;
    an->vitality = 50;
    anix = 4;
    an->race = actor_file = active_carrier = n;
}

#define EXT_COUNT 22

/* fmain.c:2647 - where are we: place name (M5) and extent (encounter zones) */
void find_place_b(uint8_t flag)
{
    uint8_t i;

    hero_sector &= 255;
    if (region_num > 7) hero_sector += 256;
    {
        uint8_t m = place_lookup((uint8_t)hero_sector, region_num > 7);
        if (m == 4) {                               /* message number 4 -- the mountains */
            if (region_num > 7) ;
            else if (region_num & 1) m = 0;
            else if (region_num > 3) m = 5;
        }
        if (new_region < NO_REGION || ((hero_x >> 8) - xreg) & 64 || ((hero_y >> 8) - yreg) & 32)
            m = 0;
        if (m && m != hero_place) {
            hero_place = m;
            if (flag) msg_place(region_num > 7, m);
        }
    }

    extn = extent_list;
    for (i = 0; i < EXT_COUNT; i++) {
        if (hero_x > extn->x1 && hero_x < extn->x2 && hero_y > extn->y1 && hero_y < extn->y2)
            break;
        extn++;
    }

    if (xtype != extn->etype) {
        xtype = extn->etype;
        if (xtype == 83 && ob_stat_get(8, 9)) {
            rescue();
            flag = 0;
            xtype = 0xFF;                           /* findagain: re-evaluate the extent */
            find_place_b(flag);
            return;
        }
        else if (xtype >= 60) {                     /* birds/turtles/special */
            if (xtype == 60 || xtype == 61) {
                if (anim_list[3].race != extn->v3 || anix < 4) {
                    encounter_x = (extn->x1 + extn->x2) / 2;
                    encounter_y = (extn->y1 + extn->y2) / 2;
                    goto force;
                }
            }
        }
        else if (xtype == 52) {
            encounter_type = 8;
            load_actors_b();
            actors_loading = FALSE;
        }
        else if (xtype >= 50 && flag == 1) {
            encounter_x = hero_x;
            encounter_y = hero_y;
force:
            encounter_type = extn->v3;
            mixflag = wt = 0;
            load_actors_b();
            actors_loading = FALSE;
            encounter_number = extn->v1;
            anix = 3;
            while (encounter_number && anix < 7) {
                if (set_encounter_b(anix, 63)) anix++;
                encounter_number--;
            }
        }
    }
    if (xtype < 70) active_carrier = 0;
    else if (xtype == 70 && (!active_carrier || (!riding && actor_file != extn->v3)))
        load_carrier_b(extn->v3);
}

/* fmain2.c:1664 */
static void do_tactic(uint8_t i, uint8_t tactic)
{
    struct shape *an = &anim_list[i];
    uint8_t r = !(rand16() & 7), f;

    an->tactic = tactic;
    if (an->goal == ATTACK2) r = !(rand16() & 3);
    if (tactic == PURSUE) { if (r) set_course(i, hero_x, hero_y, 0); }
    else if (tactic == SHOOT) {
        int16_t xd = (int16_t)(an->abs_x - anim_list[0].abs_x);
        int16_t yd = (int16_t)(an->abs_y - anim_list[0].abs_y);
        if (xd < 0) xd = -xd;
        if (yd < 0) yd = -yd;
        if ((rand16() & 1) && (xd < 8 || yd < 8 || (xd > yd - 5 && xd < yd + 7))) {
            set_course(i, hero_x, hero_y, 5);
            if (an->state < SHOOT1) an->state = SHOOT1;
        }
        else set_course(i, hero_x, hero_y, 0);
    }
    else if (tactic == RANDOM) {
        if (r) an->facing = rand16() & 7;
        an->state = WALKING;
    }
    else if (tactic == BUMBLE_SEEK) { if (r) set_course(i, hero_x, hero_y, 4); }
    else if (tactic == BACKUP) { if (r) set_course(i, hero_x, hero_y, 3); }
    else if (tactic == FOLLOW) {
        f = leader;
        if (i == f) an->tactic = RANDOM;        /* can't follow self!! */
        if (r) set_course(i, anim_list[f].abs_x, anim_list[f].abs_y + 20, 0);
    }
    else if (tactic == EVADE) {
        /* original: f = i+i (out of range for i > 9); kept within the list */
        f = (i == anix) ? i - 1 : i + i;
        if (f >= MAX_ACTORS) f = i - 1;
        if (r) set_course(i, anim_list[f].abs_x, anim_list[f].abs_y + 20, 2);
    }
    else if (tactic == EGG_SEEK) {
        if (r) set_course(i, 23087, 5667, 0);
        an->state = WALKING;
    }
}

/* fmain2.c:253 */
static void aftermath(void)
{
    uint8_t dead = 0, flee = 0, i;
    for (i = 3; i < anix; i++) {
        if (anim_list[i].type != ENEMY) ;
        else if (anim_list[i].state == DEAD) dead++;
        else if (anim_list[i].goal == FLEE) flee++;
    }
    if (anim_list[0].vitality < 1) ;
    else if (anim_list[0].vitality < 5 && dead) print("Bravely done!");
    else if (xtype < 50) {
        if (dead) { print(""); prdec(dead, 1); print_cont("foes were defeated in battle."); }
        if (flee) { print(""); prdec(flee, 1); print_cont("foes fled in retreat."); }
    }
    /* TODO (M4+): turtle eggs -> get_turtle() */
}

/* fmain.c:2013-2221: everything that happens on frames without a tile scroll */
void tick_logic_b(void)
{
    uint8_t battle2, i, k;

    if (anim_list[0].state == SLEEP) {
        daynight += 63;
        if (fatigue) fatigue--;
        if (fatigue == 0 ||
            (fatigue < 30 && daynight > 9000 && daynight < 10000) ||
            (battleflag && rand64() == 0)) {
            anim_list[0].state = STILL;
            hero_y = (anim_list[0].abs_y &= 0xffe0);
        }
    }
    if (!freeze_timer)                              /* no time in timestop */
        if ((daynight++) >= 24000) daynight = 0;
    lightlevel = daynight / 40;
    if (lightlevel >= 300) lightlevel = 600 - lightlevel;
    ob_stat_set(OBLIST_GLOBAL, 5, lightlevel < 40 ? 3 : 2);    /* spectre at night */
    i = daynight / 2000;
    if (i != dayperiod) {
        switch (dayperiod = i) {
        case 0: event(28); break;
        case 4: event(29); break;
        case 6: event(30); break;
        case 9: event(31); break;
        }
    }

    day_fade();

    if ((daynight & 0x3ff) == 0) {
        if (anim_list[0].vitality < (15 + brave / 4) && anim_list[0].state != DEAD) {
            anim_list[0].vitality++;
            prq(4);
        }
    }

    if (freeze_timer) return;                       /* stasis */

    find_place(2);

    if (actors_loading == TRUE) {                   /* shapes are loaded synchronously */
        actors_loading = FALSE;
        anix = 3;
    }
    if ((daynight & 15) == 0 && encounter_number && !actors_loading) {
        mixflag = rand16(); if (xtype > 49) mixflag = 0;
        wt = rand4();
        if ((xtype & 3) == 0) mixflag = 0;
        for (k = 1; k < 10; k++) {                  /* try to place encounter 10 times */
            set_loc();
            if (px_to_im(encounter_x, encounter_y) == 0) {
                while (encounter_number && anix < 7) {
                    if (set_encounter_b(anix, 63)) anix++;
                    encounter_number--;
                }
                for (i = 3; i < 7 && encounter_number; i++) {
                    if (anim_list[i].state == DEAD &&
                        (!anim_list[i].visible || anim_list[i].race == 2)) {
                        set_encounter_b(i, 63);
                        encounter_number--;
                    }
                }
                break;
            }
        }
    }
    if ((daynight & 31) == 0 && !actors_on_screen && !actors_loading && !active_carrier && xtype < 50) {
        if (region_num > 7) danger_level = 5 + xtype;
        else danger_level = 2 + xtype;
        if (rand64() <= danger_level) {
            encounter_type = rand4();
            if (xtype == 7 && encounter_type == 2) encounter_type = 4;
            if (xtype == 8) { encounter_type = 6; mixflag = 0; }
            if (xtype == 49) { encounter_type = 2; mixflag = 0; }
            load_actors_b();
        }
    }
    k = anim_list[nearest_person].race;
    if (nearest_person && k != last_person) {
        switch (k) {
        case 0x8d: speak(23); break;                /* beggar */
        case 0x89: speak(46); break;                /* witch */
        case 0x84: if (ob_stat_get(8, 9)) speak(16); break;    /* princess */
        case 9:    speak(43); break;                /* necromancer */
        case 7:    speak(41); break;                /* dknight */
        }
        last_person = k;
    }
    actors_on_screen = FALSE;
    leader = 0;
    battle2 = battleflag;
    battleflag = FALSE;
    for (i = 2; i < anix; i++) {                    /* skip raft */
        int16_t xd, yd;
        uint8_t mode, tactic, r;
        struct shape *an;

        if (goodfairy && goodfairy < 120) break;
        an = &anim_list[i];
        if (an->type == CARRIER) {
            if ((daynight & 15) == 0) set_course(i, hero_x, hero_y, 5);
            continue;
        }
        else if (an->type == SETFIG) continue;
        mode = an->goal;
        tactic = an->tactic;

        xd = (int16_t)(an->abs_x - hero_x);
        yd = (int16_t)(an->abs_y - hero_y);
        if (xd < 0) xd = -xd;
        if (yd < 0) yd = -yd;
        if (xd < 300 && yd < 300) {
            actors_on_screen = TRUE;
            if (anim_list[i].vitality < 1) continue;
            if (an->visible || battle2) battleflag = TRUE;
        }
        r = !bitrand(15);
        if (anim_list[0].state == DEAD || anim_list[0].state == FALL) {
            mode = (leader == 0) ? FLEE : FOLLOWER;
        }
        else if (an->vitality < 2 || (xtype > 59 && an->race != extn->v3))
            mode = FLEE;                            /* special encounters don't flee */
        if (tactic == FRUST || tactic == SHOOTFRUST) {
            if (an->weapon & 4) do_tactic(i, rand4() + 2);
            else do_tactic(i, rand2() + 3);
        }
        else if (an->state == SHOOT1) an->state = SHOOT3;
        else if (mode <= ARCHER2) {                 /* hostile character */
            int8_t thresh;
            if ((mode & 2) == 0) r = !rand4();
            if (r) {
                if (an->race == 4 && turtle_eggs) tactic = EGG_SEEK;
                else if (an->weapon < 1) { mode = CONFUSED; tactic = RANDOM; }
                else if (an->vitality < 6 && rand2()) tactic = EVADE;
                else if (mode >= ARCHER1) {
                    if (xd < 40 && yd < 30) tactic = BACKUP;
                    else if (xd < 70 && yd < 70) tactic = SHOOT;
                    else tactic = PURSUE;
                }
                else tactic = PURSUE;
            }
            thresh = 14 - mode;
            if (an->race == 7) thresh = 16;
            if ((an->weapon & 4) == 0 && xd < thresh && yd < thresh) {
                set_course(i, hero_x, hero_y, 0);
                if (an->state >= WALKING) an->state = FIGHTING;
            }
            else if (an->race == 7 && an->vitality) { an->state = STILL; an->facing = 5; }
            else do_tactic(i, tactic);
        }
        else if (mode == FLEE) do_tactic(i, BACKUP);
        else if (mode == FOLLOWER) do_tactic(i, FOLLOW);
        else if (mode == STAND) { set_course(i, hero_x, hero_y, 0); an->state = STILL; }
        else if (mode == WAIT) an->state = STILL;

        an->goal = mode;
        if (leader == 0) leader = i;
    }
    if (battle2 == 0 && battleflag) setmood(1);
    if (battleflag == 0 && battle2) { prq(7); prq(4); aftermath(); }
    if ((daynight & 127) == 0 && !actors_on_screen && !actors_loading && !witchflag &&
        anim_list[0].environ == 0 && safe_flag == 0 && anim_list[0].state != DEAD) {
        safe_r = region_num;
        safe_x = hero_x; safe_y = hero_y;
        if (hunger > 30 && stuff[24]) { stuff[24]--; hunger -= 30; event(37); }
    }
    if ((daynight & 7) == 0 && !battleflag) setmood(0);
    if ((daynight & 127) == 0 && anim_list[0].vitality && anim_list[0].state != SLEEP) {
        hunger++;
        fatigue++;
        if (hunger == 35) event(0);
        else if (hunger == 60) event(1);
        else if ((hunger & 7) == 0) {
            if (anim_list[0].vitality > 5) {
                if (hunger > 100 || fatigue > 160) { anim_list[0].vitality -= 2; prq(4); }
                if (hunger > 90) event(2);
            }
            else if (fatigue > 170) { event(12); anim_list[0].state = SLEEP; }
            else if (hunger > 140) { event(24); hunger = 130; anim_list[0].state = SLEEP; }
        }
        if (fatigue == 70) event(3);
        else if (hunger == 90) event(4);
    }
}

/* fmain.c:2806 */
struct bro { int8_t brave, luck, kind, wealth; uint8_t *stuff; };

/* fmain.c:2814 - new tells if this is a new character */
void revive_b(uint8_t new_hero)
{
    struct shape *an;
    uint8_t i;
    static const int8_t battr[3][4] = { { 35,20,15,20 }, { 20,35,15,15 }, { 15,20,35,10 } };

    an = &anim_list[1];
    an->type = RAFT;
    an->abs_x = 13668; an->abs_y = 14470;
    an->index = an->weapon = an->environ = 0;

    an = &anim_list[2];
    an->type = SETFIG;
    an->abs_x = 13668; an->abs_y = 15000;
    an->index = an->weapon = 0;

    an = &anim_list[0];
    an->type = PHIL;
    an->goal = USER;

    battleflag = goodfairy = mdex = 0;
    if (new_hero) {
        if (brother > 0 && brother < 3) {
            ob_pos_set(OBLIST_GLOBAL, brother, hero_x, hero_y);
            ob_stat_set(OBLIST_GLOBAL, brother, 1);
            ob_stat_set(OBLIST_GLOBAL, brother + 2, 3);
        }
        ob_stat_set(8, 9, 3);
        if (brother < 3) {
            brave = battr[brother][0]; luck = battr[brother][1];
            kind = battr[brother][2]; wealth = battr[brother][3];
            stuff = brother == 0 ? julstuff : brother == 1 ? philstuff : kevstuff;
        }
        brother++;

        for (i = 0; i < GOLDBASE; i++) stuff[i] = 0;    /* has no stuff */
        stuff[0] = an->weapon = 1;                      /* okay, a dirk, then */

        secret_timer = light_timer = freeze_timer = 0;
        safe_x = 19036; safe_y = 15755; region_num = safe_r = 3;
        map_adjust(safe_x, safe_y);
        viewstatus = 99;
        actors_on_screen = TRUE;
        actors_loading = FALSE;

        map_message();
        if (brother == 1) placard_text(0);
        else if (brother == 2) placard_text(1);
        else if (brother == 3) placard_text(3);
        else placard_text(5);
        placard();
        delay(120);
        if (brother > 3) { quitflag = TRUE; delay(500); }
        else if (brother > 1) {
            delay(80);
            placard_clear();
            delay(10);
            if (brother == 2) placard_text(2); else placard_text(4);
            delay(120);
        }

        actor_file = 6; set_file = 13;
        shape_read();

        if (brother < 4) {
            message_off();
            hero_place = 2;
            event(9);
            if (brother == 2) event(10);
            else if (brother == 3) event(11);
        }
    }
    else fade_down();

    hero_x = an->abs_x = safe_x;
    hero_y = an->abs_y = safe_y;
    map_x = hero_x - 144;
    map_y = hero_y - 90;
    new_region = safe_r;
    load_all();
    an->vitality = 15 + brave / 4;
    an->environ = 0;
    an->state = STILL;
    an->race = 0xff;
    daynight = 8000; lightlevel = 300;
    hunger = fatigue = 0;
    anix = 3;
    set_options();
    prq(7); prq(4);
    if (brother > 3) viewstatus = 2;
    else { viewstatus = 3; setmood(TRUE); }
    fiery_death = 0;
    xtype = 0;
}

/* fmain.c:3510 */
void get_turtle_b(void)
{
    uint8_t i;
    for (i = 0; i < 25; i++) {
        set_loc();
        if (px_to_im(encounter_x, encounter_y) == 5) break;
    }
    if (i == 25) return;
    move_extent(1, encounter_x, encounter_y);
    load_carrier_b(5);
}

/* Test helper: an ogre band around the hero right now (like a random encounter) */
void debug_encounter_b(void)
{
    uint8_t k;
    encounter_type = 0;
    mixflag = 0; wt = rand4();
    load_actors_b();
    actors_loading = FALSE;
    anix = 3;
    for (k = 0; k < 10; k++) {
        set_loc();
        if (px_to_im(encounter_x, encounter_y) == 0) {
            while (encounter_number && anix < 7) {
                if (set_encounter_b(anix, 63)) anix++;
                encounter_number--;
            }
            break;
        }
    }
}

void game_start_b(void)
{
    brother = 0;
    revive_b(TRUE);
}
