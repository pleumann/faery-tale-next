#include <z80.h>
#include "input.h"

uint8_t key_down(uint16_t port, uint8_t mask)
{
    return (z80_inp(port) & mask) ? 0 : 1;
}

#define CAPS() key_down(0xFEFE, 0x01)
#define SYM()  key_down(0x7FFE, 0x02)

uint8_t input_dirs(void)
{
    uint8_t d = 0;
    uint8_t joy = z80_inp(0x001F);

    if (joy == 0xFF)                    /* no joystick interface */
        joy = 0;
    if (joy & 0x08) d |= IN_UP;
    if (joy & 0x04) d |= IN_DOWN;
    if (joy & 0x02) d |= IN_LEFT;
    if (joy & 0x01) d |= IN_RIGHT;
    if (CAPS()) {
        if (key_down(0xEFFE, 0x08)) d |= IN_UP;     /* 7 */
        if (key_down(0xEFFE, 0x10)) d |= IN_DOWN;   /* 6 */
        if (key_down(0xF7FE, 0x10)) d |= IN_LEFT;   /* 5 */
        if (key_down(0xEFFE, 0x04)) d |= IN_RIGHT;  /* 8 */
    }
    else if (key_down(0xEFFE, 0x01)) d |= IN_FIRE;  /* 0 */
    if (joy & 0x10) d |= IN_FIRE;
    return d;
}

/* keyboard matrix: 8 half rows, 5 keys each (bit 0 first) */
static const uint16_t rows[8] = { 0xFEFE, 0xFDFE, 0xFBFE, 0xF7FE, 0xEFFE, 0xDFFE, 0xBFFE, 0x7FFE };
static const char keymap[8][5] = {
    {  0,  'Z', 'X', 'C', 'V' },
    { 'A', 'S', 'D', 'F', 'G' },
    { 'Q', 'W', 'E', 'R', 'T' },
    { '1', '2', '3', '4', '5' },
    { '0', '9', '8', '7', '6' },
    { 'P', 'O', 'I', 'U', 'Y' },
    { 13,  'L', 'K', 'J', 'H' },
    { ' ',  0,  'M', 'N', 'B' },
};

static uint8_t last[8];

uint8_t input_key(void)
{
    uint8_t r, b, now, pressed, result = 0;
    uint8_t caps = CAPS(), sym = SYM();

    for (r = 0; r < 8; r++) {
        now = ~z80_inp(rows[r]) & 0x1F;
        pressed = now & ~last[r];
        last[r] = now;
        if (!pressed || result || caps)
            continue;
        for (b = 0; b < 5; b++) {
            if (pressed & (1 << b)) {
                char c = keymap[r][b];
                if (!c) continue;
                if (sym) {
                    if (c >= '1' && c <= '7') { result = 10 + (c - '1'); break; }
                    if (c == 'Z') { result = '?'; break; }      /* SYMBOL SHIFT + Z = ':' -> look */
                    continue;
                }
                if (c == '0') continue;                         /* fight key, see input_dirs */
                result = c;
                break;
            }
        }
    }
    return result;
}

/* Kempston mouse: X 0xFBDF, Y 0xFFDF (counts up when moving up), buttons 0xFADF
   (bit 1 left, bit 0 right, low active). Absent mouse reads 0xFF everywhere. */
uint16_t mouse_x = 160;
uint8_t mouse_y = 200, mouse_buttons, mouse_present;
static uint8_t last_x, last_y, mouse_init;

void mouse_update(void)
{
    uint8_t x = z80_inp(0xFBDF), y = z80_inp(0xFFDF), b = z80_inp(0xFADF);
    int16_t nx, ny;
    if (!mouse_init) { last_x = x; last_y = y; mouse_init = 1; return; }
    if (x != last_x || y != last_y) mouse_present = 1;
    nx = (int16_t)mouse_x + (int8_t)(x - last_x);
    ny = (int16_t)mouse_y - (int8_t)(y - last_y);
    last_x = x; last_y = y;
    if (nx < 0) nx = 0;
    if (nx > 319) nx = 319;
    if (ny < 28) ny = 28;
    if (ny > 231) ny = 231;
    mouse_x = (uint16_t)nx;
    mouse_y = (uint8_t)ny;
    mouse_buttons = (b & 0x02 ? 0 : MOUSE_LEFT) | (b & 0x01 ? 0 : MOUSE_RIGHT);
    if (b != 0xFF && mouse_buttons) mouse_present = 1;
}
