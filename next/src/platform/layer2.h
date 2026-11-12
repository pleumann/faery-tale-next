/* Layer 2 in 320x256x8 mode (column-major: offset = x*256 + y, 10 pages of 8K). */
#ifndef LAYER2_H
#define LAYER2_H

#include <stdint.h>

void layer2_init(void);
void layer2_show(uint8_t bank16k);
void layer2_clip(uint16_t x1, uint8_t y1, uint16_t x2, uint8_t y2);

/* Palette in Amiga RGB4 words; pct 0..100 per channel like fade_page() in fmain2.c */
void palette_load_rgb4(uint8_t first, const uint16_t *rgb4, uint8_t count, uint8_t pct);

#endif
