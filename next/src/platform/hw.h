/* Low-level Spectrum Next hardware access. */
#ifndef HW_H
#define HW_H

#include <stdint.h>

/* Next registers used by the port */
#define NR_CPU_SPEED        0x07
#define NR_L2_ACTIVE_BANK   0x12
#define NR_L2_SHADOW_BANK   0x13
#define NR_GLOBAL_TRANSP    0x14
#define NR_SPRITE_LAYER     0x15
#define NR_CLIP_L2          0x18
#define NR_CLIP_CTRL        0x1C
#define NR_PAL_INDEX        0x40
#define NR_PAL_VALUE8       0x41
#define NR_PAL_CTRL         0x43
#define NR_PAL_VALUE9       0x44
#define NR_FALLBACK         0x4A
#define NR_MMU0             0x50
#define NR_ULA_CTRL         0x68
#define NR_L2_CTRL          0x70
#define NR_L2_XSCROLL_LSB   0x16
#define NR_L2_YSCROLL       0x17
#define NR_L2_XSCROLL_MSB   0x71

/* Physical 8K pages: Layer 2 320x256 needs 10 pages per buffer */
#define L2_BANK_A           9       /* 16K bank -> pages 18..27 */
#define L2_BANK_B           14      /* 16K bank -> pages 28..37 */
#define L2_PAGE_A           (L2_BANK_A * 2)
#define L2_PAGE_B           (L2_BANK_B * 2)

/* MMU slot 2 (0x4000-0x5FFF) is the general data window */
#define WINDOW_SLOT         2
#define WINDOW_ADDR         ((uint8_t *)0x4000)

/* nesting critical sections (interrupts off) - see hw.c */
extern uint8_t crit;
void crit_enter(void);
void crit_leave(void);

void nextreg(uint8_t reg, uint8_t val);
uint8_t nextreg_read(uint8_t reg);
void mmu_set(uint8_t slot, uint8_t page);

void irq_init(void);
void wait_frame(void);
extern volatile uint16_t frames;

#endif
