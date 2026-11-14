/*
 * _card_*: memory card file system over a card image mapped in memory.
 * Sector 0 holds the "MC" signature, sectors 1..15 the 15 directory slots,
 * 16..35 the free-block map; each slot owns one 8192-byte data block, and the
 * blocks of a file are chained through the slots.
 */

#include <stdio.h>
#include <string.h>
#include "card.h"

#define CARD_SECTOR_SIZE 128
#define CARD_BLOCK_SIZE  8192
#define CARD_SLOT_COUNT  15 /* directory slots, one data block each */

/* CardSlotEntry::status: the high nibble is the kind (in use or free). */
#define CARD_SLOT_KIND_MASK 0xF0
#define CARD_SLOT_KIND_USED 0x50
enum {
	CARD_SLOT_HEAD = 0x51,        /* first block of a file */
	CARD_SLOT_MID = 0x52,         /* chained block */
	CARD_SLOT_LAST = 0x53,        /* last block of a file of two or more */
	CARD_SLOT_FREE = 0xA0,        /* free */
	CARD_SLOT_FREE_DELETED = 0xA1 /* free, head of a deleted file */
};

typedef struct {
	unsigned char b[CARD_SECTOR_SIZE];
} card_sector128;


typedef struct {
	unsigned char b[32];
} card_chunk32;

/* One directory slot as stored in sectors 1..15. */
typedef struct {
	/* 0x00 */ int status; /* CARD_SLOT_* */
	/* 0x04 */ int size; /* file size in bytes (head slot) */
	/* 0x08 */ unsigned short next; /* next slot of the file, 0xFFFF = end */
	/* 0x0A */ char name[20];
	/* 0x1E */ unsigned char pad_1E;
	/* 0x1F */ unsigned char attr;
} CardSlotEntry; /* size = 0x20 */
static CardSlotEntry card_slot_status_table[CARD_SLOT_COUNT];

/* Free-block map words, written by _card_format to sectors 16..35. */
static int g_cardFreeMap[20];


/* Per-slot marks: slots reached by a chain walk, or to be written back. */
static signed char g_cardSlotResult[CARD_SLOT_COUNT];

/* Sector buffer. */
static unsigned char g_cardSdbBuf[CARD_SECTOR_SIZE];


/* Data block of one slot. */
typedef struct {
	unsigned char b[CARD_BLOCK_SIZE];
} card_block8192;

/* Directory id written into the header fields of _card_get_dir; empty in this build. */
#define CARD_DIR_ID ""

/* XOR of the first 127 bytes; sectors 0..63 keep it in byte 127. */
static int card_checksum(unsigned char *p)
{
	int i;
	int c = 0;

	for (i = 0x7e; i >= 0; i--) {
		int d = *p ^ c;
		c = d;
		p++;
	}
	return c;
}

/* Computes the checksum and stores it in byte 127. */
static int card_checksum_store(unsigned char *p)
{
	int i;
	int c = 0;

	for (i = 0x7e; i >= 0; i--) {
		int d = *p ^ c;
		c = d;
		p++;
	}
	*p = c;
	return c;
}

int _card_read_sector(unsigned char *mapbase, int sector, unsigned char *out)
{
	unsigned char *src;

	if ((unsigned int)sector >= 1024) {
		printf("Invalid Sector Number %d\n", sector);
		return 0;
	}
	src = (unsigned char *)((sector << 7) + (unsigned int)mapbase);
	*(card_sector128 *)out = *(card_sector128 *)src;
	if (sector < 64) {
		if (card_checksum(out) != out[127])
			return 0;
	}
	return 1;
}

int _card_write_sector(unsigned char *mapbase, int sector, unsigned char *src)
{
	unsigned char *dst;

	if ((unsigned int)sector >= 1024) {
		printf("Invalid Sector Number %d\n", sector);
		return 0;
	}
	if (sector < 64)
		card_checksum_store(src);
	dst = (unsigned char *)((sector << 7) + (unsigned int)mapbase);
	*(card_sector128 *)dst = *(card_sector128 *)src;
	return 1;
}

int _card_read_sdb(void *ctx, void *out)
{
	int i = 0;
	unsigned char *buf = g_cardSdbBuf;
	unsigned char *dst = (unsigned char *)out;

	do {
		memset(buf, 0, CARD_SECTOR_SIZE);
		_card_read_sector(ctx, i + 1, buf);
		i++;
		*(card_chunk32 *)dst = *(card_chunk32 *)buf;
		dst += 32;
	} while (i < CARD_SLOT_COUNT);

	return 1;
}

/*
 * Walks the file chain from slot, marking each slot in g_cardSlotResult.
 * Returns 0 if the chain goes backwards, out of range or loops, 1 when it
 * reaches the 0xFFFF end.
 */
