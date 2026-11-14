//
// "ps2gs.h"
//
//

#ifndef _PS2GS_H_
#define _PS2GS_H_

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include <sys/ioctl.h>
#include <linux/ps2/dev.h>

#include "gs_vc.h"

//----------------------------------------------------------------------
// types
//----------------------------------------------------------------------
typedef struct {
	ps2_gssreg_dispfb	dispfb;
	ps2_gssreg_bgcolor	bgcolor;
} ps2_gs_dispenv __attribute__((aligned(16)));

typedef __u64 ps2_gsreg_addr;

typedef struct {
	ps2_gsreg_frame			frame1;
	ps2_gsreg_addr			frame1addr;
	//
	ps2_gsreg_zbuf			zbuf1;
	ps2_gsreg_addr			zbuf1addr;
	//
	ps2_gsreg_xyoffset		xyoffset1;
	ps2_gsreg_addr			xyoffset1addr;
	//
	ps2_gsreg_scissor		scissor1;
	ps2_gsreg_addr			scissor1addr;
	//
	ps2_gsreg_prmodecont	prmodecont;
	ps2_gsreg_addr			prmodecontaddr;
	//
	ps2_gsreg_colclamp		colclamp;
	ps2_gsreg_addr			colclampaddr;
	//
	ps2_gsreg_dthe			dthe;
	ps2_gsreg_addr			dtheaddr;
	//
	ps2_gsreg_test			test1;
	ps2_gsreg_addr			test1addr;
} ps2_gs_drawenv1 __attribute__((aligned(16)));

typedef struct {
	ps2_gsreg_frame			frame2;
	ps2_gsreg_addr			frame2addr;
	// 
	ps2_gsreg_zbuf			zbuf2;
	ps2_gsreg_addr			zbuf2addr;
	// 
	ps2_gsreg_xyoffset		xyoffset2;
	ps2_gsreg_addr			xyoffset2addr;
	// 
	ps2_gsreg_scissor		scissor2;
	ps2_gsreg_addr			scissor2addr;
	// 
	ps2_gsreg_prmodecont	prmodecont;
	ps2_gsreg_addr			prmodecontaddr;
	// 
	ps2_gsreg_colclamp		colclamp;
	ps2_gsreg_addr			colclampaddr;
	// 
	ps2_gsreg_dthe			dthe;
	ps2_gsreg_addr			dtheaddr;
	// 
	ps2_gsreg_test			test2;
	ps2_gsreg_addr			test2addr;
} ps2_gs_drawenv2 __attribute__((aligned(16)));

typedef struct {
	ps2_gsreg_test		testa;
	ps2_gsreg_addr		testaaddr;
	// 
	ps2_gsreg_prim		prim;
	ps2_gsreg_addr		primaddr;
	// 
	ps2_gsreg_rgbaq		rgbaq;
	ps2_gsreg_addr		rgbaqaddr;
	// 
	ps2_gsreg_xyz		xyz2a;
	ps2_gsreg_addr		xyz2aaddr;
	// 
	ps2_gsreg_xyz		xyz2b;
	ps2_gsreg_addr		xyz2baddr;
	// 
	ps2_gsreg_test		testb;
	ps2_gsreg_addr		testbaddr;
} ps2_gs_clear __attribute__((aligned(16)));

typedef struct {
	ps2_gsreg_texflush	texflush;
	ps2_gsreg_addr		texflushaddr;
	// 
	ps2_gsreg_tex1		tex11;
	ps2_gsreg_addr		tex11addr;
	// 
	ps2_gsreg_tex0		tex01;
	ps2_gsreg_addr		tex01addr;
	// 
	ps2_gsreg_clamp		clamp1;
	ps2_gsreg_addr		clamp1addr;
} ps2_gs_texenv __attribute__((aligned(16)));

typedef struct {
	ps2_gsreg_texflush	texflush;
	ps2_gsreg_addr		texflushaddr;
	// 
	ps2_gsreg_tex1		tex12;
	ps2_gsreg_addr		tex12addr;
	// 
	ps2_gsreg_tex0		tex02;
	ps2_gsreg_addr		tex02addr;
	// 
	ps2_gsreg_clamp		clamp2;
	ps2_gsreg_addr		clamp2addr;
} ps2_gs_texenv2 __attribute__((aligned(16)));

typedef struct {
	ps2_gsreg_alpha		alpha1;
	ps2_gsreg_addr		alpha1addr;
	// 
	ps2_gsreg_pabe		pabe;
	ps2_gsreg_addr		pabeaddr;
	// 
	ps2_gsreg_texa		texa;
	ps2_gsreg_addr		texaaddr;
	// 
	ps2_gsreg_fba		fba1;
	ps2_gsreg_addr		fba1addr;
} ps2_gs_alphaenv __attribute__((aligned(16)));

