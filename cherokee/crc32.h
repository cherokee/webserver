#ifndef _CRC32_H
#define _CRC32_H

#include "common.h"

/* Returns crc32 of data block */
crc_t crc32_sz(char *buf, int size);
crc_t crc32_partial_sz (crc_t crc_in, char *buf, int size);

/* Returns crc32 of null-terminated string
#define crc32(buf) crc32_sz((buf),strlen(buf))
*/

#endif

