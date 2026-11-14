//
// "gs_internal.h"
//

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "ps2gs.h"

extern int __ps2_gs_zbuf_addr(int psm, int w, int h);
extern int __ps2_gs_zbuf_size(int psm, int w, int h);
//
extern void __ps2_gs_put_sreg(int reg, __u64 val);
extern __u64 __ps2_gs_get_sreg(int reg);
extern int __ps2_gs_dma(const struct ps2_packet *packet);
extern void __ps2_gs_set_screen_size(int w, int h);
extern int __ps2_gs_image_size(struct ps2_image *lp);
