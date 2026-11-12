/* Keyboard and Kempston joystick. */
#ifndef INPUT_H
#define INPUT_H

#include <stdint.h>

#define IN_UP     0x01
#define IN_DOWN   0x02
#define IN_LEFT   0x04
#define IN_RIGHT  0x08
#define IN_FIRE   0x10

/* Directions from the cursor keys (CAPS SHIFT + 5..8, the Next's arrow keys) and a
   Kempston joystick; IN_FIRE from joystick fire or the 0 key. */
uint8_t input_dirs(void);

/* Next newly pressed key: 'A'..'Z', '0'..'9', ' ', 13 = ENTER,
   10..16 = SYMBOL SHIFT + 1..7 (the Amiga's F1..F7 magic keys); 0 = none.
   Keys held with CAPS SHIFT (cursor keys) are ignored. */
uint8_t input_key(void);

/* 1 if the key in the given half-row port (e.g. 0xDFFE) and bit mask is pressed. */
uint8_t key_down(uint16_t port, uint8_t mask);

/* Kempston mouse, pointer in screen coordinates (0..319, 28..231) */
#define MOUSE_LEFT  1
#define MOUSE_RIGHT 2
extern uint16_t mouse_x;
extern uint8_t mouse_y, mouse_buttons, mouse_present;
void mouse_update(void);

#endif
