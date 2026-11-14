/*
 * __cfd_symbol4: opens the memory-mapped session file and decrypts its key
 * block, plus the static helpers that decode the file path and offsets.
 */

#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include "cfd.h"

typedef struct {
	unsigned char b[16];
} CfdKey16;
typedef struct {
	unsigned char b[8];
} CfdKey8;

static const CfdKey8 cfd_pathDesKey = { {
#include "../../assets/cfd/cfd2_path_iv.inc"
} };

/* DES-decrypts `count` blocks from `src` into `dst`, with cfd_pathDesKey as the DES key. */
static void cfd_decrypt_path(unsigned char *src, unsigned char *dst, unsigned int count)
{
	CfdKey8 desKey = cfd_pathDesKey;

	__cfd_symbol0(src, desKey.b, dst, count);
}

static const CfdKey16 cfd_pathCipher = { {
#include "../../assets/cfd/cfd2_path_key.inc"
} };

/* Builds the session file path by decrypting the 16-byte constant cfd_pathCipher. */
static void cfd_build_path(char *path)
{
	CfdKey16 cipher = cfd_pathCipher;

	cfd_decrypt_path(cipher.b, (unsigned char *)path, 2);
}

static const CfdKey16 cfd_symbol4_key = { {
#include "../../assets/cfd/cfd2_symbol4_key.inc"
} };
static unsigned char cfd_symbol4_mapOffset[8] = {
#include "../../assets/cfd/cfd2_symbol4_lit1.inc"
};
static unsigned char cfd_symbol4_keyOffset[8] = {
#include "../../assets/cfd/cfd2_symbol4_lit2.inc"
};

static void cfd_decrypt_offset(const unsigned char *src, unsigned int *out);

/* Maps the session file and derives its 24-byte key into `dst` (__cfd_symbol2);
   returns 0, or -1 on failure. */
int __cfd_symbol4(unsigned char *dst)
{
	char path[16];
	CfdKey16 key;
	unsigned char blockKey[8];
	unsigned char block[8];
	unsigned int mapOffset;
	unsigned int keyOffset;
	unsigned int dataOffset;
	int fd;
	void *base = 0;

	key = cfd_symbol4_key;
	cfd_decrypt_offset(cfd_symbol4_mapOffset, &mapOffset);
	cfd_decrypt_offset(cfd_symbol4_keyOffset, &keyOffset);
	cfd_build_path(path);

	fd = open(path, O_RDWR);
	if (fd >= 0) {
		base = mmap(0, 0x200000, PROT_READ | PROT_WRITE, MAP_SHARED, fd, mapOffset);
		if (base != 0) {
			unsigned char *src = (unsigned char *)base + keyOffset;

			__cfd_symbol1(key.b, key.b + 8, blockKey, 1);
			__cfd_symbol0(src, blockKey, block, 1);
			dataOffset = *(unsigned int *)block ^ *(unsigned int *)(block + 4);
			__cfd_symbol3(key.b, 16);
			__cfd_symbol3(blockKey, 8);
			__cfd_symbol3(block, 8);

			if (__cfd_symbol2(dst, (unsigned int)((unsigned char *)base + dataOffset), 16384) >= 0) {
				if (munmap(base, 0x200000) >= 0) {
					if (close(fd) >= 0) {
						__cfd_symbol3(path, 16);
						mapOffset = 0;
						keyOffset = 0;
						return 0;
					}
				}
			}
		}
	}

	__cfd_symbol3(path, 16);
	if (base != 0) {
		munmap(base, 0x200000);
	}
	if (fd >= 0) {
		close(fd);
	}
	mapOffset = 0;
	keyOffset = 0;
	return -1;
}

/* Loads a big-endian 32-bit value into *out. */
static void cfd_load_be32(unsigned int *out, const unsigned char *p)
{
	*out = (p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3];
}

static unsigned char cfd_offsetDesKey[8] = {
#include "../../assets/cfd/cfd2_off_iv.inc"
};

/* Decrypts the 8-byte block `src` (DES key cfd_offsetDesKey) and returns its first 4 bytes,
   big-endian, as an offset. */
static void cfd_decrypt_offset(const unsigned char *src, unsigned int *out)
{
	unsigned char buf[8];

	__cfd_symbol0((unsigned char *)src, cfd_offsetDesKey, buf, 1);
	cfd_load_be32(out, buf);
	__cfd_symbol3(buf, 8);
}
