#include <stdint.h>
#include "../platform/hw.h"
#include "../platform/files.h"
#include "assets.h"
#include "world.h"

uint16_t map_x, map_y;
uint8_t region_num = NO_REGION, new_region = NO_REGION;
uint16_t xreg, yreg;

/* fmain.c:615 - struct need: image[4] (disk blocks), terra1, terra2, sector, region */
struct need {
    uint16_t image[4];
    uint8_t terra1, terra2;
    uint8_t sector, region;
};

static const struct need file_index[10] = {
    { { 320, 480, 520, 560 },  0, 1, 32, 160 },   /* F1 - snowy region */
    { { 320, 360, 400, 440 },  2, 3, 32, 160 },   /* F2 - witch wood */
    { { 320, 360, 520, 560 },  2, 1, 32, 168 },   /* F3 - swampy region */
    { { 320, 360, 400, 440 },  2, 3, 32, 168 },   /* F4 - plains and rocks */
    { { 320, 480, 520, 600 },  0, 4, 32, 176 },   /* F5 - desert area */
    { { 320, 280, 240, 200 },  5, 6, 32, 176 },   /* F6 - bay / city / farms */
    { { 320, 640, 520, 600 },  7, 4, 32, 184 },   /* F7 - volcanic */
    { { 320, 280, 240, 200 },  5, 6, 32, 184 },   /* F8 - forest and wilderness */
    { { 680, 720, 800, 840 },  8, 9, 96, 192 },   /* F9  - inside of buildings */
    { { 680, 760, 800, 840 }, 10, 9, 96, 192 },   /* F10 - dungeons and caves */
};

static struct need current_loads;

#define SLOT1 ((uint8_t *)0x2000)

void world_init(void)
{
    uint8_t i;
    for (i = 0; i < 4; i++)
        current_loads.image[i] = 0;
    current_loads.terra1 = current_loads.terra2 = 0xFF;
    current_loads.sector = current_loads.region = 0;
}

void load_new_region(void)
{
    const struct need *nd;
    uint8_t h, i;

    if (new_region >= NO_REGION)
        return;
    nd = &file_index[new_region];

    if (nd->sector != current_loads.sector) {
        h = file_open("FTA/SECTORS.DAT");
        file_seek(h, nd->sector == 32 ? SECTORS_OUTDOOR : SECTORS_INDOOR);
        file_read_pages(h, SECTOR_PAGE, 32768UL);
        file_close(h);
        current_loads.sector = nd->sector;
    }
    if (nd->region != current_loads.region) {
        h = file_open("FTA/MAPS.DAT");
        file_seek(h, (uint32_t)((nd->region - 160) / 8) * MAP_BYTES);
        file_read_at(h, MAP_PAGE, 0, MAP_BYTES);
        file_close(h);
        current_loads.region = nd->region;
    }
    if (nd->terra1 != current_loads.terra1 || nd->terra2 != current_loads.terra2) {
        h = file_open("FTA/TERRA.DAT");
        file_seek(h, (uint32_t)nd->terra1 * TERRA_BYTES);
        file_read_at(h, MAP_PAGE, MAP_TERRA_OFF, TERRA_BYTES);
        file_seek(h, (uint32_t)nd->terra2 * TERRA_BYTES);
        file_read_at(h, MAP_PAGE, MAP_TERRA_OFF + TERRA_BYTES, TERRA_BYTES);
        file_close(h);
        current_loads.terra1 = nd->terra1;
        current_loads.terra2 = nd->terra2;
    }
    h = 0xFF;
    for (i = 0; i < 4; i++) {
        if (nd->image[i] != current_loads.image[i]) {
            if (h == 0xFF)
                h = file_open("FTA/TILES.DAT");
            file_seek(h, (uint32_t)((nd->image[i] - TILE_BANK_FIRST_BLOCK) / 40) * TILE_BANK_BYTES);
            file_read_pages(h, TILE_PAGE + i * 4, TILE_BANK_BYTES);
            current_loads.image[i] = nd->image[i];
        }
    }
    if (h != 0xFF)
        file_close(h);

    /* TODO (M4): desert gate patch, fmain.c:3594, needs stuff[STATBASE] */

    region_num = new_region;
    new_region = NO_REGION;
}

