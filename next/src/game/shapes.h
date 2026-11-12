/* Shape sets in RAM (seq_list in fmain.c, read_shapes/shape_read in fmain2.c). */
#ifndef GAME_SHAPES_H
#define GAME_SHAPES_H

#include <stdint.h>

struct seq_info {
    uint8_t page;               /* first 8K page */
    uint16_t base_off;          /* offset of frame 0 in that page (2 MB build: in place) */
    uint8_t frames;
    uint8_t wcells, hcells;     /* size in 16x16 cells */
    uint8_t height;             /* real height in pixels */
    uint16_t frame_bytes;       /* wcells * hcells * 256 */
};

extern struct seq_info seq_list[7];

void shapes_init(void);                 /* read the SHAPES.DAT index */
void read_shapes(uint8_t cfile);        /* load cfile entry into its seq_list slot */
void shape_read(void);                  /* brother, objects, raft, actor_file, set_file */
uint8_t cfile_slot(uint8_t cfile);

#endif
