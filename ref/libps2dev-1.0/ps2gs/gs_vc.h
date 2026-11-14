/* gs_vc.h */

#ifndef _GS_VC_H
#define _GS_VC_H

int ps2_gs_vc_graphicsmode(void);
int ps2_gs_vc_textmode(void);
int ps2_gs_vc_enablevcswitch(int (*acqfunc)(void), int (*relfunc)(void));

#define PS2_GS_VC_ACQ_FAILURE		-1
#define PS2_GS_VC_ACQ_SUCCESS		0

#define PS2_GS_VC_REL_FAILURE		-1
#define PS2_GS_VC_REL_SUCCESS		0
#define PS2_GS_VC_REL_SUCCESS_BG	1

int ps2_gs_vc_disablevcswitch(void);
void ps2_gs_vc_lock(void);
void ps2_gs_vc_unlock(void);

#endif /* _GS_VC_H */
