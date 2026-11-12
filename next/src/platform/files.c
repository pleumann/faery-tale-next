#include <arch/zxn/esxdos.h>
#include <errno.h>
#include "hw.h"
#include "files.h"
#include "bank.h"

uint8_t cur_bank = ROM_PAGE;

/* esxDOS is reached through RST 8 in the ROM: banked code in slot 0 must step aside. */
#define ROM_IN()   mmu_set(0, ROM_PAGE)
#define ROM_OUT()  mmu_set(0, cur_bank)

#define DEFAULT_WINDOW_PAGE 10      /* bank 5 low half, normally at 0x4000 */

uint8_t file_open(const char *name)
{
    uint8_t h;
    errno = 0;
    ROM_IN();
    h = esx_f_open(name, ESX_MODE_R | ESX_MODE_OPEN_EXIST);
    ROM_OUT();
    return errno ? FILE_ERROR : h;
}

void file_close(uint8_t h)
{
    ROM_IN();
    esx_f_close(h);
    ROM_OUT();
}

uint8_t file_seek(uint8_t h, uint32_t pos)
{
    errno = 0;
    ROM_IN();
    esx_f_seek(h, pos, ESX_SEEK_SET);
    ROM_OUT();
    return errno;
}

uint32_t file_read_pages(uint8_t h, uint8_t first_page, uint32_t len)
{
    uint32_t total = 0;
    uint16_t want, n;

    errno = 0;
    while (len) {
        want = len > 8192 ? 8192 : (uint16_t)len;
        mmu_set(WINDOW_SLOT, first_page++);
        ROM_IN();
        n = esx_f_read(h, WINDOW_ADDR, want);
        ROM_OUT();
        total += n;
        len -= n;
        if (n != want || errno)
            break;
    }
    mmu_set(WINDOW_SLOT, DEFAULT_WINDOW_PAGE);
    return total;
}

uint16_t file_read_at(uint8_t h, uint8_t page, uint16_t offset, uint16_t len)
{
    uint16_t n;
    errno = 0;
    mmu_set(WINDOW_SLOT, page);
    ROM_IN();
    n = esx_f_read(h, WINDOW_ADDR + offset, len);
    ROM_OUT();
    mmu_set(WINDOW_SLOT, DEFAULT_WINDOW_PAGE);
    return n;
}

uint32_t file_load_pages(const char *name, uint8_t first_page)
{
    uint32_t n;
    uint8_t h = file_open(name);
    if (h == FILE_ERROR)
        return 0;
    n = file_read_pages(h, first_page, 0xFFFFFFFFUL);
    file_close(h);
    return n;
}

uint16_t file_load(const char *name, void *dst, uint16_t len)
{
    uint16_t n;
    uint8_t h = file_open(name);
    if (h == FILE_ERROR)
        return 0;
    ROM_IN();
    n = esx_f_read(h, dst, len);
    ROM_OUT();
    file_close(h);
    return n;
}
