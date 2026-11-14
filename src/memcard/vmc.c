/*
 * sceVmc*: the "virtual memory card" library (PS1 memory card emulation),
 * thin wrappers that map the _card_* return codes to the sceVmc ones.
 */

#include "card.h"

int sceVmcInit(void)
{
	return 1;
}

int sceVmcEnd(void)
{
	return 1;
}

int sceVmcAttach(void *image)
{
	int r = _card_attach(image, 1);
	if (r == -1)
		return -1;
	if (r > 0)
		return -2;
	return 1;
}

int sceVmcGetInfo(void *image, int *outFree, int *outAttached, int *outSize)
{
	int r;
	/* NOTE: bug: the result of _card_get_info is dropped and r is tested uninitialized */
	_card_get_info(image, outFree, outAttached, outSize);
	if (r == -1)
		return -1;
	if (r > 0)
		return -2;
	return 1;
}

int sceVmcReadFile(void *image, const char *name, void *buf, int offset, int size)
{
	int r = _card_read_file(image, name, buf);
	if (r == -1)
		return -1;
	if (r == -2)
		return -3;
	return 1;
}

int sceVmcWriteFile(void *image, const char *name, const void *buf, int offset, int size)
{
	int r = _card_write_file(image, name, buf);
	if (r == -1)
		return -1;
	if (r == -2)
		return -3;
	return 1;
}

int sceVmcDeleteFile(void *image, const char *name)
{
	int r = _card_delete_file(image, name);
	if (r == -1)
		return -1;
	if (r == -2)
		return -3;
	return 1;
}

int sceVmcCreateFile(void *image, const char *name, int nblocks)
{
	int r = _card_create_file(image, name, nblocks);
	if (r == -1)
		return -1;
	if (r == -2)
		return -4;
	if (r == -3)
		return -5;
	return 1;
}

int sceVmcGetDir(void *image, int a1, int a2, DIRENTRY *ent)
{
	return _card_get_dir(image, ent);
}

int sceVmcFormat(void *image)
{
	_card_format(image);
	return 1;
}
