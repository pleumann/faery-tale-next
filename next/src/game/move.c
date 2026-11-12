/* Banked module (PAGE_MOVE): hero control, the actor loop, melee/missiles and the depth
   sort of the main loop - fmain.c lines 1384-1865 and 2238-2359. */

#include <stdint.h>
#include "world.h"
#include "state.h"
#include "actors.h"
#include "banked.h"

/* fmain.c:137 - fight animation transitions */
static const int8_t trans_list[9][4] = {
    {  1, 8, 0, 1 }, {  2, 0, 1, 0 }, {  3, 1, 2, 8 }, {  4, 2, 3, 7 }, {  5, 3, 4, 6 },
    {  6, 4, 5, 5 }, {  8, 5, 6, 4 }, {  8, 6, 7, 3 }, {  0, 6, 8, 2 },
};

/* fmain2.c:871 */
static const uint8_t fallstates[24] = {
    0,0,0,0,0,0, 0x20,0x22,0x3a,0x6f,0x70,0x71,
    0x24,0x27,0x3c,0x6f,0x70,0x71, 0x37,0x38,0x3d,0x6f,0x70,0x71 };

/* fmain2.c:885 */
static const int8_t bowshotx[8] = {  0, 0, 3, 6,-3,-3,-3,-6 };
static const int8_t bowshoty[8] = { -6,-6,-1, 0, 6, 8, 0,-1 };
static const int8_t gunshoty[8] = {  2, 0, 4, 7, 9, 4, 7, 8 };

/* fmain.c:1384-1462 - the hero's state from the controls; dir 0..7 or 9, fire = button */
void hero_control_b(uint8_t dir, uint8_t fire)
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

void actors_update_b(void)
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

