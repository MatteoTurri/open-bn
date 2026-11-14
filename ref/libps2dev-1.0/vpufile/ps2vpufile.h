

#ifndef _PS2VPUFILE_H
#define _PS2VPUFILE_H

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

typedef struct vpu_file_struct VPUFILE;


VPUFILE * vpuobj_open(char * fn, unsigned int opt);

#define	O_TEXT_PS2MEM	0x1
#define	O_DATA_PS2MEM	0x2

void vpuobj_close(VPUFILE * f);
int vpuobj_text(VPUFILE *f, void **start, int *size);
int vpuobj_data(VPUFILE *f, void **start, int *size);
void * vpuobj_relocate_addr(VPUFILE *f, int section, void *addr);
int vpuobj_symbol_nr(VPUFILE *f);
int vpuobj_symbol(VPUFILE *f, int index, char **name,
                        unsigned long *val, int *section);
int vpuobj_symbol_by_name(VPUFILE *f, char *name,
                        unsigned long *val, int *section);

void vpuobj_show_map(void);

struct __vpu_symbol {
	int magic;
	void **symbol;
	char *symbol_name;
	char *file_name;
	int  sign;
};


#define VPU_SYMTAB_MAGIC 0xff0550ff
#define VPU_SYMTAB_SIGNATURE(a,b,c)	\
	(VPU_SYMTAB_MAGIC + (unsigned long) a )

#define IMPORT_VPU_SYMBOL(typ, sym, fn)				\
	typ sym = (typ)0;						\
	__asm__(".section __vpu_symtab,\"a\"\n.previous");	\
	const struct __vpu_symbol __vpu_symtab_##sym		\
		__attribute__((section("__vpu_symtab"))) =	\
		{ magic: VPU_SYMTAB_MAGIC, 			\
			symbol: (void **)&sym, 			\
			symbol_name: #sym, 			\
			file_name: fn, 			\
			sign: VPU_SYMTAB_SIGNATURE(&sym, #sym, fn) };

#ifdef __cplusplus
}
#endif // __cplusplus

#endif /* _PS2VPUFILE_H */
