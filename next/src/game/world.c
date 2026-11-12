#include <stdint.h>
#include "../platform/hw.h"
#include "../platform/files.h"
#include "assets.h"
#include "world.h"
#include "state.h"
#include "actors.h"

uint16_t map_x, map_y;
uint8_t region_num = NO_REGION, new_region = NO_REGION;
uint16_t xreg, yreg;
uint16_t hero_sector;

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

static void cache_flush(void);

void world_init(void)
{
    uint8_t i;
    cache_flush();
    for (i = 0; i < 4; i++)
        current_loads.image[i] = 0;
    current_loads.terra1 = current_loads.terra2 = 0xFF;
    current_loads.sector = current_loads.region = 0;
}

/* ---- tile bank cache ----
   8 slots of 32K (4 pages each) for the 17 tile banks; a region needs 4 of them. Banks of
   the neighbouring region are loaded ahead in 2K pieces while the hero walks towards it,
   so crossing a region border only loads the small map/terrain data. */

#define CACHE_SLOTS 8
#define NO_BANK     0xFF

static const uint8_t slot_pages[CACHE_SLOTS][4] = {
    { 40, 41, 42, 43 }, { 44, 45, 46, 47 }, { 48, 49, 50, 51 }, { 52, 53, 54, 55 },
    {  6,  7,  8,  9 }, { 90, 91, 92, 93 }, {  2,  3, 12, 13 }, { 16, 17, 94, 95 } };
static uint8_t slot_bank[CACHE_SLOTS] = { NO_BANK, NO_BANK, NO_BANK, NO_BANK,
                                          NO_BANK, NO_BANK, NO_BANK, NO_BANK };
uint8_t tile_pages[16];                 /* page of each group of 16 tiles of the region */

static uint8_t pf_bank = NO_BANK, pf_slot, pf_h = FILE_ERROR;
static uint16_t pf_off;

static void cache_flush(void)
{
    uint8_t s;
    for (s = 0; s < CACHE_SLOTS; s++) slot_bank[s] = NO_BANK;
    pf_bank = NO_BANK;
}

static uint8_t bank_of(uint16_t block) { return (uint8_t)((block - TILE_BANK_FIRST_BLOCK) / 40); }

static uint8_t find_slot(uint8_t bank)
{
    uint8_t s;
    for (s = 0; s < CACHE_SLOTS; s++)
        if (slot_bank[s] == bank) return s;
    return NO_BANK;
}

/* a slot that holds none of the banks in `keep` (4 bank numbers, NO_BANK allowed) */
static uint8_t free_slot(const uint8_t *keep, const uint8_t *keep2)
{
    uint8_t s, k, ok;
    for (s = 0; s < CACHE_SLOTS; s++) {
        if (s == pf_slot && pf_bank != NO_BANK) continue;
        ok = 1;
        for (k = 0; k < 4; k++)
            if (slot_bank[s] != NO_BANK && (slot_bank[s] == keep[k] || (keep2 && slot_bank[s] == keep2[k])))
                ok = 0;
        if (ok) return s;
    }
    return 0;
}

static void pf_chunk(void)
{
    if (pf_h == FILE_ERROR) pf_h = file_open("FTA/TILES.DAT");
    file_seek(pf_h, (uint32_t)pf_bank * TILE_BANK_BYTES + pf_off);
    file_read_at(pf_h, slot_pages[pf_slot][pf_off >> 13], pf_off & 0x1FFF, 2048);
    pf_off += 2048;
    if (pf_off == 32768U) {                 /* the whole bank is in */
        slot_bank[pf_slot] = pf_bank;
        pf_bank = NO_BANK;
    }
}

static void region_banks(const struct need *nd, uint8_t *b)
{
    uint8_t i;
    for (i = 0; i < 4; i++) b[i] = bank_of(nd->image[i]);
}

/* make the 4 banks of a region resident and point tile_pages at them */
static void tiles_for_region(const struct need *nd)
{
    uint8_t want[4], i, k, s;
    region_banks(nd, want);
    for (i = 0; i < 4; i++) {
        s = find_slot(want[i]);
        if (s == NO_BANK) {
            if (pf_bank == want[i]) {       /* being loaded ahead: finish it now */
                while (pf_bank != NO_BANK) pf_chunk();
                s = find_slot(want[i]);
            } else {
                uint8_t h;
                s = free_slot(want, 0);
                slot_bank[s] = NO_BANK;
                h = file_open("FTA/TILES.DAT");
                file_seek(h, (uint32_t)want[i] * TILE_BANK_BYTES);
                for (k = 0; k < 4; k++)
                    file_read_pages(h, slot_pages[s][k], 8192);
                file_close(h);
                slot_bank[s] = want[i];
            }
        }
        for (k = 0; k < 4; k++)
            tile_pages[i * 4 + k] = slot_pages[s][k];
    }
}

