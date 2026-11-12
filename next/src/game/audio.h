/* Music and sound effects (banked, PAGE_AUDIO) - resident wrappers in glue.c */
#ifndef GAME_AUDIO_H
#define GAME_AUDIO_H

#include <stdint.h>

void audio_init(void);          /* load MUSIC.DAT, set up the AY chips */
void audio_update(void);        /* call often: runs one player step per elapsed frame */
void audio_options(uint8_t music, uint8_t sound);
void stopscore(void);
void music_play(uint8_t off);     /* start tracks off..off+3 (12 = intro) */

/* effect() and setmood() are declared in state.h */

#endif