/* fmain.c gen_mini(): pick the region from the screen centre */
void gen_mini(void)
{
    uint8_t lregion, xr, yr;
    uint16_t xs, ys;

    if (region_num < 8) {
        xs = (map_x + 151) >> 8;
        ys = (map_y + 64) >> 8;
        xr = (xs >> 6) & 1;
        yr = (ys >> 5) & 3;
        lregion = xr + yr + yr;
        if (lregion != region_num) {
            new_region = lregion;
            load_new_region();
        }
    } else {
        lregion = region_num;
    }
    yr = lregion >> 1;
    xr = (lregion > 7) ? 0 : (lregion & 1);
    xreg = (uint16_t)xr << 6;
    yreg = (uint16_t)yr << 5;
}

/* Sector number for tile coordinates, clamped to the loaded region like _genmini. */
static uint8_t sector_at(uint16_t imx, uint16_t imy)
{
    int16_t xs, ys;

    xs = (int16_t)(imx >> 4) - (int16_t)xreg;
    if (xs & 64)
        xs = (xs & 32) ? 0 : 63;
    xs += xreg;
    ys = (int16_t)(imy >> 3) - (int16_t)yreg;
    if (ys < 0) ys = 0;
    if (ys > 31) ys = 31;
    mmu_set(1, MAP_PAGE);
    return SLOT1[((uint16_t)ys << 7) + (uint16_t)xs];
}

static uint8_t sector_byte(uint8_t sec, uint16_t imx, uint16_t imy)
{
    uint16_t off = ((uint16_t)sec << 7) + ((imy & 7) << 4) + (imx & 15);
    mmu_set(1, SECTOR_PAGE + (off >> 13));
    return SLOT1[off & 0x1FFF];
}

uint8_t world_tile(uint16_t imx, uint16_t imy)
{
    uint8_t t;
    imx &= 0x07FF;
    imy &= 0x7FFF;
    t = sector_byte(sector_at(imx, imy), imx, imy);
    mmu_set(1, 0xFF);
    return t;
}

uint8_t px_to_im(uint16_t x, uint16_t y)
{
    uint8_t bit = 0x80, t, r = 0;
    uint16_t imx, imy;
    const uint8_t *terra;

    if (x & 8) bit >>= 4;
    if (y & 8) bit >>= 1;
    if (y & 16) bit >>= 2;
    imx = x >> 4;
    imy = y >> 5;
    t = sector_byte(sector_at(imx, imy), imx, imy);
    mmu_set(1, MAP_PAGE);
    terra = SLOT1 + MAP_TERRA_OFF + ((uint16_t)t << 2);
    if (terra[2] & bit)
        r = terra[1] >> 4;
    mmu_set(1, 0xFF);
    return r;
}

/* fsubs.asm _map_adjust: dead zone +-20/+-10, snap beyond +-70 / -24..+44 */
void map_adjust(uint16_t x, uint16_t y)
{
    uint16_t tx = x - 144, ty = y - 70;
    int16_t dx = (int16_t)((uint16_t)(tx - map_x) << 1) >> 1;
    int16_t dy = (int16_t)((uint16_t)(ty - map_y) << 1) >> 1;

    if (dx < -70) map_x = tx + 70;
    else if (dx > 70) map_x = tx - 70;
    else if (dx < -20) map_x--;
    else if (dx > 20) map_x++;

    if (dy < -24) map_y = ty + 24;
    else if (dy > 44) map_y = ty - 44;
    else if (dy < -10) map_y--;
    else if (dy > 10) map_y++;
}
