#include <z80.h>
#include "input.h"

uint8_t key_down(uint16_t port, uint8_t mask)
{
    return (z80_inp(port) & mask) ? 0 : 1;
}

uint8_t input_dirs(void)
{
    uint8_t d = 0;
    uint8_t joy = z80_inp(0x001F);

    if (joy == 0xFF)                    /* no joystick interface */
        joy = 0;
    if (joy & 0x08 || key_down(0xFBFE, 0x01) || key_down(0xEFFE, 0x08)) d |= IN_UP;     /* Q, 7 */
    if (joy & 0x04 || key_down(0xFDFE, 0x01) || key_down(0xEFFE, 0x10)) d |= IN_DOWN;   /* A, 6 */
    if (joy & 0x02 || key_down(0xDFFE, 0x02) || key_down(0xF7FE, 0x10)) d |= IN_LEFT;   /* O, 5 */
    if (joy & 0x01 || key_down(0xDFFE, 0x01) || key_down(0xEFFE, 0x04)) d |= IN_RIGHT;  /* P, 8 */
    if (joy & 0x10 || key_down(0x7FFE, 0x01)) d |= IN_FIRE;                            /* SPACE */
    return d;
}
