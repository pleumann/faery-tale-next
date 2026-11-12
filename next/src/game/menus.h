/* Option menus (banked, PAGE_MENU) - resident wrappers in glue.c */
#ifndef GAME_MENUS_H
#define GAME_MENUS_H

#include <stdint.h>

struct inv_item {
    uint8_t image_number;       /* what image number to use */
    uint8_t xoff, yoff;         /* x and y offset on image screen */
    uint8_t ydelta;             /* y increment value */
    uint8_t img_off, img_height;/* what part of the image to draw */
    uint8_t maxshown;           /* maximum number that can be shown */
    const char *name;
};

void menu_key(uint8_t key);
void menu_click(uint8_t slot);  /* left click on menu slot 0..11 */
void print_options(void);
void set_options(void);
uint8_t paused(void);
void menus_start(void);
void show_inventory(void);      /* Items/List: the inventory picture on the playfield */
void get_turtle(void);
uint8_t setfig_can_talk(uint8_t id);

#endif