static int card_activate_slot(int slot)
{
	int cur = slot;

	g_cardSlotResult[slot] = 1;
	while (card_slot_status_table[cur].next != 0xFFFF) {
		int next = card_slot_status_table[cur].next;

		if (next < cur)
			return 0;
		cur = next;
		if ((unsigned int)next >= CARD_SLOT_COUNT)
			return 0;
		if (g_cardSlotResult[next] == 1)
			return 0;
		g_cardSlotResult[next] = 1;
	}
	return 1;
}

/*
 * Formats the card: all 15 slots CARD_SLOT_FREE with no chain, the free-block
 * map sectors 16..35 cleared to -1, and the "MC" signature in sector 0.
 */
int _card_format(void *ctx)
{
	int i;

	for (i = 0; i < CARD_SLOT_COUNT; i++) {
		memset(g_cardSdbBuf, 0, sizeof(g_cardSdbBuf));
		memset(&card_slot_status_table[i], 0, sizeof(CardSlotEntry));
		card_slot_status_table[i].status = CARD_SLOT_FREE;
		card_slot_status_table[i].size = 0;
		card_slot_status_table[i].next = 0xFFFF;
		memcpy(g_cardSdbBuf, &card_slot_status_table[i], sizeof(CardSlotEntry));
		_card_write_sector(ctx, i + 1, g_cardSdbBuf);
	}

	for (i = 0; i < 20; i++) {
		g_cardFreeMap[i] = -1;
		bzero(g_cardSdbBuf, sizeof(g_cardSdbBuf));
		memcpy(g_cardSdbBuf, &g_cardFreeMap[i], 4);
		_card_write_sector(ctx, i + 16, g_cardSdbBuf);
	}

	memset(g_cardSdbBuf, 0, sizeof(g_cardSdbBuf));
	g_cardSdbBuf[0] = 'M';
	g_cardSdbBuf[1] = 'C';
	_card_write_sector(ctx, 0, g_cardSdbBuf);

	return 0;
}

/*
 * Loads the slot table from sectors 1..15. Without the "MC" signature the card
 * is formatted first if mode is 1, otherwise -1 is returned. File heads with a
 * broken chain, and slots no chain reaches, become free; with mode 1 the table
 * is written back. Returns the number of file heads freed.
 */
int _card_attach(void *ctx, int mode)
{
	int i;
	int count = 0;
	int changed;

	memset(g_cardSdbBuf, 0, sizeof(g_cardSdbBuf));
	_card_read_sector(ctx, 0, g_cardSdbBuf);

	if (!(g_cardSdbBuf[0] == 'M' && g_cardSdbBuf[1] == 'C')) {
		if (mode != 1)
			return -1;
		_card_format(ctx);
	}

	for (i = 0; i < CARD_SLOT_COUNT; i++) {
		memset(g_cardSdbBuf, 0, sizeof(g_cardSdbBuf));
		_card_read_sector(ctx, i + 1, g_cardSdbBuf);
		memcpy(&card_slot_status_table[i], g_cardSdbBuf, sizeof(CardSlotEntry));
	}

	do {
		changed = 0;
		for (i = 0; i < CARD_SLOT_COUNT; i++)
			g_cardSlotResult[i] = 0;

		for (i = 0; i < CARD_SLOT_COUNT; i++) {
			if (card_slot_status_table[i].status == CARD_SLOT_HEAD) {
				if (card_activate_slot(i) == 0) {
					card_slot_status_table[i].status = CARD_SLOT_FREE;
					changed = 1;
					count++;
				}
			}
		}
	} while (changed);

	for (i = 0; i < CARD_SLOT_COUNT; i++) {
		if (g_cardSlotResult[i] == 0)
			card_slot_status_table[i].status = CARD_SLOT_FREE;
	}

	if (mode != 1)
		return count;

	for (i = CARD_SLOT_COUNT - 1; i >= 0; i--) {
		memset(g_cardSdbBuf, 0, sizeof(g_cardSdbBuf));
		memcpy(g_cardSdbBuf, &card_slot_status_table[i], sizeof(CardSlotEntry));
		_card_write_sector(ctx, i + 1, g_cardSdbBuf);
	}

	return count;
}

/*
 * Creates a file of nblocks slots: a CARD_SLOT_HEAD with the size in bytes and
 * the name, then CARD_SLOT_MID slots, the last one CARD_SLOT_LAST. Returns
 * -1 if the card does not attach, -2 if the name exists, -3 if there are not
 * enough free slots, 0 on success.
 */