/* Called once per frame: load 2K of a tile bank the neighbouring region will need. */
void world_prefetch(void)
{
    uint8_t xs, ys, xr, yr, target, want[4], cur[4], i;

    if (region_num > 7 || new_region < NO_REGION) return;
    if (pf_bank != NO_BANK) { pf_chunk(); return; }

    xs = hero_x >> 8;
    ys = hero_y >> 8;
    xr = (xs >> 6) & 1;
    yr = (ys >> 5) & 3;
    if ((xs & 63) > 52 || (xs & 63) < 12) xr ^= 1;          /* near a side border */
    else if ((ys & 31) > 24 && yr < 3) yr++;                /* near the next band */
    else if ((ys & 31) < 8 && yr > 0) yr--;
    else return;
    target = xr + yr + yr;
    if (target == region_num) return;

    region_banks(&file_index[target], want);
    region_banks(&file_index[region_num], cur);
    for (i = 0; i < 4; i++) {
        if (find_slot(want[i]) == NO_BANK) {
            pf_slot = free_slot(cur, want);
            slot_bank[pf_slot] = NO_BANK;
            pf_bank = want[i];
            pf_off = 0;
            pf_chunk();
            return;
        }
    }
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
    for (i = 0; i < 4; i++)
        current_loads.image[i] = nd->image[i];
    tiles_for_region(nd);

    /* fmain.c:3594 - the desert gate stays closed until the hero has 5 statues */
    if (new_region == 4 && stuff[STATBASE] < 5) {
        uint16_t m = 11 * 128 + 26;
        mmu_set(1, MAP_PAGE);
        SLOT1[m] = SLOT1[m + 1] = SLOT1[m + 128] = SLOT1[m + 129] = 254;
        mmu_set(1, 0xFF);
    }

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

uint8_t terra_byte(uint8_t tile, uint8_t idx)
{
    uint8_t v;
    mmu_set(1, MAP_PAGE);
    v = SLOT1[MAP_TERRA_OFF + ((uint16_t)tile << 2) + idx];
    mmu_set(1, 0xFF);
    return v;
}

uint16_t world_hero_sector(uint16_t x, uint16_t y)
{
    uint16_t sec;
    mmu_set(1, MAP_PAGE);
    sec = SLOT1[(uint8_t)(x >> 8) + ((((y >> 8) - yreg) & 31) << 7)];
    mmu_set(1, 0xFF);
    if (region_num > 7)
        sec += 256;
    return sec;
}

/* fsubs.asm _mapxy: the tile byte at tile coordinates, readable and writable (doors) */
static uint16_t mapxy_off(uint16_t imx, uint16_t imy)
{
    int16_t xs, ys;
    uint8_t sec;
    xs = (int16_t)(imx >> 4) - (int16_t)xreg;
    if (xs & 64) xs = (xs & 32) ? 0 : 63;
    ys = (int16_t)(imy >> 3) - (int16_t)yreg;
    if (ys & 32) ys = (ys & 16) ? 0 : 31;
    mmu_set(1, MAP_PAGE);
    sec = SLOT1[((uint16_t)ys << 7) + (uint16_t)xs + xreg];
    return ((uint16_t)sec << 7) + ((imy & 7) << 4) + (imx & 15);
}

uint8_t mapxy_get(uint16_t imx, uint16_t imy)
{
    uint16_t off = mapxy_off(imx, imy);
    uint8_t v;
    mmu_set(1, SECTOR_PAGE + (off >> 13));
    v = SLOT1[off & 0x1FFF];
    mmu_set(1, 0xFF);
    return v;
}

void mapxy_set(uint16_t imx, uint16_t imy, uint8_t v)
{
    uint16_t off = mapxy_off(imx, imy);
    mmu_set(1, SECTOR_PAGE + (off >> 13));
    SLOT1[off & 0x1FFF] = v;
    mmu_set(1, 0xFF);
}

uint16_t world_image_block(uint8_t i)
{
    return current_loads.image[i];
}
