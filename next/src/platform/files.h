/* File loading from the SD card via esxDOS (MMU0/1 must hold the ROM during calls). */
#ifndef FILES_H
#define FILES_H

#include <stdint.h>

#define FILE_ERROR 0xFF

/* Open for reading; returns a handle or FILE_ERROR (errno holds the esxDOS code). */
uint8_t file_open(const char *name);
void file_close(uint8_t h);
uint8_t file_seek(uint8_t h, uint32_t pos);          /* 0 on success */

/* Read len bytes into consecutive 8K pages starting at first_page. Returns bytes read. */
uint32_t file_read_pages(uint8_t h, uint8_t first_page, uint32_t len);

/* Read len bytes (len + offset <= 8192) into one page at the given offset. */
uint16_t file_read_at(uint8_t h, uint8_t page, uint16_t offset, uint16_t len);

/* Create/truncate for writing; plain reads and writes of resident memory. */
uint8_t file_create(const char *name);
uint16_t file_write(uint8_t h, const void *src, uint16_t len);
uint16_t file_read(uint8_t h, void *dst, uint16_t len);

/* Whole-file convenience helpers. */
uint32_t file_load_pages(const char *name, uint8_t first_page);
uint16_t file_load(const char *name, void *dst, uint16_t len);

#endif
