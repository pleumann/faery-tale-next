/* World map access: regions, sectors, tiles, terrain (fmain.c load_new_region/gen_mini,
   fsubs.asm _genmini/_px_to_im). */
#ifndef GAME_WORLD_H
#define GAME_WORLD_H

#include <stdint.h>

/* Physical 8K pages holding the current region's data */
#define TILE_PAGE       40      /* 256 tiles x 512 bytes = 16 pages (40..55) */
#define SECTOR_PAGE     56      /* 32K sector data = 4 pages (56..59) */
#define MAP_PAGE        60      /* 4K region map at 0, terrain (2 x 512) at 4096 */
#define MAP_TERRA_OFF   4096

#define NO_REGION       10

extern uint16_t map_x, map_y;   /* world pixel of the playfield's top-left corner */
extern uint8_t region_num, new_region;
extern uint16_t xreg, yreg;     /* region origin in sectors */

void world_init(void);
void load_new_region(void);     /* loads new_region if set; returns with ROM in slot 0/1 */
void gen_mini(void);            /* choose region from map position, load if changed */

/* Tile number at tile coordinates (imx = x/16, imy = y/32) - like _genmini */
uint8_t world_tile(uint16_t imx, uint16_t imy);

/* Byte idx (0..3) of the terrain entry of a tile: 0 shadow mask number, 1 type/mask mode,
   2 solidity bits, 3 overview map colour */
uint8_t terra_byte(uint8_t tile, uint8_t idx);

/* Sector number the hero stands in (+256 indoors) - end of _genmini */
uint16_t world_hero_sector(uint16_t x, uint16_t y);
extern uint16_t hero_sector;

/* Terrain type at world pixel (x, y), 0 if not solid there - like _px_to_im */
uint8_t px_to_im(uint16_t x, uint16_t y);

/* Camera follows (x, y) with the original dead zone - like _map_adjust */
void map_adjust(uint16_t x, uint16_t y);

#endif
