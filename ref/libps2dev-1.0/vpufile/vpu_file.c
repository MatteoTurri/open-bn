/*
  vpu_file.c: sample code for vpu deivces
 
 	Copyright (C) 2000  Sony Computer Entertainment Inc.
 
  This file is subject to the terms and conditions of the GNU Library
  General Public License Version 2. See the file "COPYING.LIB" in the 
  main directory of this archive for more details.
*/

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <elf.h>
#include <sys/mman.h>
#include <error.h>
#include <string.h>
#include "ps2mem.h"

//#define DEBUG 
//#undef DEBUG 

#ifdef DEBUG
#define OUTPUT_ERRMSG
#endif

#include "ps2vpufile.h"

static void update_vpu_symtab(VPUFILE *p);

static
void  unmapfile(void *addr, int sz)
{
	munmap(addr,sz);
}

static unsigned long _pgsz = 0;
#define PAGE_SIZE (_pgsz ? _pgsz : (_pgsz=getpagesize()))
#define PAGE_MASK (_pgsz ? (_pgsz-1) : (_pgsz=getpagesize(), _pgsz-1))
#define ALIGN(x, a)	(typeof(x))(((unsigned long)(x) + (a - 1)) & ~(a - 1))


/* return real start address */
static
void * mapfile_fd(int fd, int offset, int *sz, void **mapped)
{
	void *m;

	int bias=0;
	if (offset&PAGE_MASK) {
		bias = offset & PAGE_MASK;
		offset -= bias;
		*sz += bias;
	}

	m = mmap(0, *sz, PROT_READ|PROT_WRITE, MAP_PRIVATE, fd, offset );
	if (m==(void*)-1) {
#ifdef OUTPUT_ERRMSG
		perror("mmap");
#endif
		return (void *)-1;
	}
	*mapped = m;
	return m+bias;
}

static
int check_elf_header(Elf32_Ehdr *eh)
{
	if (strncmp(ELFMAG, eh->e_ident, SELFMAG)) {
#ifdef OUTPUT_ERRMSG
		printf("not elf\n");
#endif
		goto out;
	}
	if (eh->e_ident[EI_CLASS] != ELFCLASS32) {
#ifdef OUTPUT_ERRMSG
		printf("not elf32\n");
#endif
		goto out;
	}
	if (eh->e_ident[EI_DATA] != ELFDATA2LSB) {
#ifdef OUTPUT_ERRMSG
		printf("not little endian\n");
#endif
		goto out;
	}
	if (eh->e_version  !=  EV_CURRENT) {
#ifdef OUTPUT_ERRMSG
		printf("not version 1\n");
#endif
		goto out;
	}
	if (eh->e_type != ET_EXEC) {
#ifdef OUTPUT_ERRMSG
		printf("not exec\n");
#endif
		goto out;
	}
	if (eh->e_machine != EM_MIPS) {
#ifdef OUTPUT_ERRMSG
		printf("not mips\n");
#endif
		goto out;
	}
#ifdef DEBUG
	printf("flags:%8.8x\n", eh->e_flags);
	printf("shoff:%8.8x\n", eh->e_shoff);
#endif
	return 0;
out:
	return -1;
}

static
Elf32_Shdr *
get_section_header(Elf32_Ehdr *eh, Elf32_Shdr *sh, char *str, char *name, int *ndx)
{
	int i;
	char * sect_name = 0;
	
	for (i=0; i<eh->e_shnum; i++) {
		//printf("sh:%x\n",sh);
		sect_name = str + sh->sh_name;
		if (!strcmp(sect_name, name)) break;
	
		sh = (Elf32_Shdr *) ((char *)sh + eh->e_shentsize);
	}
	if (!strcmp(sect_name, name)) {
#ifdef DEBUG
		printf("section name:%s offset:%x size:%x\n",
			sect_name, sh->sh_offset, sh->sh_size);
#endif
		*ndx=i;
		return sh;
	}
	return 0;
}


