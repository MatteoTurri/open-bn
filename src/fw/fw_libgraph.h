/*
 * The libgraph packet helpers of fw.c (sceGsSetDef*). The environment packets
 * are the types of libps2dev's ps2gs.h, which lays the GS registers out as
 * libgraph does: each register takes one qword pair {data(64), address(64)},
 * and the functions return the packet size in qwords.
 */

#ifndef FW_LIBGRAPH_H
#define FW_LIBGRAPH_H

#include "ps2gs.h"

/* LoadImage packet: GIFtag (A+D, 4 registers) + BITBLTBUF/TRXPOS/TRXREG/TRXDIR
 * + a second GIFtag in IMAGE mode with NLOOP = qwords of pixel data. */
typedef unsigned int sceGsU128 __attribute__((mode(TI)));

typedef struct {
	unsigned long long NLOOP : 15;
	unsigned long long EOP : 1;
	unsigned long long pad16 : 16;
	unsigned long long id : 14;
	unsigned long long PRE : 1;
	unsigned long long PRIM : 11;
	unsigned long long FLG : 2;
	unsigned long long NREG : 4;
	unsigned long long REGS0 : 4;
	unsigned long long REGS1 : 4;
	unsigned long long REGS2 : 4;
	unsigned long long REGS3 : 4;
	unsigned long long REGS4 : 4;
	unsigned long long REGS5 : 4;
	unsigned long long REGS6 : 4;
	unsigned long long REGS7 : 4;
	unsigned long long REGS8 : 4;
	unsigned long long REGS9 : 4;
	unsigned long long REGS10 : 4;
	unsigned long long REGS11 : 4;
	unsigned long long REGS12 : 4;
	unsigned long long REGS13 : 4;
	unsigned long long REGS14 : 4;
	unsigned long long REGS15 : 4;
} sceGsGifTag;

#define SCEGS_CLEAR_TAG(tp)	(*(sceGsU128 *)(tp) = 0)

typedef struct sceGsLoadImage1 {
	sceGsGifTag giftag0;
	ps2_gsreg_bitbltbuf bitbltbuf;
	ps2_gsreg_addr bitbltbufaddr;
	ps2_gsreg_trxpos trxpos;
	ps2_gsreg_addr trxposaddr;
	ps2_gsreg_trxreg trxreg;
	ps2_gsreg_addr trxregaddr;
	ps2_gsreg_trxdir trxdir;
	ps2_gsreg_addr trxdiraddr;
	sceGsGifTag giftag1;
} sceGsLoadImage1;

int sceGsSetDefLoadImage(sceGsLoadImage1 *p, short fbp, short fbw, short psm, short x, short y, short w, short h);
short sceGszbufaddr(int psm, short w, short h);
int sceGsSetDefDrawEnv(ps2_gs_drawenv1 *draw, short psm, short w, short h, short ztest, short zpsm);
int sceGsSetDefClear(ps2_gs_clear *cp, short ztest, short x, short y, short w, short h, unsigned char r,
    unsigned char g, unsigned char b, unsigned char a, unsigned int z);
int sceGsSetDefTexEnv(ps2_gs_texenv *tp, short flush, short tbp0, short tbw, short psm, short w2, short h2, short tfx,
    short cbp, short cpsm, short cld, short filter);
int sceGsSetDefAlphaEnv(ps2_gs_alphaenv *ap, short pabe);
void sceGsSetDefDispEnv(void *out, short psm, short w, short h, short dx, short dy);
void sceGsSetDefDispEnvV(void *out, short psm, short w, short h, short dx, short dy);

#endif /* FW_LIBGRAPH_H */
