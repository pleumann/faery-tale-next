/* Status panel and text output: the Amiga's 640x57 hires panel as an 80-column tilemap.
   Text uses the Spectrum ROM font, one character per 8x8 tile, coloured through the
   tilemap palette offsets. Replaces print/print_cont/extract/msg/prdec/ppick/drawcompass
   of fmain2.c and fsubs.asm. Resident, because banked modules pass their own strings. */
#ifndef GAME_UI_H
#define GAME_UI_H

#include <stdint.h>

void ui_font_from_rom(void);    /* call while the ROM is mapped at 0x0000 */
void ui_init(void);

void print(const char *s);      /* scroll the message area, print on a new line */
void print_cont(const char *s); /* continue the current line */
void prdec(uint16_t n, uint8_t len);
void extract(const char *s);    /* word wrap to 37 columns, % = brother's name */
void event(uint8_t n);
void speak(uint8_t n);
void msg_place(uint8_t inside, uint8_t n);
void question(uint8_t n);
void name(void);
void announce_container(const char *s);
void announce_treasure(const char *s);

void prq(uint8_t n);            /* status updates: 4 vitality, 5 options, 7 stats, 10 take what */
void drawcompass(uint8_t dir);  /* 0..7, 9 = none */
uint8_t place_lookup(uint8_t sector, uint8_t inside);

/* menu text: 6 cells at a menu slot (0..11), fg pen 0/1, bg pen 0..15 */
void ui_menu_text(uint8_t slot, const char *label, uint8_t len, uint8_t fg, uint8_t bg);
void ui_menu_reset(void);
void ui_panel(uint8_t on);

extern const char *const datanames[3];

#endif
