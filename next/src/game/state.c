/* Global game state and helpers that must stay resident. */
#include <stdint.h>
#include "state.h"

uint8_t *stuff = julstuff;
uint8_t julstuff[STUFF_SIZE], philstuff[STUFF_SIZE], kevstuff[STUFF_SIZE];

uint16_t safe_x, safe_y, safe_r;
int16_t riding, flying, wcarry, turtleprox, raftprox;
int16_t brave, luck, kind, wealth, hunger, fatigue;
int16_t brother, princess;
uint16_t hero_place;
uint16_t daynight, lightlevel;
int16_t actor_file, set_file, active_carrier;
uint16_t xtype;
int16_t leader;
int16_t secret_timer, light_timer, freeze_timer;
uint16_t encounter_type;
uint8_t viewstatus, actors_on_screen, actors_loading, safe_flag, battleflag;
uint8_t frustflag, quitflag, witchflag, goodfairy;
int16_t nearest, nearest_person, perdist, last_person;
int16_t dayperiod, sleepwait;
uint8_t encounter_number, danger_level;
uint16_t encounter_x, encounter_y;
int16_t mixflag, wt;
uint8_t fiery_death, turtle_eggs, bumped;
uint8_t anix2, mdex;
uint8_t anim_index[20];
struct missile missile_list[6];
uint8_t keyfight;
uint8_t leave_program;

/* fmain.c:338 - extents (modified at run time by move_extent) */
struct extent extent_list[] = {
    {  2118,27237,  2618,27637, 70,0,1,11 },    /* bird extent */
    {     0,    0,     0,    0, 70,0,1,5 },     /* turtle extent */
    {  6749,34951,  7249,35351, 70,0,1,10 },    /* dragon extent */
    {  4063,34819,  4909,35125, 53,4,1,6 },     /* spider pit */
    {  9563,33883, 10144,34462, 60,1,1,9 },     /* necromancer */
    { 22945, 5597, 23225, 5747, 61,3,2,4 },     /* turtle eggs */
    { 10820,35646, 10877,35670, 83,1,1,0 },     /* princess extent */
    { 19596,17123, 19974,17401, 48,8,8,2 },     /* graveyard ext */
    { 19400,17034, 20240,17484, 80,4,20,0 },    /* around city */
    { 0x2400,0x8200,0x3100,0x8a00, 52,3,1,8 },  /* astral plane */
    {  5272,33300,  6112,34200, 81,0,1,0 },     /* king pax */
    { 11712,37350, 12416,38020, 82,0,1,0 },     /* sorceress pax */
    {  2752,33300,  8632,35400, 80,0,1,0 },     /* peace 1 - buildings */
    { 10032,35550, 12976,40270, 80,0,1,0 },     /* peace 2 - specials */
    {  4712,38100, 10032,40350, 80,0,1,0 },     /* peace 3 - cabins */
    { 21405,25583, 21827,26028, 60,1,1,7 },     /* hidden valley */
    {  6156,12755, 12316,15905,  7,1,8,0 },     /* swamp region */
    {  5140,34860,  6260,37260,  8,1,8,0 },     /* spider region */
    {   660,33510,  2060,34560,  8,1,8,0 },     /* spider region */
    { 18687,15338, 19211,16136, 80,0,1,0 },     /* village */
    { 16953,18719, 20240,17484,  3,1,3,0 },     /* around village */
    { 20593,18719, 23113,22769,  3,1,3,0 },     /* around city */
    { 0,0, 0x7fff, 0x9fff,  3,1,8,0 },          /* whole world */
};
struct extent *extn = extent_list;

/* fsubs.asm _rand: seed = (seed & 0xFFFF) * 45821 + 1, result = ror(seed, 6) & 0x7FFFFFFF.
   Only the low 16 bits of the result are ever used by the game. */
uint32_t seed1 = 19837325UL;

uint16_t rand16(void)
{
    uint32_t s = (seed1 & 0xFFFF) * 45821UL + 1;
    seed1 = s;
    return (uint16_t)(s >> 6);          /* low 16 bits of the rotate */
}

/* rnd(n): (rand() & 0xFFFF) % n */
uint16_t rnd(uint16_t n)
{
    return rand16() % n;
}


