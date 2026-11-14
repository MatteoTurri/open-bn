/* Chloe: the path from Linux back to the native PS2 (bn_do_reboot in src/app/bnexec.cpp).
 * When ChloeBanana loads a block from /dev/hda, the steps Rings..Pudding fill a ChloeObj and
 * ChloeMaple decrypts and runs the encrypted stage ChloeDoushite..ChloeUsotsuki; otherwise the
 * steps Cake..Pekoe go through /dev/akmem and "BootNative". Both end by running /sbin/reboot. */

#ifndef CHLOE_H
#define CHLOE_H

/* The Chloe* names are obfuscated (sweets and fruit). Around the encrypted stage the library
   is a small device/loader layer: a state object with a checksum, a few property pairs and a
   table of libc function pointers. Several Chloe* are plain thunks to another member of the
   family. These are the shared declarations: the state object and the Chloe* functions. */

typedef int (*ChloeFn)();

typedef struct ChloeObj {
	/* 0x00 */ unsigned int hash;  /* state checksum, updated by the setters */
	/* 0x04 */ int memSize;  /* memory size (ChloeRings) */
	/* 0x08 */ int pageSize;  /* getpagesize() */
	/* 0x0C */ int propA;  /* property A (ChloePeach) */
	/* 0x10 */ int propB;  /* property B (ChloePeach) */
	/* 0x14 */ int propC;  /* property C (ChloeCocoa); crypt() key in ChloeMelon */
	/* 0x18 */ int propD;  /* property D (ChloeCocoa); crypt() salt in ChloeMelon */
	/* 0x1C */ int blobAddr;  /* address of an external blob (ChloeRings) */
	/* 0x20 */ int blobDistance;  /* distance between two external blobs (ChloeCrisp) */
	/* 0x24 */ int cryptResult;  /* crypt() result (ChloeMelon) */
	/* 0x28 */ int sysmipsArg;  /* sysmips() argument (ChloePudding) */
	/* 0x2C */ ChloeFn openFn;
	/* 0x30 */ ChloeFn readFn;
	/* 0x34 */ ChloeFn writeFn;
	/* 0x38 */ ChloeFn closeFn;
	/* 0x3C */ ChloeFn ioctlFn;
	/* 0x40 */ ChloeFn lseekFn;
	/* 0x44 */ ChloeFn lstatFn;
	/* 0x48 */ ChloeFn mallocFn;
	/* 0x4C */ ChloeFn freeFn;
	/* 0x50 */ ChloeFn memsetFn;
	/* 0x54 */ ChloeFn memcpyFn;
	/* 0x58 */ int pad_58[6];  /* not used by the Chloe* functions */
} ChloeObj; /* size = 0x70 */

#ifdef __cplusplus
extern "C" {
#endif

/* Blob boundary markers: only their addresses are used, to compute blob
   sizes by subtraction (ChloeUsotsuki - ChloeDoushite is the size of the
   encrypted blob run by ChloeMaple). */
extern char ChloeDoushite[];
extern char ChloeUsotsuki[];
extern char ChloeKnife[];
extern char ChloeFork[];

/* The MIPS_TASKFLAGS kernel call of ChloeLime and ChloePudding takes three
   arguments (the second is a pointer for a read); <sys/sysmips.h> declares
   four ints, so it is left unprototyped. */
#ifndef __cplusplus
extern int sysmips();
#endif

/* chloe.c */
int ChloeMaple(ChloeObj *p);
int ChloeRings(ChloeObj *p, int memSize);
/* chloe_berry.c */
int ChloeBerry(ChloeObj *p);
/* chloe_cake.c */
int ChloeCake(void *buf, int size, int nslots, int slots);
int ChloeSlices(ChloeObj *p, unsigned int addr);
int ChloePlum(ChloeObj *p, int value);
/* chloe_candy.c */
int ChloeCandy(ChloeObj *p);
/* chloe_crisp.c */
int ChloeCrisp(ChloeObj *p);
/* chloe_freezing.c */
int ChloeFreezing(ChloeObj *p);
/* chloe_grape.c */
int ChloeGrape(ChloeObj *p);
/* chloe_lemon.c */
int ChloeLemon(ChloeObj *p);
int ChloeSnacks(ChloeObj *p);
/* chloe_lime.c */
int ChloeLime(ChloeObj *p);
/* chloe_mandarin.c */
int ChloeMandarin(ChloeObj *p);
/* chloe_mango.c */
int ChloeMango(ChloeObj *p);
/* chloe_melon.c */
int ChloeMelon(ChloeObj *p);
/* chloe_milk.c */
int ChloeMilk(ChloeObj *p);
/* chloe_orange.c */
int ChloeOrange(ChloeObj *p, const char *name);
int ChloeApple(ChloeObj *p);
int ChloeChips(ChloeObj *p);
int ChloeYogurt(ChloeObj *p);
int ChloeWatermelon(ChloeObj *p);
/* chloe_peach.c */
int ChloePeach(ChloeObj *p, int propA, int propB);
int ChloeCocoa(ChloeObj *p, int propC, int propD);
/* chloe_pie.c */
int ChloePie(ChloeObj *p);
int ChloeTart(ChloeObj *p);
/* chloe_pineapple.c */
int ChloePineapple(ChloeObj *p);
/* chloe_pizza.c */
int ChloePizza(ChloeObj *p);
/* chloe_pudding.c */
int ChloePudding(ChloeObj *p);
void *ChloeBanana(const char *path, int *out_size);
/* chloe_pumpkin.c */
int ChloePumpkin(ChloeObj *p);
int ChloeCrumble(ChloeObj *p);
int ChloeTapioca(ChloeObj *p);
/* chloe_rolls.c */
int ChloeRolls(ChloeObj *p);
int ChloeChocolate(ChloeObj *p);
int ChloeMint(ChloeObj *p);
/* chloe_scone.c */
int ChloeScone(ChloeObj *p);
int ChloePekoe(ChloeObj *p);
/* chloe_sherbet.c */
int ChloeSherbet(ChloeObj *p);
/* chloe_strawberry.c */
int ChloeStrawberry(ChloeObj *p);
/* chloe_tea.c */
int ChloeTea(ChloeObj *p);
int ChloeCoffee(ChloeObj *p);
/* chloe_vanilla.c */
int ChloeVanilla(ChloeObj *p);

#ifdef __cplusplus
}
#endif

#endif /* CHLOE_H */
