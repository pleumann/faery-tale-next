/* Playfield rendering: Layer 2 as a 320x256 torus (20 x 8 tiles of 16x32),
   hardware scrolling, only new tile columns/rows are drawn. Replaces map_draw,
   strip_draw, row_draw and scrollmap from fsubs.asm. */
#ifndef GAME_VIEW_H
#define GAME_VIEW_H

#include <stdint.h>

/* Playfield position on screen (Amiga: 288x140 at x 16 in a 320x200 display) */
#define VIEW_X      16
#define VIEW_Y      28
#define VIEW_W      288
#define VIEW_H      140

void view_init(void);
void view_invalidate(void);     /* force full redraw (region change, teleport) */
void view_update(void);         /* draw new tiles for map_x/map_y (seams are off screen) */
void view_scroll(void);
void view_clear(void);
void view_blit_object(uint8_t frame, uint8_t off, uint8_t h, uint16_t px, uint8_t py);         /* set the scroll registers - right after the vblank */

#endif
