/* Actors as Next hardware sprites. Replaces save_blit/mask_blit/shape_blit/rest_blit
   from fsubs.asm and the drawing loop in fmain.c (lines 2381-2609). */
#ifndef GAME_SPRITES_H
#define GAME_SPRITES_H

#include <stdint.h>

#define SHADOW_PAGE      61     /* 192 shadow masks x 64 bytes = 12K (61..62) */

void sprites_init(void);
void sprites_palette(const uint16_t *rgb4, uint8_t pct);
void sprites_draw(void);        /* compose all visible actors (patterns double buffered) */
void sprites_flush(void);
void sprites_hide(void);
void pointer_init(void);
void pointer_set(uint16_t x, uint8_t y, uint8_t visible);       /* write sprite attributes - right after the vblank */

#endif
