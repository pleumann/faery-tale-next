/* File loading from the SD card via esxDOS (MMU0/1 must hold the ROM during calls). */
#ifndef FILES_H
#define FILES_H

#include <stdint.h>

/* Load a whole file into consecutive 8K pages starting at first_page.
   Returns number of bytes read, or 0 on error. */
uint32_t file_load_pages(const char *name, uint8_t first_page);

/* Load up to len bytes of a file into a normal (unbanked) address. */
uint16_t file_load(const char *name, void *dst, uint16_t len);

#endif