int _card_create_file(void *ctx, const char *name, int nblocks)
{
	int count = 0;
	int i;
	int allocated;
	int prev;

	if (_card_attach(ctx, 0) == -1)
		return -1;

	i = 0;
	do {
		if (card_slot_status_table[i].status == CARD_SLOT_HEAD
		    && strcmp(card_slot_status_table[i].name, name) == 0)
			return -2;
		i++;
	} while (i < CARD_SLOT_COUNT);

	i = 0;
	do {
		g_cardSlotResult[i] = 0;
		if ((card_slot_status_table[i].status & CARD_SLOT_KIND_MASK) == CARD_SLOT_FREE)
			count++;
		i++;
	} while (i < CARD_SLOT_COUNT);
	if (count < nblocks)
		return -3;

	allocated = 0;
	prev = 0;
	i = 0;
	do {
		if ((card_slot_status_table[i].status & CARD_SLOT_KIND_MASK) == CARD_SLOT_FREE) {
			if (allocated == 0) {
				card_slot_status_table[i].status = CARD_SLOT_HEAD;
				card_slot_status_table[i].size = nblocks * CARD_BLOCK_SIZE;
				strncpy(card_slot_status_table[i].name, name, 20);
				prev = i;
				g_cardSlotResult[i] = 1;
			} else {
				card_slot_status_table[prev].next = i;
				card_slot_status_table[i].status = CARD_SLOT_MID;
				prev = i;
				g_cardSlotResult[i] = 1;
			}
			allocated++;
			if (allocated >= nblocks) {
				card_slot_status_table[i].next = 0xFFFF;
				if (allocated >= 2)
					card_slot_status_table[i].status = CARD_SLOT_LAST;
				break;
			}
		}
		i++;
	} while (i < CARD_SLOT_COUNT);

	for (i = CARD_SLOT_COUNT - 1; i >= 0; i--) {
		if (g_cardSlotResult[i]) {
			memset(g_cardSdbBuf, 0, sizeof(g_cardSdbBuf));
			*(card_chunk32 *)g_cardSdbBuf = *(card_chunk32 *)&card_slot_status_table[i];
			_card_write_sector(ctx, i + 1, g_cardSdbBuf);
		}
	}

	return 0;
}

/*
 * Deletes a file: the head becomes CARD_SLOT_FREE_DELETED and every chained slot
 * gets the free kind (0xA0 high nibble); only the touched slots are written back.
 * Returns -1 if the card does not attach, -2 if the name is not found, 0 on success.
 */
int _card_delete_file(void *ctx, const char *name)
{
	int slot = -1;
	int i;

	if (_card_attach(ctx, 0) == -1)
		return -1;

	i = 0;
	do {
		if (card_slot_status_table[i].status == CARD_SLOT_HEAD
		    && strcmp(card_slot_status_table[i].name, name) == 0) {
			slot = i;
			break;
		}
		i++;
	} while (i < CARD_SLOT_COUNT);
	if (slot == -1)
		return -2;

	card_slot_status_table[i].status = CARD_SLOT_FREE_DELETED;
	g_cardSlotResult[i] = 1;

	while (card_slot_status_table[i].next != 0xFFFF) {
		i = card_slot_status_table[i].next;
		card_slot_status_table[i].status = (card_slot_status_table[i].status & 0xF) | CARD_SLOT_FREE;
		g_cardSlotResult[i] = 1;
	}

	for (i = CARD_SLOT_COUNT - 1; i >= 0; i--) {
		if (g_cardSlotResult[i]) {
			memset(g_cardSdbBuf, 0, sizeof(g_cardSdbBuf));
			*(card_chunk32 *)g_cardSdbBuf = *(card_chunk32 *)&card_slot_status_table[i];
			_card_write_sector(ctx, i + 1, g_cardSdbBuf);
		}
	}

	return 0;
}

