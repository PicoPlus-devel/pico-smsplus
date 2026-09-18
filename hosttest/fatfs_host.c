// FatFs API on top of stdio for the hosttest harness. Paths are used verbatim.
#include <stdio.h>
#include "ff.h"

FRESULT f_open(FIL *fp, const char *path, uint8_t mode)
{
	if (!fp || !path) return FR_DISK_ERR;
	const char *m = "rb";
	if (mode & FA_WRITE) m = (mode & FA_CREATE_ALWAYS) ? "wb" : "r+b";
	FILE *f = fopen(path, m);
	if (!f) return FR_NO_FILE;

	fseek(f, 0, SEEK_END);
	fp->size = (FSIZE_t)ftell(f);
	fseek(f, 0, SEEK_SET);
	fp->fp = f;
	return FR_OK;
}

FRESULT f_close(FIL *fp)
{
	if (!fp || !fp->fp) return FR_OK;
	fclose((FILE *)fp->fp);
	fp->fp = NULL;
	return FR_OK;
}

FRESULT f_read(FIL *fp, void *buff, UINT btr, UINT *br)
{
	if (!fp || !fp->fp || !buff || !br) return FR_DISK_ERR;
	*br = (UINT)fread(buff, 1, btr, (FILE *)fp->fp);
	return FR_OK;
}

FRESULT f_write(FIL *fp, const void *buff, UINT btw, UINT *bw)
{
	if (!fp || !fp->fp || !buff || !bw) return FR_DISK_ERR;
	*bw = (UINT)fwrite(buff, 1, btw, (FILE *)fp->fp);
	return FR_OK;
}
