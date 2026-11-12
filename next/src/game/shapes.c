#include <stdint.h>
#include <string.h>
#include "../platform/hw.h"
#include "../platform/files.h"
#include "assets.h"
#include "actors.h"
#include "state.h"
#include "shapes.h"

/* SHAPES.DAT index entry (see tools/convert.py) */
struct set_info {
    uint32_t offset;
    uint8_t frames, wcells, hcells, height;
};

static struct set_info sets[SHAPE_SETS];

struct seq_info seq_list[7];

/* fmain2.c:643 cfiles[].seq_num */
static const uint8_t cfile_slots[SHAPE_SETS] = {
    PHIL, PHIL, PHIL, OBJECTS, RAFT, CARRIER, ENEMY, ENEMY, ENEMY, ENEMY,
    DRAGON, CARRIER, ENEMY, SETFIG, SETFIG, SETFIG, SETFIG, SETFIG
};

/* physical pages per slot: PHIL, OBJECTS, ENEMY, RAFT, SETFIG, CARRIER, DRAGON */
static const uint8_t slot_page[7] = { 64, 28, 32, 72, 71, 36, 69 };

uint8_t cfile_slot(uint8_t cfile)
{
    return cfile_slots[cfile];
}

void shapes_init(void)
{
    uint8_t h = file_open("FTA/SHAPES.DAT");
    if (h == FILE_ERROR)
        return;
    file_seek(h, 2);
    file_read(h, sets, sizeof(sets));
    file_close(h);
}

void read_shapes(uint8_t cfile)
{
    struct set_info *s = &sets[cfile];
    struct seq_info *q = &seq_list[cfile_slots[cfile]];
    uint8_t h;

    q->page = slot_page[cfile_slots[cfile]];
    q->frames = s->frames;
    q->wcells = s->wcells;
    q->hcells = s->hcells;
    q->height = s->height;
    q->frame_bytes = (uint16_t)s->wcells * s->hcells * 256;
    q->base_off = 0;
#ifdef MEMFS
    /* 2 MB build: use the frames where they are */
    (void)h;
    memfs_locate("FTA/SHAPES.DAT", s->offset, &q->page, &q->base_off);
    return;
#endif
    h = file_open("FTA/SHAPES.DAT");
    if (h == FILE_ERROR)
        return;
    file_seek(h, s->offset);
    file_read_pages(h, q->page, (uint32_t)q->frame_bytes * s->frames);
    file_close(h);
}

/* fmain2.c:673 */
void shape_read(void)
{
    read_shapes(3);
    read_shapes(brother - 1);
    read_shapes(4);
    read_shapes(actor_file);
    read_shapes(set_file);
}