int _card_get_dir(void *ctx, DIRENTRY *out)
{
	char hdr[96];  /* unused, but part of the layout */
	char *p = hdr;
	int count = 0;
	int i;

	{
		struct {
			char s[2];
		} f = { { CARD_DIR_ID[0], 0 } };
		memcpy(p + 36, &f, sizeof f);
	}
	{
		struct {
			char s[3];
		} f = { { CARD_DIR_ID[0], CARD_DIR_ID[1], 0 } };
		memcpy(p + 40, &f, sizeof f);
	}
	{
		struct {
			char s[4];
		} f = { { CARD_DIR_ID[0], CARD_DIR_ID[1], CARD_DIR_ID[2], 0 } };
		memcpy(p + 44, &f, sizeof f);
	}
	{
		struct {
			char s[5];
		} f = { { CARD_DIR_ID[0], CARD_DIR_ID[1], CARD_DIR_ID[2], CARD_DIR_ID[3], 0 } };
		memcpy(p + 48, &f, sizeof f);
	}
	{
		struct {
			char s[6];
		} f = { { CARD_DIR_ID[0], CARD_DIR_ID[1], CARD_DIR_ID[2], CARD_DIR_ID[3], CARD_DIR_ID[4], 0 } };
		memcpy(p + 56, &f, sizeof f);
	}
	{
		struct {
			char s[7];
		} f = { { CARD_DIR_ID[0], CARD_DIR_ID[1], CARD_DIR_ID[2], CARD_DIR_ID[3], CARD_DIR_ID[4],
		    CARD_DIR_ID[5], 0 } };
		memcpy(p + 64, &f, sizeof f);
	}
	{
		struct {
			char s[8];
		} f = { { CARD_DIR_ID[0], CARD_DIR_ID[1], CARD_DIR_ID[2], CARD_DIR_ID[3], CARD_DIR_ID[4],
		    CARD_DIR_ID[5], CARD_DIR_ID[6], 0 } };
		memcpy(p + 72, &f, sizeof f);
	}
	p[0] = 0;

	if (_card_attach(ctx, 0) == -1)
		return -1;

	for (i = 0; i < CARD_SLOT_COUNT; i++) {
		if (card_slot_status_table[i].status == CARD_SLOT_HEAD) {
			if (out != 0) {
				strncpy(out->EntryName, card_slot_status_table[i].name, 20);
				out->AttrFile = card_slot_status_table[i].attr;
				out->FileSizeByte = card_slot_status_table[i].size;
				out->EntryNamePtr = out->EntryName;
				out++;
			}
			count++;
		}
	}

	if (out != 0)
		out->EntryNamePtr = 0;

	return count;
}

/*
 * Copies the file into out by following its slot chain; the data block of slot
 * i is at ctx + (i + 1) * 8192. Returns -1 if the card does not attach, -2 if
 * the name is not found, 0 on success.
 */
int _card_read_file(void *ctx, const char *name, void *out)
{
	int i;
	unsigned char *dst = (unsigned char *)out;
	int slot = -1;

	if (_card_attach(ctx, 0) == -1)
		return -1;

	i = 0;
	do {
		if (card_slot_status_table[i].status == CARD_SLOT_HEAD
		    && strcmp(card_slot_status_table[i].name, name) == 0) {
			slot = i;
			break;
		}
		i++;
	} while (i < CARD_SLOT_COUNT);
	if (slot == -1)
		return -2;
	i = slot;
	do {
		*(card_block8192 *)dst = *(card_block8192 *)((i << 13) + (unsigned int)ctx + CARD_BLOCK_SIZE);
		dst += CARD_BLOCK_SIZE;
		i = card_slot_status_table[i].next;
	} while (i != 0xFFFF);

	return 0;
}

/*
 * Writes data over an existing file along its slot chain. It does not attach:
 * it reloads only the slot table, and returns -1 if the name is not found.
 */
int _card_write_file(void *ctx, const char *name, const void *data)
{
	int i;
	const unsigned char *src = (const unsigned char *)data;
	int slot = -1;

	_card_read_sdb(ctx, card_slot_status_table);

	i = 0;
	do {
		if (card_slot_status_table[i].status == CARD_SLOT_HEAD
		    && strcmp(card_slot_status_table[i].name, name) == 0) {
			slot = i;
			break;
		}
		i++;
	} while (i < CARD_SLOT_COUNT);
	if (slot < 0)
		return -1;
	i = slot;

	do {
		*(card_block8192 *)((i << 13) + (unsigned int)ctx + CARD_BLOCK_SIZE) = *(const card_block8192 *)src;
		src += CARD_BLOCK_SIZE;
		i = card_slot_status_table[i].next;
	} while (i != 0xFFFF);

	return 0;
}

/*
 * Reports whether the card attaches, the _card_attach result and the free space
 * in bytes (15 slots of 8192 bytes minus those in use). Returns the _card_attach
 * result, or 0 when it fails.
 */
int _card_get_info(void *ctx, int *outFree, int *outAttached, int *outSize)
{
	int result;
	int i;
	int used;

	result = _card_attach(ctx, 0);
	if (result == -1) {
		if (outAttached)
			*outAttached = 0;
		if (outFree)
			*outFree = 0;
		if (outSize)
			*outSize = 0;
		return 0;
	}

	if (outAttached)
		*outAttached = 1;

	if (result > 0) {
		if (outSize)
			*outSize = result;
	}
	used = 0;
	{
		CardSlotEntry *p = card_slot_status_table;

		i = CARD_SLOT_COUNT - 1;
		do {
			int v, next;

			v = p->status;
			p++;
			i--;
			next = used + CARD_BLOCK_SIZE;
			v = (v & CARD_SLOT_KIND_MASK) ^ CARD_SLOT_KIND_USED;
			if (v == 0)
				used = next;
		} while (i >= 0);
	}

	if (outFree)
		*outFree = 0x1E000 - used;

	return result;
}