struct ldsec {
	char *name;	// section name
	void *addr;	// section adder
	int  size;	// section size
	int  shndx;	// section index
	void *mapped_addr;	// mmaped adder
	int  mapped_size;	// mmaped size
};

#define VALIDATE_VPUFILE(f) (f && (void*)f!=(void*)-1)

#define TEXT_INX	0
#define DATA_INX	1
#define SYMTAB_INX	2
#define STRTAB_INX	3
#define LDSEC_NR 	4

const struct ldsec ldsec_init[LDSEC_NR] = {
	{".vutext",(void*)1,0,0},
	{".vudata",(void*)1,0,0},
	{".symtab",(void*)0,0,0},
	{".strtab",(void*)0,0,0}
};

struct vpu_file_struct {
	int nsym;	// num of symbol
	char *name;	// file name
	struct ldsec ldsec[LDSEC_NR];
};


VPUFILE * 
vpuobj_open(char * fn, unsigned int opt)
{
	int i;
	VPUFILE *vpu_file;
	struct ldsec ldsec[LDSEC_NR];
	Elf32_Ehdr eh;
	int fd;
	Elf32_Shdr *shs;
	int shsz;
	char *shstr;
	void *str_map;
	int str_sz;

	memcpy(ldsec,ldsec_init, sizeof(ldsec));

	fd = open (fn, O_RDONLY);
	if (fd <0) {
#ifdef OUTPUT_ERRMSG
		perror("open");
#endif
		return 0;
	}

	i = read(fd, &eh, sizeof(eh));

	if (i!=sizeof(eh)) {
		close(fd);
#ifdef OUTPUT_ERRMSG
		fprintf(stderr,"%s:read failed.", fn);
#endif
		errno = EINVAL;
		return 0;
	}

	if (check_elf_header(&eh)) {
		close(fd);
#ifdef OUTPUT_ERRMSG
		fprintf(stderr,"%s:funny elf header.", fn);
#endif
		errno = EINVAL;
		return 0;
	}

	shsz=eh.e_shnum*eh.e_shentsize;
	shs = (Elf32_Shdr *)alloca(shsz);
	if (!shs) {
		close(fd);
#ifdef OUTPUT_ERRMSG
		fprintf(stderr,"%s:alloca failed.", fn);
#endif
		return 0;
	}

	if (lseek(fd, eh.e_shoff, SEEK_SET) < 0) {
		close(fd);
#ifdef OUTPUT_ERRMSG
		fprintf(stderr,"%s:lseek failed.", fn);
#endif
		return 0;
	}

	i = read(fd, shs, shsz);
	if (i != shsz) {
		close(fd);
#ifdef OUTPUT_ERRMSG
		fprintf(stderr,"%s:read failed.", fn);
#endif
		errno = EINVAL;
		return 0;
	}


	str_sz = shs[eh.e_shstrndx].sh_size;
	shstr = mapfile_fd(fd, 
		shs[eh.e_shstrndx].sh_offset, &str_sz, &str_map);

	if (str_map == (void*)-1) {
		close(fd);
#ifdef OUTPUT_ERRMSG
		fprintf(stderr,"%s:shstr mmap failed.", fn);
#endif
		return 0;
	}


	for ( i = 0; i < LDSEC_NR; i ++ ) {

		Elf32_Shdr *sh;
		int shndx;
		ldsec[i].addr=0;
		sh = get_section_header(&eh, 
			shs, shstr, ldsec[i].name, &shndx);
		if (!sh || sh->sh_size==0) 
			continue;
		ldsec[i].shndx = shndx;
		ldsec[i].size = sh->sh_size;
		ldsec[i].mapped_size = sh->sh_size;
		ldsec[i].addr = mapfile_fd(fd, sh->sh_offset, 
			&ldsec[i].mapped_size, &ldsec[i].mapped_addr);

		if (((opt&O_TEXT_PS2MEM)&& (i==TEXT_INX)) ||
		    ((opt&O_DATA_PS2MEM)&& (i==DATA_INX))) {
			unsigned int size_in_page;
			char *mem;
			size_in_page = ALIGN(sh->sh_size, PAGE_SIZE)/PAGE_SIZE;
			mem  = ps2mem_alloc_pages(size_in_page);
			if (mem==(void*)-1) {
				unmapfile(str_map, str_sz);
				close(fd);
				goto out;
			}
			memcpy(mem, ldsec[i].addr, sh->sh_size);
			unmapfile(ldsec[i].mapped_addr,
					ldsec[i].mapped_size);
			ldsec[i].mapped_addr = mem;
			ldsec[i].mapped_size = size_in_page * PAGE_SIZE;
			ldsec[i].addr = mem;
			ldsec[i].size = sh->sh_size;
		}
#ifdef DEBUG 
		printf("ldsec[%d] addr:%8.8lx: sz:%x\n", 
			i, (unsigned long)ldsec[i].addr , ldsec[i].size);
#endif
	}
	unmapfile(str_map, str_sz);
	close(fd);

	/* The file has niether text nor data. */
	if ( !ldsec[TEXT_INX].addr && !ldsec[DATA_INX].addr ) {
#ifdef OUTPUT_ERRMSG
		fprintf(stderr,"%s:niether text nor data.", fn);
#endif
		errno = EINVAL;
		goto out;
	}

	vpu_file = malloc(sizeof(VPUFILE));
	if (!vpu_file) {
#ifdef OUTPUT_ERRMSG
		fprintf(stderr,"%s:no memory", fn);
#endif
	    out:
		for ( i = 0; i < LDSEC_NR; i ++ ) 
			if (ldsec[i].mapped_addr)
				unmapfile(ldsec[i].mapped_addr,
					ldsec[i].mapped_size);
		return 0;
	}

	vpu_file->name = strdup(fn);
	if (!vpu_file->name) {
#ifdef OUTPUT_ERRMSG
		fprintf(stderr,"%s:no memory", fn);
#endif
		free(vpu_file);
		goto out;
	}


	/* update symtab */
	vpu_file->nsym=0;
	for (i = 0 ; i < ldsec[SYMTAB_INX].size/sizeof(Elf32_Sym); i++) {
		Elf32_Sym *sym; 

		sym = ldsec[SYMTAB_INX].addr;
		sym += i;

		if (ELF32_ST_BIND(sym->st_info) == STB_GLOBAL) {
			int type;

		    	if (ELF32_ST_TYPE(sym->st_info) != STT_NOTYPE)  {
#ifdef OUTPUT_ERRMSG
				fprintf(stderr,"%s:unexpected symbol.", fn);
#endif
				free(vpu_file);
				errno = EINVAL;
				goto out;
			}
				

			if (sym->st_shndx == ldsec[TEXT_INX].shndx)
				type = TEXT_INX;
			else if (sym->st_shndx == ldsec[DATA_INX].shndx)
				type = DATA_INX;
			else
				type = -1;

			if (type != -1) {
				sym->st_value += (unsigned int)ldsec[type].addr;
				vpu_file->nsym++;
#ifdef DEBUG 
				printf("%c:%s\t %8.8x\n", 
			
					(type == TEXT_INX) ? 'T': 'D',
					(char *)(ldsec[STRTAB_INX].addr 
						+(int)sym->st_name),
					sym->st_value);
#endif
			}
		}
	}

	memcpy(vpu_file->ldsec, ldsec, sizeof(ldsec));

	/* update vpu_symtab_tbl */
	update_vpu_symtab(vpu_file);

#ifdef DEBUG 
	vpuobj_show_map();
#endif

	return vpu_file;
}

