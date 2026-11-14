#ifndef _PS2MEMLIB_H
#define _PS2MEMLIB_H

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

void * ps2mem_alloc_pages(unsigned int size_in_page);
int ps2_mem_alloc_dmamem(void);

struct __dmamem_symbol {
	int magic;
	void **symbol;
	int size;
	int align;
	int sign;
};

#define DMAMEM_SYMTAB_MAGIC 0xee65cafe
#define DMAMEM_SYMTAB_SIGNATURE(a)	\
	(DMAMEM_SYMTAB_MAGIC + (unsigned long) a )

#define ALLOC_DMAMEM_ALIGN(sym, type, aval)			\
	typeof(type) *sym = 0;					\
	__asm__(".section __dmamem_symtab,\"a\"\n.previous");	\
	const struct __dmamem_symbol __dmamem_symtab_##sym	\
		__attribute__((section("__dmamem_symtab"))) =	\
		{ magic: DMAMEM_SYMTAB_MAGIC,			\
			symbol: (void **)&sym,			\
			size: sizeof(type),			\
			align: aval,				\
			sign: DMAMEM_SYMTAB_SIGNATURE(&sym) };

#define ALLOC_DMAMEM(sym, type)		ALLOC_DMAMEM_ALIGN(sym, type, 16)
#define ALLOC_DMAMEM_IN(sym, type)	ALLOC_DMAMEM_ALIGN(sym, type, 128)

#ifdef __cplusplus
}
#endif // __cplusplus

#endif /* _PS2MEMLIB_H */
