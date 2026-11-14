/*
 * _card_*: memory card file system over a card image mapped in memory
 * (card.c), the back end of the sceVmc* library.
 */

#ifndef CARD_H
#define CARD_H

#include "vmc.h"

int _card_read_sector(unsigned char *mapbase, int sector, unsigned char *out);
int _card_write_sector(unsigned char *mapbase, int sector, unsigned char *src);
int _card_read_sdb(void *ctx, void *out);
int _card_format(void *ctx);
int _card_attach(void *ctx, int mode);
int _card_create_file(void *ctx, const char *name, int nblocks);
int _card_delete_file(void *ctx, const char *name);
int _card_get_dir(void *ctx, DIRENTRY *out);
int _card_read_file(void *ctx, const char *name, void *out);
int _card_write_file(void *ctx, const char *name, const void *data);
int _card_get_info(void *ctx, int *outFree, int *outAttached, int *outSize);

#endif