void 
vpuobj_close(VPUFILE * f)
{
	int i;

	if (!VALIDATE_VPUFILE(f)) 
		return;

	for ( i = 0; i < LDSEC_NR; i ++ ) 
		unmapfile(f->ldsec[i].mapped_addr,f->ldsec[i].mapped_size);
	if (f->name)
		free(f->name);
	free(f);
}


int vpuobj_text(VPUFILE *f, void **start, int *size)
{
	*start = f->ldsec[TEXT_INX].addr;
	*size = f->ldsec[TEXT_INX].size;
	return 0;
}

int vpuobj_data(VPUFILE *f, void **start, int *size)
{
	*start = f->ldsec[DATA_INX].addr;
	*size = f->ldsec[DATA_INX].size;
	return 0;
}

int vpuobj_symbol_nr(VPUFILE *f) 
{
	return f->nsym;
}


void * vpuobj_relocate_addr(VPUFILE *f, int section, void *addr)
{
	/* section == 0 ... text, section == 1 ... data */
	int index;
	void *p;

	if (!VALIDATE_VPUFILE(f)) {
		errno = EINVAL;
		return (void*)-1;
	}
	if (section != 0 && section != 1)
		return (void*)-1;
	index =  section==0 ? TEXT_INX : DATA_INX;

	if (addr >= f->ldsec[index].addr && 
		(addr < (f->ldsec[index].addr+f->ldsec[index].size)))
			return addr;

	p = addr + (unsigned long)f->ldsec[index].addr;

	if (p >= f->ldsec[index].addr && 
		(p < (f->ldsec[index].addr+f->ldsec[index].size)))
			return p;

	errno = EINVAL;
	return (void*)-1;
}
         

