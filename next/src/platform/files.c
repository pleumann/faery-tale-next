#include <arch/zxn/esxdos.h>
#include <errno.h>
#include "hw.h"
#include "files.h"
#include "bank.h"

uint8_t cur_bank = ROM_PAGE;

/* esxDOS is reached through RST 8 in the ROM: banked code in slot 0 must step aside. */
uint8_t in_esx;                 /* the ISR must not remap memory during esxDOS calls */
#define ROM_IN()   do { in_esx = 1; mmu_set(0, ROM_PAGE); } while (0)
#define ROM_OUT()  do { mmu_set(0, cur_bank); in_esx = 0; } while (0)

#define DEFAULT_WINDOW_PAGE 10      /* bank 5 low half, normally at 0x4000 */

/* file names may live in banked code: copy them before the ROM is mapped in */
static char fname[32];

static const char *resident_name(const char *name)
{
    uint8_t i = 0;
    while (name[i] && i < sizeof(fname) - 1) { fname[i] = name[i]; i++; }
    fname[i] = 0;
    return fname;
}

#ifdef MEMFS
/* ---- 2 MB build: the data files live in RAM pages (tools/memfs.py) ---- */
#include <string.h>
#include "memfs.h"

#define MEMFS_HANDLES 4
static struct { uint8_t file; uint8_t used; uint32_t pos; } mh[MEMFS_HANDLES];
#define SLOT1 ((uint8_t *)0x2000)

static int8_t memfs_find(const char *name)
{
    uint8_t i;
    for (i = 0; i < MEMFS_COUNT; i++)
        if (!strcmp(memfs_files[i].name, name)) return i;
    return -1;
}

uint8_t memfs_locate(const char *name, uint32_t pos, uint8_t *page, uint16_t *off)
{
    int8_t f = memfs_find(name);
    uint32_t a;
    if (f < 0) return 0;
    a = (uint32_t)memfs_files[f].offset + pos;
    *page = memfs_files[f].page + (uint8_t)(a >> 13);
    *off = (uint16_t)(a & 0x1FFF);
    return 1;
}

/* copy len bytes of file f from pos to (dst_page, dst_off), or to dst_ptr if not NULL */
static uint32_t memfs_copy(uint8_t h, uint8_t dst_page, uint16_t dst_off, uint8_t *dst_ptr, uint32_t len)
{
    const struct memfs_file *mf = &memfs_files[mh[h].file];
    uint32_t done = 0;
    if (mh[h].pos >= mf->size) return 0;
    if (len > mf->size - mh[h].pos) len = mf->size - mh[h].pos;
    while (done < len) {
        uint32_t a = (uint32_t)mf->offset + mh[h].pos;
        uint16_t soff = (uint16_t)(a & 0x1FFF), n = 8192 - soff;
        if (dst_ptr == 0 && 8192 - dst_off < n) n = 8192 - dst_off;
        if (len - done < n) n = (uint16_t)(len - done);
        mmu_set(1, mf->page + (uint8_t)(a >> 13));
        if (dst_ptr) {
            memcpy(dst_ptr, SLOT1 + soff, n);
            dst_ptr += n;
        } else {
            mmu_set(WINDOW_SLOT, dst_page);
            memcpy(WINDOW_ADDR + dst_off, SLOT1 + soff, n);
            dst_off += n;
            if (dst_off == 8192) { dst_off = 0; dst_page++; }
        }
        mh[h].pos += n;
        done += n;
    }
    mmu_set(1, 0xFF);
    mmu_set(WINDOW_SLOT, 10);
    return done;
}
#define IS_MEM(h) ((h) & 0x80)
#define MH(h) ((h) & 0x7F)
#endif

uint8_t file_open(const char *name)
{
#ifdef MEMFS
    {
        int8_t f = memfs_find(name);
        uint8_t i;
        if (f >= 0) {
            for (i = 0; i < MEMFS_HANDLES; i++)
                if (!mh[i].used) {
                    mh[i].used = 1; mh[i].file = f; mh[i].pos = 0;
                    errno = 0;
                    return 0x80 | i;
                }
            return FILE_ERROR;
        }
    }
#endif
    {
    uint8_t h;
    errno = 0;
    name = resident_name(name);
    ROM_IN();
    h = esx_f_open(name, ESX_MODE_R | ESX_MODE_OPEN_EXIST);
    ROM_OUT();
    return errno ? FILE_ERROR : h;
    }
}

void file_close(uint8_t h)
{
#ifdef MEMFS
    if (IS_MEM(h)) { mh[MH(h)].used = 0; return; }
#endif
    ROM_IN();
    esx_f_close(h);
    ROM_OUT();
}

uint8_t file_seek(uint8_t h, uint32_t pos)
{
#ifdef MEMFS
    if (IS_MEM(h)) { mh[MH(h)].pos = pos; return 0; }
#endif
    errno = 0;
    ROM_IN();
    esx_f_seek(h, pos, ESX_SEEK_SET);
    ROM_OUT();
    return errno;
}

void (*file_idle_hook)(void);

/* Long reads go in 2K pieces; between them the idle hook keeps e.g. the music going. */
#define CHUNK 2048

uint32_t file_read_pages(uint8_t h, uint8_t first_page, uint32_t len)
{
    uint32_t total = 0;
    uint16_t want, n, off = 0;

#ifdef MEMFS
    if (IS_MEM(h)) return memfs_copy(MH(h), first_page, 0, 0, len);
#endif
    errno = 0;
    while (len) {
        want = len > CHUNK ? CHUNK : (uint16_t)len;
        mmu_set(WINDOW_SLOT, first_page);
        ROM_IN();
        n = esx_f_read(h, WINDOW_ADDR + off, want);
        ROM_OUT();
        total += n;
        len -= n;
        off += n;
        if (off == 8192) { off = 0; first_page++; }
        if (n != want || errno)
            break;
        if (file_idle_hook) {
            mmu_set(WINDOW_SLOT, DEFAULT_WINDOW_PAGE);
            file_idle_hook();
        }
    }
    mmu_set(WINDOW_SLOT, DEFAULT_WINDOW_PAGE);
    return total;
}

uint16_t file_read_at(uint8_t h, uint8_t page, uint16_t offset, uint16_t len)
{
    uint16_t n;
#ifdef MEMFS
    if (IS_MEM(h)) return (uint16_t)memfs_copy(MH(h), page, offset, 0, len);
#endif
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
    n = file_read(h, dst, len);
    file_close(h);
    return n;
}

uint8_t file_create(const char *name)
{
    uint8_t h;
    errno = 0;
    name = resident_name(name);
    ROM_IN();
    h = esx_f_open(name, ESX_MODE_W | ESX_MODE_OPEN_CREAT_TRUNC);
    ROM_OUT();
    return errno ? FILE_ERROR : h;
}

/* plain transfers to/from resident memory (not slot 0, it holds the ROM meanwhile) */
uint16_t file_write(uint8_t h, const void *src, uint16_t len)
{
    uint16_t n;
    errno = 0;
    ROM_IN();
    n = esx_f_write(h, (void *)src, len);
    ROM_OUT();
    return errno ? 0 : n;
}

uint16_t file_read(uint8_t h, void *dst, uint16_t len)
{
    uint16_t n;
#ifdef MEMFS
    if (IS_MEM(h)) return (uint16_t)memfs_copy(MH(h), 0, 0, (uint8_t *)dst, len);
#endif
    errno = 0;
    ROM_IN();
    n = esx_f_read(h, dst, len);
    ROM_OUT();
    return errno ? 0 : n;
}
