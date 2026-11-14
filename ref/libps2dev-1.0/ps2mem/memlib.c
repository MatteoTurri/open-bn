/*
  memlib.c
  */

/*
 	Copyright (C) 2001  Sony Computer Entertainment Inc.
 
  This file is subject to the terms and conditions of the GNU Library
  General Public License Version 2. See the file "COPYING.LIB" in the 
  main directory of this archive for more details.
*/

#include <stdio.h>
#include <linux/ps2/dev.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <sys/mman.h>
#include "ps2mem.h"

//#define DEBUG
#define ALIGN(x, a)	(typeof(x))(((unsigned long)(x) + (a - 1)) & ~(a - 1))

extern void *__start___dmamem_symtab(void) __attribute__((weak));
extern void *__stop___dmamem_symtab(void) __attribute__((weak));

void * ps2mem_alloc_pages(unsigned int size_in_page)
{
	void *m;
	unsigned size_in_byte;
	int fd;

	size_in_byte = getpagesize() * size_in_page;
	
	m = (void *) -1;
	fd = open (PS2_DEV_MEM, O_RDWR);
	if (fd <0) {
#ifdef DEBUG
		perror("open");
#endif
		goto out;
	}

	m = mmap(0, size_in_byte, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
	if (m==(void*)-1) {
#ifdef DEBUG
		perror("mmap");
#endif
		close(fd);
		goto out;
	}
	close(fd);
out:
	return m;
}


int ps2mem_alloc_dmamem(void)
{
	struct __dmamem_symbol *sym;
	int size = 0;
	int fd;
	void *mem;

	sym = (void *)__start___dmamem_symtab;
	if (!sym) {
#ifdef DEBUG
		printf("no __dmamem_symtab\n");
#endif
		errno = ENODATA;
		return -1;
	}

	while ((void *)sym < (void *)__stop___dmamem_symtab) {
		if (*sym->symbol != NULL) {
#ifdef DEBUG
			printf("already allocated\n");
#endif
			errno = EALREADY;
			return -1;
		}
		if (sym->magic != DMAMEM_SYMTAB_MAGIC ||
			DMAMEM_SYMTAB_SIGNATURE(sym->symbol) != sym->sign)
			break;
#ifdef DEBUG
		printf("size=%d\n", sym->size);
#endif
		size = ALIGN(size, sym->align);
		size += ALIGN(sym->size, sym->align);

		sym++;
	}

	fd = open(PS2_DEV_MEM, O_RDWR);
	if (fd < 0) {
#ifdef DEBUG
		perror(PS2_DEV_MEM);
#endif
		return -1;
	}
	mem = mmap(0, size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
	if (mem == MAP_FAILED) {
#ifdef DEBUG
		perror("alloc_dmamem");
#endif
		close(fd);
		return -1;
	}
	close(fd);

	sym = (void *)__start___dmamem_symtab;
	while ((void *)sym < (void *)__stop___dmamem_symtab) {
		if (sym->magic != DMAMEM_SYMTAB_MAGIC ||
			DMAMEM_SYMTAB_SIGNATURE(sym->symbol) != sym->sign)
			break;

		mem = ALIGN(mem, sym->align);
		*sym->symbol = mem;
#ifdef DEBUG
		printf("addr=%08lx size=%d\n", 
			(unsigned long)*sym->symbol, sym->size);
#endif
		mem += ALIGN(sym->size, sym->align);
		sym++;
	}

	return 0;
}