int vpuobj_symbol(VPUFILE *f, int index, char **name,
                        unsigned long *val, int *section)
{
	/* section == 0 ... text, section == 1 ... data */

	int i;
	int j;

	if (!VALIDATE_VPUFILE(f)) {
		errno = EINVAL;
		return -1;
	}
	j=0;
	for (i = 0 ; i < f->ldsec[SYMTAB_INX].size/sizeof(Elf32_Sym); i++) {
		Elf32_Sym *sym; 

		sym = f->ldsec[SYMTAB_INX].addr;
		sym += i;

		if (ELF32_ST_BIND(sym->st_info) == STB_GLOBAL) {

		    	if (ELF32_ST_TYPE(sym->st_info) != STT_NOTYPE)  {
				continue;
			}
			if ( j++ != index ) 
				continue;
				

			if (sym->st_shndx == f->ldsec[TEXT_INX].shndx)
				*section = 0;
			else if (sym->st_shndx == f->ldsec[DATA_INX].shndx)
				*section = 1;
			else
				*section = -1;

			*name = f->ldsec[STRTAB_INX].addr +(int)sym->st_name;
			*val =  sym->st_value;
			return 0;
		}
	}
	errno = 0;
	return -1;
}

int vpuobj_symbol_by_name(VPUFILE *f, char *name,
                        unsigned long *val, int *section)
{
	/* section == 0 ... text, section == 1 ... data */

	int i;
	int j;


	if (!VALIDATE_VPUFILE(f)) {
		errno = EINVAL;
		return -1;
	}
	j = 0;
	for (i = 0 ; i < f->ldsec[SYMTAB_INX].size/sizeof(Elf32_Sym); i++) {
		Elf32_Sym *sym; 

		sym = f->ldsec[SYMTAB_INX].addr;
		sym += i;

		if (ELF32_ST_BIND(sym->st_info) == STB_GLOBAL) {

		    	if (ELF32_ST_TYPE(sym->st_info) != STT_NOTYPE)  {
				continue;
			}

			if (strcmp(name, 
				f->ldsec[STRTAB_INX].addr +(int)sym->st_name)) {
					j++;
					continue;
			}
				
			if (sym->st_shndx == f->ldsec[TEXT_INX].shndx)
				*section = 0;
			else if (sym->st_shndx == f->ldsec[DATA_INX].shndx)
				*section = 1;
			else
				*section = -1;

			*val = sym->st_value;
			return j;
		}
	}
	errno = 0;
	return -1;
}

