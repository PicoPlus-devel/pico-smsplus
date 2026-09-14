// Host-build shim for FatFs, backed by stdio in hosttest/fatfs_host.c.
// Only what the smsplus core touches (system.c save/load state) is provided.
#pragma once
#include <stdint.h>
#include <stddef.h>

#define FF_MAX_LFN 255

#define FA_READ          0x01
#define FA_WRITE         0x02
#define FA_CREATE_ALWAYS 0x08

typedef unsigned int UINT;
typedef uint64_t     FSIZE_t;

typedef enum {
	FR_OK = 0,
	FR_DISK_ERR,
	FR_NO_FILE,
} FRESULT;

// FIL: fp holds a host FILE *; size cached at open for f_size().
typedef struct { FSIZE_t size; void *fp; } FIL;

FRESULT f_open (FIL *fp, const char *path, uint8_t mode);
FRESULT f_close(FIL *fp);
FRESULT f_read (FIL *fp, void *buff, UINT btr, UINT *br);
FRESULT f_write(FIL *fp, const void *buff, UINT btw, UINT *bw);

static inline FSIZE_t f_size(FIL *fp) { return fp->size; }
