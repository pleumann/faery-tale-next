/* Actors as Next hardware sprites. Replaces save_blit/mask_blit/shape_blit/rest_blit
   from fsubs.asm and the drawing loop in fmain.c (lines 2381-2609). */
#ifndef GAME_SPRITES_H
#define GAME_SPRITES_H

#include <stdint.h>

#define SHAPE_PAGE_PHIL  64     /* brother's figures: 67 frames x 512 bytes = 5 pages (64..68) */
#define SHADOW_PAGE      61     /* 192 shadow masks x 64 bytes = 12K (61..62) */

void sprites_init(void);
uint8_t sprites_load_set(uint8_t set, uint8_t page);   /* from SHAPES.DAT, 0 on error */
void sprites_palette(const uint16_t *rgb4, uint8_t pct);
void sprites_draw(void);        /* compose all visible actors (patterns double buffered) */
void sprites_flush(void);       /* write sprite attributes - right after the vblank */

#endif
