//
// "gs_alpha.c"
//
/*
 	Copyright (C) 2001  Sony Computer Entertainment Inc.
 
  This file is subject to the terms and conditions of the GNU Library
  General Public License Version 2. See the file "COPYING.LIB" in the 
  main directory of this archive for more details.
*/

#include <stdio.h>
#include <stdlib.h>
#include "gs_internal.h"

//----------------------------------------------------------------------
int ps2_gs_set_alphaenv(ps2_gs_alphaenv *ap, int pabe)
{
	/* As*Cs + (1-As)*Cd */
	ap->alpha1addr = PS2_GS_ALPHA_1;
	*(__u64 *)&ap->alpha1 =
		PS2_GS_SETREG_ALPHA(PS2_GS_ALPHA_A_CS, PS2_GS_ALPHA_B_CD,
							PS2_GS_ALPHA_C_AS, PS2_GS_ALPHA_D_CD, 0);
	
	ap->pabeaddr = PS2_GS_PABE;
	*(__u64 *)&ap->pabe = pabe;
	
	ap->texaaddr = PS2_GS_TEXA;
	*(__u64 *)&ap->texa =
		PS2_GS_SETREG_TEXA(127, PS2_GS_TEXA_AEM_BLACKTHRU, 129);
	
	ap->fba1addr = PS2_GS_FBA_1;
	*(__u64 *)&ap->fba1 = PS2_GS_SETREG_FBA(0);
	
	return sizeof(ps2_gs_alphaenv) / 16;
}

//----------------------------------------------------------------------
int ps2_gs_set_alphaenv2(ps2_gs_alphaenv2 *ap, int pabe)
{
	/* As*Cs + (1-As)*Cd */
	ap->alpha2addr = PS2_GS_ALPHA_2;
	*(__u64 *)&ap->alpha2 =
		PS2_GS_SETREG_ALPHA(PS2_GS_ALPHA_A_CS, PS2_GS_ALPHA_B_CD,
							PS2_GS_ALPHA_C_AS, PS2_GS_ALPHA_D_CD, 0);
	
	ap->pabeaddr = PS2_GS_PABE;
	*(__u64 *)&ap->pabe = pabe;
	
	ap->texaaddr = PS2_GS_TEXA;
	*(__u64 *)&ap->texa =
		PS2_GS_SETREG_TEXA(127, PS2_GS_TEXA_AEM_BLACKTHRU, 129);
	
	ap->fba2addr = PS2_GS_FBA_2;
	*(__u64 *)&ap->fba2 = PS2_GS_SETREG_FBA(0);
	
	return sizeof(ps2_gs_alphaenv2) / 16;
}
