#include <arch/zxn.h>
#include <intrinsic.h>
#include "hw.h"

/* Next register writes are two port accesses (select, data). The frame interrupt also
   uses Next registers (the music player maps pages), so every access is a critical
   section. The counter makes them nest: only the outermost one enables interrupts. */
uint8_t crit;

void crit_enter(void)
{
    intrinsic_di();
    crit++;
}

void crit_leave(void)
{
    if (!--crit)
        intrinsic_ei();
}

void nextreg(uint8_t reg, uint8_t val)
{
    crit_enter();
    IO_NEXTREG_REG = reg;
    IO_NEXTREG_DAT = val;
    crit_leave();
}

uint8_t nextreg_read(uint8_t reg)
{
    uint8_t v;
    crit_enter();
    IO_NEXTREG_REG = reg;
    v = IO_NEXTREG_DAT;
    crit_leave();
    return v;
}

void mmu_set(uint8_t slot, uint8_t page)
{
    crit_enter();
    IO_NEXTREG_REG = NR_MMU0 + slot;
    IO_NEXTREG_DAT = page;
    crit_leave();
}
