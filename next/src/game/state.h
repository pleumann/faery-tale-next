/* Global game state (fmain.c globals) and small helpers shared by all game modules. */
#ifndef GAME_STATE_H
#define GAME_STATE_H

#include <stdint.h>

#define TRUE  1
#define FALSE 0

/* goal modes (ftale.h) */
#define USER        0
#define ATTACK1     1
#define ATTACK2     2
#define ARCHER1     3
#define ARCHER2     4
#define FLEE        5
#define STAND       6
#define DEATH       7
#define WAIT        8
#define FOLLOWER    9
#define CONFUSED    10

/* tactics */
#define FRUST       0
#define PURSUE      1
#define FOLLOW      2
#define BUMBLE_SEEK 3
#define RANDOM      4
#define BACKUP      5
#define EVADE       6
#define HIDE        7
#define SHOOT       8
#define SHOOTFRUST  9
#define EGG_SEEK    10
#define DOOR_SEEK   11
#define DOOR_LET    12

/* inventory (fmain.c:426) */
#define MAGICBASE   9
#define KEYBASE     16
#define STATBASE    25
#define GOLDBASE    31
#define ARROWBASE   35
#define STUFF_SIZE  36          /* original arrays were 35, stuff[ARROWBASE] wrote past them */

struct missile {
    uint16_t abs_x, abs_y;
    int8_t missile_type, time_of_flight, speed, direction, archer;
};

struct object {
    uint16_t xc, yc;
    uint8_t ob_id, ob_stat;
};

struct extent {
    uint16_t x1, y1, x2, y2;
    uint8_t etype, v1, v2, v3;
};

extern uint8_t *stuff;
extern uint8_t julstuff[STUFF_SIZE], philstuff[STUFF_SIZE], kevstuff[STUFF_SIZE];

extern uint16_t safe_x, safe_y, safe_r;
extern int16_t riding, flying, wcarry, turtleprox, raftprox;
extern int16_t brave, luck, kind, wealth, hunger, fatigue;
extern int16_t brother, princess;
extern uint16_t hero_place;
extern uint16_t daynight, lightlevel;
extern int16_t actor_file, set_file, active_carrier;
extern uint16_t xtype;
extern int16_t leader;
extern int16_t secret_timer, light_timer, freeze_timer;
extern uint16_t encounter_type;
extern uint8_t viewstatus, actors_on_screen, actors_loading, safe_flag, battleflag;
extern uint8_t frustflag, quitflag, witchflag, goodfairy;
extern int16_t nearest, nearest_person, perdist, last_person;
extern int16_t dayperiod, sleepwait;
extern uint8_t encounter_number, danger_level;
extern uint16_t encounter_x, encounter_y;
extern int16_t mixflag, wt;
extern uint8_t fiery_death, turtle_eggs, bumped;
extern uint8_t anix2, mdex;
extern uint8_t anim_index[20];
extern struct missile missile_list[6];
extern struct extent extent_list[];
extern struct extent *extn;
extern uint8_t keyfight;
extern uint8_t leave_program;      /* Quit in the Save/Exit menu: reset the machine */

/* random numbers - fsubs.asm _rand */
extern uint32_t seed1;
uint16_t rand16(void);          /* rand() & 0xFFFF */
#define rand2()   (rand16() & 1)
#define rand4()   (rand16() & 3)
#define rand8()   (rand16() & 7)
#define rand64()  (rand16() & 63)
#define rand256() (rand16() & 255)
#define bitrand(x) (rand16() & (x))
uint16_t rnd(uint16_t n);

/* text output - the message area comes with M5 */
void event(uint8_t n);
void speak(uint8_t n);
void print(const char *s);
void prq(uint8_t n);

/* sound - comes with M6 */
void effect(uint8_t num, uint16_t period);
void setmood(uint8_t now);

#endif
