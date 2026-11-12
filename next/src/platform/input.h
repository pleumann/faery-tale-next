/* Keyboard and Kempston joystick. */
#ifndef INPUT_H
#define INPUT_H

#include <stdint.h>

#define IN_UP     0x01
#define IN_DOWN   0x02
#define IN_LEFT   0x04
#define IN_RIGHT  0x08
#define IN_FIRE   0x10

/* Directions from Q/A/O/P, cursor keys (5-8, with or without CAPS SHIFT) and joystick. */
uint8_t input_dirs(void);

/* 1 if the key in the given half-row port (e.g. 0xDFFE) and bit mask is pressed. */
uint8_t key_down(uint16_t port, uint8_t mask);

#endif
