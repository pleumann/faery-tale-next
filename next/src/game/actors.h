/* Actors (anim_list) and their movement - ported from fmain.c main loop and fmain2.c. */
#ifndef GAME_ACTORS_H
#define GAME_ACTORS_H

#include <stdint.h>

/* motion states (ftale.h) */
#define WALKING     12
#define STILL       13
#define FIGHTING    0
#define DYING       14
#define DEAD        15
#define SINK        16
#define OSCIL       17
#define TALKING     19
#define FROZEN      20
#define FLYING      21
#define FALL        22
#define SLEEP       23
#define SHOOT1      24
#define SHOOT3      25

/* actor types = seq_list slots (ftale.h enum sequences) */
#define PHIL        0
#define OBJECTS     1
#define ENEMY       2
#define RAFT        3
#define SETFIG      4
#define CARRIER     5
#define DRAGON      6

#define DIR_NONE    9

/* struct shape (ftale.h); field order kept, layout is not relied upon */
struct shape {
    uint16_t abs_x, abs_y, rel_x, rel_y;
    int8_t  type;
    uint8_t race;
    int8_t  index, visible, weapon, environ, goal, tactic, state, facing;
    int16_t vitality;
    int8_t  vel_x, vel_y;
};

struct state {
    int8_t figure, wpn_no, wpn_x, wpn_y;
};

#define MAX_ACTORS  20

extern struct shape anim_list[MAX_ACTORS];
extern uint8_t anix;            /* number of active actors */
extern uint8_t cycle;           /* main loop counter */
extern uint16_t hero_x, hero_y;
extern const struct state statelist[87];
extern const int8_t diroffs[16];

uint16_t newx(uint16_t x, uint8_t dir, int8_t speed);
uint16_t newy(uint16_t y, uint8_t dir, int8_t speed);
uint8_t proxcheck(uint16_t x, uint16_t y, uint8_t i);

void actors_init_hero(uint16_t x, uint16_t y);
void hero_input(uint8_t dir);   /* dir 0..7 or DIR_NONE */
void actors_update(void);

#endif
