#include <arch/zxn/esxdos.h>
#include <errno.h>
#include "hw.h"
#include "files.h"

#define DEFAULT_WINDOW_PAGE 10      /* bank 5 low half, normally at 0x4000 */

uint32_t file_load_pages(const char *name, uint8_t first_page)
{
    uint32_t total = 0;
    uint16_t n;
    uint8_t h;

    errno = 0;
    h = esx_f_open(name, ESX_MODE_R | ESX_MODE_OPEN_EXIST);
    if (errno)
        return 0;
    do {
        mmu_set(WINDOW_SLOT, first_page++);
        n = esx_f_read(h, WINDOW_ADDR, 8192);
        total += n;
    } while (n == 8192 && !errno);
    mmu_set(WINDOW_SLOT, DEFAULT_WINDOW_PAGE);
    esx_f_close(h);
    return total;
}

uint16_t file_load(const char *name, void *dst, uint16_t len)
{
    uint16_t n;
    uint8_t h;

    errno = 0;
    h = esx_f_open(name, ESX_MODE_R | ESX_MODE_OPEN_EXIST);
    if (errno)
        return 0;
    n = esx_f_read(h, dst, len);
    esx_f_close(h);
    return n;
}
