/* Placards and still screens (banked, PAGE_PLACARD) - resident wrappers in glue.c */
#ifndef GAME_PLACARD_H
#define GAME_PLACARD_H

#include <stdint.h>

void font_load(void);
void map_message(void);         /* fade out, clear the playfield, hide the panel */
void message_off(void);         /* back to the game screen */
void placard_text(uint8_t n);   /* narr.asm placard text n (mst table) */
void placard_name(void);        /* the brother's name at the pen position */
void placard_ssp(const char *s);/* XY-coded string from resident memory */
void placard_pen(uint8_t c);
void placard_center(uint8_t on);
void placard(void);             /* the meander border */
void placard_clear(void);       /* inner area to colour 0 */
void bigdraw(void);             /* the magic map on the playfield */
void l2_pixel(int16_t x, int16_t y, uint8_t c);

void delay(uint16_t ticks);     /* Amiga Delay(): 1/50 s ticks, music keeps playing */
void fade_down(void);
void fade_normal(void);

#endif