extern void * __start___vpu_symtab(void) __attribute__ ((weak));
extern void * __stop___vpu_symtab(void) __attribute__ ((weak));

static void 
update_vpu_symtab(VPUFILE *p)
{
	char *fn;
	struct __vpu_symbol *sym;
	sym = (void *) __start___vpu_symtab;
	if (!sym) {
#ifdef DEBUG
		printf("no __vpu_symtab\n" );
#endif
		return;
	}

	fn = strrchr(p->name, '/');
	if (!fn) fn = p->name;

	while (1) {
		unsigned long val;
		int sec;
		int i;

		if ((void*)sym >= (void *)__stop___vpu_symtab) 
			break;

#ifdef DEBUG
		printf("w:%lx\n",(unsigned long)sym );
#endif
		if ( sym->magic != VPU_SYMTAB_MAGIC ||
			VPU_SYMTAB_SIGNATURE(sym->symbol, sym->symbol_name,
				sym->file_name) != sym->sign)
			break;


		if ( sym->file_name  && 
		     *(sym->file_name) && strcmp(sym->file_name, fn)) {
			sym ++;
			continue;
		}

#ifdef DEBUG
		printf("%s\n", sym->symbol_name);
		printf("%lx\n", (unsigned long)sym->symbol);
#endif
		i = vpuobj_symbol_by_name(p, sym->symbol_name, &val, &sec);
		if (i >= 0) {
			*(sym->symbol) = (void *)val;
		}
		sym ++;
	}
}

// debug funcs.

void vpuobj_show_symbols(VPUFILE * f)
{
	int i;
	int j;

	if (!VALIDATE_VPUFILE(f))
		return;

	j = 0;
	for (i = 0 ; i < f->ldsec[SYMTAB_INX].size/sizeof(Elf32_Sym); i++) {
		Elf32_Sym *sym; 

		sym = f->ldsec[SYMTAB_INX].addr;
		sym += i;

		if (ELF32_ST_BIND(sym->st_info) == STB_GLOBAL) {
			int type;

		    	if (ELF32_ST_TYPE(sym->st_info) != STT_NOTYPE)  {
				continue;
			}
				

			if (sym->st_shndx == f->ldsec[TEXT_INX].shndx)
				type = TEXT_INX;
			else if (sym->st_shndx == f->ldsec[DATA_INX].shndx)
				type = DATA_INX;
			else
				type = -1;

			if (type != -1) {
				printf("%d  %c:%s\t %8.8lx\n", 
					j++,
					type == TEXT_INX ? 'T': 'D',
					(char *) f->ldsec[STRTAB_INX].addr 
						+(int)sym->st_name,
					(unsigned long)sym->st_value);
			}
		}
	}
}

void vpuobj_show_symbol(VPUFILE *p,int n)
{
	char *str;
	unsigned long val;
	int sec;
	if (vpuobj_symbol(p, n, &str, &val, &sec) == 0)
		printf("%d (%d) %s %lx\n", n, sec, str, val);
	else 
		printf("%lx %d fail\n", (unsigned long)p, n);
}

void vpuobj_show_symbol_by_name(VPUFILE *p,char *n)
{
	unsigned long val;
	int sec;
	int i;
	i = vpuobj_symbol_by_name(p, n, &val, &sec);
	if (i >= 0)
		printf("%d (%d) %s %lx\n", i, sec, n, val);
	else 
		printf("%lx %s fail\n", (unsigned long)p, n);
}

void vpuobj_show_map(void)
{
	char str[1024];

#if 0 // please revert later
	sprintf(str,"cat /proc/%d/maps", getpid());
	system(str);
	printf("\n");
#endif

}

