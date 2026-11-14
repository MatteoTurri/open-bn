/*
  ps2vpu.h: 
 
 	Copyright (C) 2000  Sony Computer Entertainment Inc.
 
  This file is subject to the terms and conditions of the GNU Library
  General Public License Version 2. See the file "COPYING.LIB" in the 
  main directory of this archive for more details.
*/

#ifndef _PS2VPU_H
#define _PS2VPU_H

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

//----------------------------------------------------------------------
typedef struct ps2_vpu_struct ps2_vpu;

//----------------------------------------------------------------------
// prototypes
//----------------------------------------------------------------------
void ps2_vpu_reset(ps2_vpu *dev);
void ps2_vpu_start(ps2_vpu *dev, unsigned int start);
int ps2_vpu_busy(ps2_vpu *dev);

ps2_vpu *ps2_vpu_open(int vpu);
int ps2_vpu_close(ps2_vpu *pvpu);

void *ps2_vpu_text(ps2_vpu *pvpu);
void *ps2_vpu_data(ps2_vpu *pvpu);
int ps2_vpu_text_size(ps2_vpu *pvpu);
int ps2_vpu_data_size(ps2_vpu *pvpu);
int ps2_vpu_fd(ps2_vpu *pvpu);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif /* _PS2VPU_H */
