/*
 * DNAS protected-region boundaries. A protected function body is stored
 * scrambled and is descrambled in place by a protection shim while it runs.
 */
#ifndef DNAS_PROT_H
#define DNAS_PROT_H

/*
 * The macros emit asm markers (#DNASBEG, #DNASPRE, #DNASEND, #DNASSTR) that
 * never reach the object file: a build pass between compiler and assembler
 * replaces each one with the fixed protection prologue or epilogue text,
 * which loads the region descriptor and label into fixed registers and calls
 * the shim. The register bindings below are those of that text.
 *
 * Shims: DNAS1 _sce_dnas_pwrap_main; DNAS2 module 11 __sce_dnas2_symbol11002,
 * module 12 __sce_dnas2_symbol12002. A DNAS2 unit defines SCE_DNAS2_SHIM
 * before including this header; it is read where a macro is used.
 */
#define _SCE_DNAS_CAT2(a, b) a##b
#define _SCE_DNAS_CAT(a, b) _SCE_DNAS_CAT2(a, b)

extern void __sce_dnas2_symbol11002();
extern void __sce_dnas2_symbol12002();
extern void _sce_dnas_pwrap_main();

/*
 * End of a region: two markers (the branch over the descriptor words, then
 * the loads of descriptor and label into v0/a0), then the shim call with
 * descriptor words 2, 0 and 1.
 */
#define _SCE_DNAS_END(shim)						     \
	{ register int *_sce_dnas_d __asm__ ("$2");			     \
	  register void *_sce_dnas_l __asm__ ("$4");			     \
	  __asm__ __volatile__ ("#DNASPRE" : : );			     \
	  __asm__ __volatile__ ("#DNASEND %0 %1"			     \
				: "=r" (_sce_dnas_d), "=r" (_sce_dnas_l));   \
	  shim(_sce_dnas_l, _sce_dnas_d[2], _sce_dnas_d[0], _sce_dnas_d[1]); }

/*
 * Start of a region: a marker that loads the descriptor into v1, then, if the
 * region is scrambled (d[0] == 0), the shim call with the region label and
 * descriptor words 3, 1 and 2.
 */
#define _SCE_DNAS_BEGIN(shim)						     \
	{ register int *_sce_dnas_d __asm__ ("$3");			     \
	  __asm__ __volatile__ ("#DNASBEG %0" : "=r" (_sce_dnas_d));	     \
	  if (_sce_dnas_d[0] == 0)					     \
		  shim(&&_SCE_DNAS_CAT(_sce_dnas_rgn, __LINE__),	     \
		       _sce_dnas_d[3], _sce_dnas_d[1], _sce_dnas_d[2]); }      \
	_SCE_DNAS_CAT(_sce_dnas_rgn, __LINE__): ;

/* DNAS2 */
#define SCE_DNAS_PROT_BEGIN() _SCE_DNAS_BEGIN(SCE_DNAS2_SHIM)
#define SCE_DNAS_PROT_END() _SCE_DNAS_END(SCE_DNAS2_SHIM)

/* DNAS1: same macros, with the DNAS1 shim. */
#define SCE_DNAS1_PROT_BEGIN() _SCE_DNAS_BEGIN(_sce_dnas_pwrap_main)
#define SCE_DNAS1_PROT_END() _SCE_DNAS_END(_sce_dnas_pwrap_main)

/* String literal stored inside a region (both families). */
#define SCE_DNAS_STR(s)							     \
	({ char *_sce_dnas_p;						     \
	   __asm__ __volatile__ ("#DNASSTR %0 " #s : "=r" (_sce_dnas_p));   \
	   _sce_dnas_p; })

#endif
