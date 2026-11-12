/* Save and load (fmain2.c savegame/saveload, fmain.c mod1save) - resident, because the
   file calls need the data outside slot 0. Own format: the Amiga version wrote raw
   memory blocks whose layout depended on the compiler. Files FTA/SAVEA.DAT .. SAVEH.DAT */

#include <stdint.h>
#include "../platform/files.h"
#include "../platform/bank.h"
#include "world.h"
#include "state.h"
#include "actors.h"
#include "shapes.h"
#include "banked.h"
#include "ui.h"
#include "menus.h"
#include "view.h"

#define SAVE_MAGIC  0x46544131UL    /* "FTA1" */

uint8_t objects_io_b(uint8_t h, uint8_t save);

struct item { void *p; uint16_t len; };

/* everything that makes up the game state */
static const struct item items[] = {
    { &map_x, 2 }, { &map_y, 2 }, { &hero_x, 2 }, { &hero_y, 2 },
    { &safe_x, 2 }, { &safe_y, 2 }, { &safe_r, 2 },
    { &riding, 2 }, { &flying, 2 }, { &wcarry, 2 }, { &turtleprox, 2 }, { &raftprox, 2 },
    { &brave, 2 }, { &luck, 2 }, { &kind, 2 }, { &wealth, 2 }, { &hunger, 2 }, { &fatigue, 2 },
    { &brother, 2 }, { &princess, 2 }, { &hero_sector, 2 }, { &hero_place, 2 },
    { &daynight, 2 }, { &lightlevel, 2 }, { &actor_file, 2 }, { &set_file, 2 },
    { &active_carrier, 2 }, { &xtype, 2 }, { &leader, 2 },
    { &secret_timer, 2 }, { &light_timer, 2 }, { &freeze_timer, 2 },
    { &encounter_type, 2 }, { &dayperiod, 2 },
    { &region_num, 1 }, { &anix, 1 }, { &anix2, 1 }, { &mdex, 1 },
    { anim_list, sizeof(anim_list) },
    { julstuff, STUFF_SIZE }, { philstuff, STUFF_SIZE }, { kevstuff, STUFF_SIZE },
    { missile_list, sizeof(missile_list) },
    { extent_list, 2 * sizeof(struct extent) },
};

#ifdef MEMFS
static char savename[] = "FTASAVEA.DAT";       /* next to the single fta.nex */
#define SLOT_CHAR 7
#else
static char savename[] = "FTA/SAVEA.DAT";
#define SLOT_CHAR 8
#endif

/* slot 0..7 = A..H; save = 1 to save, 0 to load */
void savegame(uint8_t slot, uint8_t save)
{
    uint32_t magic = SAVE_MAGIC;
    uint8_t h, i, ok;

    savename[SLOT_CHAR] = 'A' + slot;
    h = save ? file_create(savename) : file_open(savename);
    ok = h != FILE_ERROR;
    if (ok) {
        if (save) ok = file_write(h, &magic, 4) == 4;
        else ok = file_read(h, &magic, 4) == 4 && magic == SAVE_MAGIC;
        for (i = 0; ok && i < sizeof(items) / sizeof(items[0]); i++) {
            if (save) ok = file_write(h, items[i].p, items[i].len) == items[i].len;
            else ok = file_read(h, items[i].p, items[i].len) == items[i].len;
        }
        if (ok) {
            BANK_ENTER(PAGE_OBJECTS);
            ok = objects_io_b(h, save);
            BANK_LEAVE();
        }
        file_close(h);
    }
    if (!ok) {
        print(save ? "ERROR: Couldn't save game." : "ERROR: Couldn't load game.");
        return;
    }
    if (save) {
        print("Game saved.");
        return;
    }
    /* after loading (fmain2.c:1539) */
    stuff = brother == 1 ? julstuff : brother == 2 ? philstuff : kevstuff;
    wt = encounter_number = 0;
    shape_read();
    new_region = region_num;
    region_num = NO_REGION;
    load_all();
    gen_mini();
    set_options();
    viewstatus = 99;
    prq(4); prq(7);
    print(""); print(""); print("");
    encounter_type = actors_loading = 0;
    setmood(TRUE);
}