typedef struct {
	ps2_gsreg_alpha	alpha2;
	ps2_gsreg_addr		alpha2addr;
	// 
	ps2_gsreg_pabe		pabe;
	ps2_gsreg_addr		pabeaddr;
	// 
	ps2_gsreg_texa		texa;
	ps2_gsreg_addr		texaaddr;
	// 
	ps2_gsreg_fba		fba2;
	ps2_gsreg_addr		fba2addr;
} ps2_gs_alphaenv2 __attribute__((aligned(16)));

typedef struct {
	ps2_gs_dispenv		disp[2];
	ps2_giftag			giftag0;
	ps2_gs_drawenv1		draw0;
	ps2_gs_clear		clear0;
	ps2_giftag			giftag1;
	ps2_gs_drawenv1		draw1;
	ps2_gs_clear		clear1;
} ps2_gs_dbuff __attribute__((aligned(16)));

typedef struct {
	ps2_gs_dispenv		disp[2];
	ps2_giftag			giftag0;
	ps2_gs_drawenv1		draw01;
	ps2_gs_drawenv2		draw02;
	ps2_gs_clear		clear0;
	ps2_giftag			giftag1;
	ps2_gs_drawenv1		draw11;
	ps2_gs_drawenv2		draw12;
	ps2_gs_clear		clear1;
} ps2_gs_dbuffdc __attribute__((aligned(16)));

typedef struct {
	ps2_giftag			giftag;
	ps2_gsreg_finish	finish;
	ps2_gsreg_addr		finishaddr;
} ps2_gs_finish __attribute__((aligned(16)));

typedef struct {
	int fd_gs;
	int fd_event;
	int fd_event_open;
	int version;
	int interlace_mode;
	int out_mode;
	int ff_mode;
	int interlace_format;
	int resolution;
	int refresh_rate;
	int width;
	int height;
	int center_x;
	int center_y;
	int offset_x;
	int offset_y;
	float pixel_ratio; // w/h
	struct ps2_screeninfo oldinfo;
} ps2_gs_gparam;


//----------------------------------------------------------------------
// prototypes
//----------------------------------------------------------------------
int ps2_gs_open(int event_fd);
int ps2_gs_close(void);
int ps2_gs_reset(int mode, int inter, int omode, int ffmode, int resolution, int refreshRate);
int ps2_gs_start_display(int start);
int ps2_gs_set_alphaenv(ps2_gs_alphaenv *ap, int pabe);
int ps2_gs_set_alphaenv2(ps2_gs_alphaenv2 *ap, int pabe);
int ps2_gs_set_clear(ps2_gs_clear *cp, int ztest, int x, int y,
					 int w, int h, int r, int g, int b, int a, __u32 z);
int ps2_gs_set_clear2(ps2_gs_clear *cp, int ztest, int x, int y,
					 int w, int h, int r, int g, int b, int a, __u32 z);
int ps2_gs_sync_v(int mode);
int ps2_gs_set_dbuff(ps2_gs_dbuff *db, int psm, int w, int h, int ztest, int zpsm, int clear);
void ps2_gs_set_dbuff_dc(ps2_gs_dbuffdc *db, int psm, int w, int h, int ztest, int zpsm, int clear);
int ps2_gs_swap_dbuff(ps2_gs_dbuff *db, int id);
int ps2_gs_swap_dbuff_dc(ps2_gs_dbuffdc *db, int id);
void ps2_gs_set_half_offset(ps2_gs_drawenv1 *draw, int halfoff);
void ps2_gs_set_half_offset2(ps2_gs_drawenv2 *draw, int halfoff);
void ps2_gs_set_dispenv(ps2_gs_dispenv *disp, int psm, int w);
void ps2_gs_put_dispenv(ps2_gs_dispenv *disp);
int ps2_gs_set_drawenv(ps2_gs_drawenv1 *draw, int psm, int w, int h, int ztest, int zpsm);
int ps2_gs_set_drawenv2(ps2_gs_drawenv2 *draw, int psm, int w, int h, int ztest, int zpsm);
int ps2_gs_put_drawenv(ps2_giftag *giftag);
void ps2_gs_set_finish(ps2_gs_finish *p);
void ps2_gs_wait_finish(ps2_gs_finish *p);
ps2_gs_gparam *ps2_gs_get_gparam(void);
int ps2_gs_set_image(struct ps2_image *pImg, int fbp, int fbw, int psm, int x, int y, int w, int h, void *ptr);
int ps2_gs_load_image(struct ps2_image *lp);
int ps2_gs_store_image(struct ps2_image *sp);
int ps2_gs_set_texenv(ps2_gs_texenv *tp, int flush, struct ps2_image *image, int w, int h,
					  int tfx, int cbp, int cpsm, int cld, int filter);
int ps2_gs_set_texenv2(ps2_gs_texenv2 *tp, int flush, struct ps2_image *image, int w, int h,
					   int tfx, int cbp, int cpsm, int cld, int filter);
void ps2_gs_set_center(int cx, int cy);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // _PS2GS_H_
