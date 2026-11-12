#include <arch/zxn.h>
#include "hw.h"

void nextreg(uint8_t reg, uint8_t val)
{
    IO_NEXTREG_REG = reg;
    IO_NEXTREG_DAT = val;
}

uint8_t nextreg_read(uint8_t reg)
{
    IO_NEXTREG_REG = reg;
    return IO_NEXTREG_DAT;
}

void mmu_set(uint8_t slot, uint8_t page)
{
    IO_NEXTREG_REG = NR_MMU0 + slot;
    IO_NEXTREG_DAT = page;
}
