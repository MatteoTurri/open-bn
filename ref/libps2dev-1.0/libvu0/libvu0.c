/*
 	Copyright (C) 2001  Sony Computer Entertainment Inc.
 
  This file is subject to the terms and conditions of the GNU Library
  General Public License Version 2. See the file "COPYING.LIB" in the 
  main directory of this archive for more details.
*/

#include"libvu0.h"

//==========================================================================
// ps2_vu0_apply_matrix - Multiply matrix by vector
// SYNOPSIS
// 	void ps2_vu0_apply_matrix(ps2_vu0_fvector v0, ps2_vu0_fmatrix m0, ps2_vu0_fvector v1)
// ARGUMENTS
// 	v1		IN:vector
// 	m0		IN:matrix
// 	v0		OUT:vector
// 
// DESCRIPTION
// 	マトリックスmにベクトルv0を右から乗算して、v1に与える
// 
// 		v0=m0*v1
// RETURN_VALUE
// 	なし
//-------------------------------------------------------------------------- 	

void ps2_vu0_apply_matrix(ps2_vu0_fvector v0, ps2_vu0_fmatrix m0,ps2_vu0_fvector v1)
{
	asm __volatile__("
	lqc2    vf4,0x0(%1)
	lqc2    vf5,0x10(%1)
	lqc2    vf6,0x20(%1)
	lqc2    vf7,0x30(%1)
	lqc2    vf8,0x0(%2)
	vmulax.xyzw	ACC,   vf4,vf8
	vmadday.xyzw	ACC,   vf5,vf8
	vmaddaz.xyzw	ACC,   vf6,vf8
	vmaddw.xyzw	vf9,vf7,vf8
	sqc2    vf9,0x0(%0)
	": : "r" (v0) , "r" (m0) ,"r" (v1));
}


//==========================================================================
// 
// ps2_vu0_mul_matrix - Multiply matrices
// SYNOPSIS
// 	void ps2_vu0_mul_matrix(ps2_vu0_fmatrix m0, ps2_vu0_fmatrix m1, ps2_vu0_fmatrix m2)
// ARGUMENTS
// 	m1,m2		IN:matrix
// 	m0		OUT:matrix
// DESCRIPTION
// 	マトリックスm1にマトリックスm2を右から乗算して、m0に与える
// 
// 		m0=m1*m2
// RETURN_VALUE
// 	なし
// 
//-------------------------------------------------------------------------- 	

void ps2_vu0_mul_matrix(ps2_vu0_fmatrix m0, ps2_vu0_fmatrix m1,ps2_vu0_fmatrix m2)
{

	asm __volatile__("
	lqc2    vf4,0x0(%2)
	lqc2    vf5,0x10(%2)
	lqc2    vf6,0x20(%2)
	lqc2    vf7,0x30(%2)
	li    $7,4
1:
	lqc2    vf8,0x0(%1)
	vmulax.xyzw	ACC,   vf4,vf8
	vmadday.xyzw	ACC,   vf5,vf8
	vmaddaz.xyzw	ACC,   vf6,vf8
	vmaddw.xyzw	vf9,vf7,vf8
	sqc2    vf9,0x0(%0)
	addi    $7,-1
	addi    %1,0x10
	addi    %0,0x10
	bne    $0,$7,1b
	":: "r" (m0), "r" (m2), "r" (m1) : "$7");
}

//==========================================================================
// 
// ps2_vu0_outer_product - Outer product of vectors
// SYNOPSIS
// 	void ps2_vu0_outer_product(ps2_vu0_fvector v0, ps2_vu0_fvector v1, ps2_vu0_fvector v2)
// ARGUMENTS
// 	v1,v2		IN: vector
//	v0		OUT: vector
// DESCRIPTION
// 	ベクトルv1,v2の外積を求めてv0に与える
// RETURN_VALUE
// 	なし
// 
//-------------------------------------------------------------------------- 	
 
void ps2_vu0_outer_product(ps2_vu0_fvector v0, ps2_vu0_fvector v1, ps2_vu0_fvector v2)
{
	asm __volatile__
	("
	lqc2    vf4,0x0(%1)
	lqc2    vf5,0x0(%2)
	vopmula.xyz	ACC,vf4,vf5
	vopmsub.xyz	vf6,vf5,vf4
	vsub.w vf6,vf6,vf6		#vf6.xyz=0;
	sqc2    vf6,0x0(%0)
	": : "r" (v0) , "r" (v1) ,"r" (v2));
}

//==========================================================================
// 
// ps2_vu0_inner_product - Inner product of vectors
// SYNOPSIS
// 	float ps2_vu0_inner_product(ps2_vu0_fvector v0, ps2_vu0_fvector v1)
// ARGUMENTS
// 	v0,v1		IN: vector
// DESCRIPTION
// 	ベクトルv0,v1の内積を求める
// RETURN_VALUE
// 	内積
// 
//-------------------------------------------------------------------------- 	

float ps2_vu0_inner_product(ps2_vu0_fvector v0, ps2_vu0_fvector v1)
{
	register float ret;

	asm __volatile__("
	lqc2    vf4,0x0(%1)
	lqc2    vf5,0x0(%2)
	vmul.xyz vf5,vf4,vf5
	vaddy.x vf5,vf5,vf5
	vaddz.x vf5,vf5,vf5
	qmfc2   $2 ,vf5
	mtc1    $2,%0
	": "=f" (ret) :"r" (v0) ,"r" (v1) :"$2" );
	return ret;
}

//==========================================================================
// ps2_vu0_normalize - Normalize vector
// SYNOPSIS
// 	void ps2_vu0_normalize(ps2_vu0_fvector v0, ps2_vu0_fvector v1)
// ARGUMENTS
// 	v1		IN:vector
// 	v0		OUT:vector
// DESCRIPTION
// 	ベクトルv0の正規化を行いv1に与える
// RETURN_VALUE
// 	なし
// 
//-------------------------------------------------------------------------- 	
void ps2_vu0_normalize(ps2_vu0_fvector v0, ps2_vu0_fvector v1)
{
#if 0
	asm __volatile__("
	lqc2    vf4,0x0(%1)
	vmul.xyz vf5,vf4,vf4
	vaddy.x vf5,vf5,vf5
	vaddz.x vf5,vf5,vf5

	vrsqrt Q,vf0w,vf5x
	vsub.xyzw vf6,vf0,vf0		#vf6.xyzw=0;
	vaddw.xyzw vf6,vf6,vf4		#vf6.w=vf4.w;
	vwaitq

	vmulq.xyz  vf6,vf4,Q
	sqc2    vf6,0x0(%0)
	": : "r" (v0) , "r" (v1));

#else   //rsqrt bug 

        asm __volatile__("
        lqc2    vf4,0x0(%1)
        vmul.xyz vf5,vf4,vf4
        vaddy.x vf5,vf5,vf5
        vaddz.x vf5,vf5,vf5

        vsqrt Q,vf5x
        vwaitq
        vaddq.x vf5x,vf0x,Q
        vnop
        vnop
        vdiv    Q,vf0w,vf5x
        vsub.xyzw vf6,vf0,vf0           #vf6.xyzw=0;
        vwaitq

        vmulq.xyz  vf6,vf4,Q
        sqc2    vf6,0x0(%0)
        ": : "r" (v0) , "r" (v1));
#endif
}


//==========================================================================
// 
// ps2_vu0_transpose_matrix - Transpose matrix
// SYNOPSIS
// 	void ps2_vu0_transpose_matrix(ps2_vu0_fmatrix m0, ps2_vu0_fmatrix m1)
// ARGUMENTS
// 	m1		IN:matrix
// 	m0		OUT:matrix
// DESCRIPTION
// 	マトリックスm1の転置行列を求めm0に与える
// RETURN_VALUE
// 	なし
// 
//-------------------------------------------------------------------------- 	

void ps2_vu0_transpose_matrix(ps2_vu0_fmatrix m0, ps2_vu0_fmatrix m1)
{
	asm __volatile__("
	lq $8,0x0000(%1)
	lq $9,0x0010(%1)
	lq $10,0x0020(%1)
	lq $11,0x0030(%1)

	pextlw     $12,$9,$8
	pextuw     $13,$9,$8
	pextlw     $14,$11,$10
	pextuw     $15,$11,$10

	pcpyld     $8,$14,$12
	pcpyud     $9,$12,$14
	pcpyld     $10,$15,$13
	pcpyud     $11,$13,$15

	sq $8,0x0000(%0)
	sq $9,0x0010(%0)
	sq $10,0x0020(%0)
	sq $11,0x0030(%0)
	": : "r" (m0) , "r" (m1):"$8","$9","$10","$11","$12","$13","$14","$15");
}


//==========================================================================
//ps2_vu0_inverse_matrix	- Inverse normalized matrix
// SYNOPSIS
//	void ps2_vu0_inverse_matrix(ps2_vu0_fmatrix m0, ps2_vu0_fmatrix m1);
// ARGUMENTS
// 	m1		IN:matrix(回転 & 平行移動)
// 	m0		OUT:matrix
// DESCRIPTION
// 	マトリックスm1の逆行列を求めて、マトリックスm0に与える
//	逆行列 (4x4) を求める。（行列は正則と仮定する）
// RETURN_VALUE
// 	なし
// 
//-------------------------------------------------------------------------- 	

void ps2_vu0_inverse_matrix(ps2_vu0_fmatrix m0, ps2_vu0_fmatrix m1)
{
	asm __volatile__("
	lq $8,0x0000(%1)
	lq $9,0x0010(%1)
	lq $10,0x0020(%1)
	lqc2 vf4,0x0030(%1)

	vmove.xyzw vf5,vf4
	vsub.xyz vf4,vf4,vf4		#vf4.xyz=0;
	vmove.xyzw vf9,vf4
	qmfc2    $11,vf4

	#転置
	pextlw     $12,$9,$8
	pextuw     $13,$9,$8
	pextlw     $14,$11,$10
	pextuw     $15,$11,$10
	pcpyld     $8,$14,$12
	pcpyud     $9,$12,$14
	pcpyld     $10,$15,$13

	qmtc2    $8,vf6
	qmtc2    $9,vf7
	qmtc2    $10,vf8

	#内積
	vmulax.xyz	ACC,   vf6,vf5
	vmadday.xyz	ACC,   vf7,vf5
	vmaddz.xyz	vf4,vf8,vf5
	vsub.xyz	vf4,vf9,vf4

	sq $8,0x0000(%0)
	sq $9,0x0010(%0)
	sq $10,0x0020(%0)
	sqc2 vf4,0x0030(%0)
	": : "r" (m0) , "r" (m1):"$8","$9","$10","$11","$12","$13","$14","$15");

}


//==========================================================================
//ps2_vu0_div_vector	v0 = v1/q  (DIV + MULq)
// SYNOPSIS
//	void ps2_vu0_div_vector(ps2_vu0_fvector v0, ps2_vu0_fvector v1, float q)
// ARGUMENTS
// 	v1		IN:vector
// 	q		IN:scalar
// 	v0		OUT:vector
// DESCRIPTION
// 	ベクトルv1をスカラーqで除算して、ベクトルv0に与える
// RETURN_VALUE
// 	なし
//
//-------------------------------------------------------------------------- 	
void ps2_vu0_div_vector(ps2_vu0_fvector v0, ps2_vu0_fvector v1, float q)
{
	asm __volatile__("
	lqc2    vf4,0x0(%1)
	mfc1    $8,%2
	qmtc2    $8,vf5
	vdiv    Q,vf0w,vf5x
	vwaitq
	vmulq.xyzw      vf4,vf4,Q
	sqc2    vf4,0x0(%0)
	": : "r" (v0) , "r" (v1), "f" (q):"$8");
}

//==========================================================================
//ps2_vu0_div_vector_xyz	v0.xyz = v1/q (w を保存したい除算)
// SYNOPSIS
//	void ps2_vu0_div_vector_xyz(ps2_vu0_fvector v0, ps2_vu0_fvector v1, float q)
// ARGUMENTS
// 	v1		IN:vector
// 	q		IN:scalar
// 	v0		OUT:vector
// DESCRIPTION
// 	ベクトルv1のx,yの要素をスカラqで除算して、ベクトルv0に与える
// RETURN_VALUE
// 	なし
// 
//-------------------------------------------------------------------------- 	

void ps2_vu0_div_vector_xyz(ps2_vu0_fvector v0, ps2_vu0_fvector v1, float q)
{
	asm __volatile__("
	lqc2    vf4,0x0(%1)
	mfc1    $8,%2
	qmtc2    $8,vf5

	vdiv    Q,vf0w,vf5x
	vwaitq
	vmulq.xyz      vf4,vf4,Q

	sqc2    vf4,0x0(%0)
	": : "r" (v0) , "r" (v1), "f" (q) : "$8");
}

//==========================================================================
//ps2_vu0_inter_vector	内挿 
// SYNOPSIS
//	void ps2_vu0_inter_vector(ps2_vu0_fvector v0, ps2_vu0_fvector v1, ps2_vu0_fvector v2, float t)
// ARGUMENTS
// 	v1,v2		IN:vector
//	t		入力:内挿値	
// 	v0		OUT:vector
// DESCRIPTION
// 	// v0 = v1*t + v2*(1-t) （内挿)
// RETURN_VALUE
// 	なし
//
//-------------------------------------------------------------------------- 	
void ps2_vu0_inter_vector(ps2_vu0_fvector v0, ps2_vu0_fvector v1, ps2_vu0_fvector v2, float t)
{
	asm __volatile__("
	lqc2    vf4,0x0(%1)
	lqc2    vf5,0x0(%2)
	mfc1    $8,%3
	qmtc2    $8,vf6

        vaddw.x    vf7,vf0,vf0	#vf7.x=1;
        vsub.x    vf8,vf7,vf6	#vf8.x=1-t;
	vmulax.xyzw	ACC,   vf4,vf6
	vmaddx.xyzw	vf9,vf5,vf8

	sqc2    vf9,0x0(%0)
	": : "r" (v0) , "r" (v1), "r" (v2), "f" (t) : "$8");
}

//==========================================================================
//ps2_vu0_add_vector - Addtion of vectors
// SYNOPSIS
//	void ps2_vu0_add_vector(ps2_vu0_fvector v0, ps2_vu0_fvector v1, ps2_vu0_fvector v2)
// ARGUMENTS
// 	v1,v2		IN:vector
// 	v0		OUT:vector
// DESCRIPTION
//	4並列加算 (ADD/xyzw)
// 	ベクトルv1の各要素とベクトルv2の各要素を各々加算する。
// RETURN_VALUE
// 	なし
// 
//-------------------------------------------------------------------------- 	
void ps2_vu0_add_vector(ps2_vu0_fvector v0, ps2_vu0_fvector v1, ps2_vu0_fvector v2)
{
	asm __volatile__("
	lqc2    vf4,0x0(%1)
	lqc2    vf5,0x0(%2)
	vadd.xyzw	vf6,vf4,vf5
	sqc2    vf6,0x0(%0)
	": : "r" (v0) , "r" (v1), "r" (v2));
}

//==========================================================================
//ps2_vu0_sub_vector - Subtraction of vectors
// SYNOPSIS
//	void ps2_vu0_sub_vector(ps2_vu0_fvector v0, ps2_vu0_fvector v1, ps2_vu0_fvector v2)
// ARGUMENTS
// 	v1,v2		IN:vector
// 	v0		OUT:vector
// DESCRIPTION
//	4並列減算 (SUB/xyzw)
// 	ベクトルv1の各要素とベクトルv2の各要素を各々減算する。
// RETURN_VALUE
// 	なし
// 
//-------------------------------------------------------------------------- 	

void ps2_vu0_sub_vector(ps2_vu0_fvector v0, ps2_vu0_fvector v1, ps2_vu0_fvector v2)
{
	asm __volatile__("
	lqc2    vf4,0x0(%1)
	lqc2    vf5,0x0(%2)
	vsub.xyzw	vf6,vf4,vf5
	sqc2    vf6,0x0(%0)
	": : "r" (v0) , "r" (v1), "r" (v2));
}

//==========================================================================
//ps2_vu0_mul_vector - Multiply corresponding elements in vectors
// SYNOPSIS
//	void ps2_vu0_mul_vector(ps2_vu0_fvector v0, ps2_vu0_fvector v1, ps2_vu0_fvector v2)
// ARGUMENTS
// 	v1,v2		IN:vector
// 	v0		OUT:vector
// DESCRIPTION
//	4並列積算（MUL/xyzw: 内積ではないことに注意）
// 	ベクトルv1の各要素とベクトルv2の各要素を各々乗算する。
// RETURN_VALUE
// 	なし
// 
//-------------------------------------------------------------------------- 	

void ps2_vu0_mul_vector(ps2_vu0_fvector v0, ps2_vu0_fvector v1, ps2_vu0_fvector v2)
{
	asm __volatile__("
	lqc2    vf4,0x0(%1)
	lqc2    vf5,0x0(%2)
	vmul.xyzw	vf6,vf4,vf5
	sqc2    vf6,0x0(%0)
	": : "r" (v0) , "r" (v1), "r" (v2));
}

//==========================================================================
//ps2_vu0_scale_vector - Multiply vector by scalar
// SYNOPSIS
//	void ps2_vu0_scale_vector(ps2_vu0_fvector v0, ps2_vu0_fvector v1, float t)
// ARGUMENTS
// 	v1		IN:vector
// 	t		IN:scalar
// 	v0		OUT:vector
// DESCRIPTION
// 	ベクトルv1にスカラーtを乗算してベクトルv0に与える
//	スカラー値を乗算する (MULx/xyzw) 
// RETURN_VALUE
// 	なし
//
//-------------------------------------------------------------------------- 	

void ps2_vu0_scale_vector(ps2_vu0_fvector v0, ps2_vu0_fvector v1, float t)
{
	asm __volatile__("
	lqc2    vf4,0x0(%1)
	mfc1    $8,%2
	qmtc2    $8,vf5
	vmulx.xyzw	vf6,vf4,vf5
	sqc2    vf6,0x0(%0)
	": : "r" (v0) , "r" (v1), "f" (t):"$8");
}

//==========================================================================
//ps2_vu0_trans_matrix	行列の平行移動
// SYNOPSIS
//	void ps2_vu0_trans_matrix(ps2_vu0_fmatrix m0, ps2_vu0_fmatrix m1, ps2_vu0_fvector tv);
// ARGUMENTS
// 	tv		入力:移動ベクトル
// 	m1		IN:matrix
// 	m0		OUT:matrix
// DESCRIPTION
// 	与えられたベクトル分、マトリックスを平行移動する
// RETURN_VALUE
// 	なし
// 
//-------------------------------------------------------------------------- 	
void ps2_vu0_trans_matrix(ps2_vu0_fmatrix m0, ps2_vu0_fmatrix m1, ps2_vu0_fvector tv)
{
	asm __volatile__("
	lqc2    vf4,0x0(%2)
	lqc2    vf5,0x30(%1)

	lq    $7,0x0(%1)
	lq    $8,0x10(%1)
	lq    $9,0x20(%1)
	vadd.xyz	vf5,vf5,vf4
	sq    $7,0x0(%0)
	sq    $8,0x10(%0)
	sq    $9,0x20(%0)
	sqc2    vf5,0x30(%0)
	": : "r" (m0) , "r" (m1), "r" (tv):"$7","$8","$9");
}

//==========================================================================
//ps2_vu0_copy_vector - Copy vector
// SYNOPSIS
//	void ps2_vu0_copy_vector(ps2_vu0_fvector v0, ps2_vu0_fvector v1);
// ARGUMENTS
// 	m1		IN:matrix
// 	m0		OUT:matrix
// DESCRIPTION
// 	マトリックスm1をマトリックスm0にコピーする。
//	(lq と sq の組合せ。alignment check 用に)
// RETURN_VALUE
// 	なし
// 
//-------------------------------------------------------------------------- 	

void ps2_vu0_copy_vector(ps2_vu0_fvector v0, ps2_vu0_fvector v1)
{
	asm __volatile__("
	lq    $6,0x0(%1)
	sq    $6,0x0(%0)
	": : "r" (v0) , "r" (v1):"$6");
}


//==========================================================================
//ps2_vu0_copy_matrix - Copy matrix
// SYNOPSIS
//	void ps2_vu0_copy_matrix(ps2_vu0_fmatrix m0, ps2_vu0_fmatrix m1);
// ARGUMENTS
// 	m1		IN:matrix
// 	m0		OUT:matrix
// DESCRIPTION
// 	マトリックスm1をマトリックスm0にコピーする。
//	(lq と sq の組合せ。alignment check 用に)
// RETURN_VALUE
// 	なし
// 
//-------------------------------------------------------------------------- 	

void ps2_vu0_copy_matrix(ps2_vu0_fmatrix m0, ps2_vu0_fmatrix m1)
{
	asm __volatile__("
	lq    $6,0x0(%1)
	lq    $7,0x10(%1)
	lq    $8,0x20(%1)
	lq    $9,0x30(%1)
	sq    $6,0x0(%0)
	sq    $7,0x10(%0)
	sq    $8,0x20(%0)
	sq    $9,0x30(%0)
	": : "r" (m0) , "r" (m1):"$6","$7","$8","$9");
}

//==========================================================================
// ps2_vu0_ftoi4_vector	FTOI4
// SYNOPSIS
//	void ps2_vu0_ftoi4_vector(ps2_vu0_ivector v0, ps2_vu0_fvector v1);
// ARGUMENTS
// 	v1		IN:vector
// 	v0		OUT:vector
// DESCRIPTION
// 	与えられたベクトルの要素を整数に変換する
// RETURN_VALUE
// 	なし
// 
//-------------------------------------------------------------------------- 	

void ps2_vu0_ftoi4_vector(ps2_vu0_ivector v0, ps2_vu0_fvector v1)
{
	asm __volatile__("
	lqc2    vf4,0x0(%1)
	vftoi4.xyzw	vf5,vf4
	sqc2    vf5,0x0(%0)
	": : "r" (v0) , "r" (v1));
}

//==========================================================================
//ps2_vu0_ftoi0_vector 	FTOI0
// SYNOPSIS
//	void ps2_vu0_ftoi0_vector(ps2_vu0_ivector v0, ps2_vu0_fvector v1);
// ARGUMENTS
// 	v1		IN:vector
// 	v0		OUT:vector
// DESCRIPTION
// 	与えられたベクトルの要素を整数に変換する
// RETURN_VALUE
// 	なし
// 
//-------------------------------------------------------------------------- 	
void ps2_vu0_ftoi0_vector(ps2_vu0_ivector v0, ps2_vu0_fvector v1)
{
	asm __volatile__("
	lqc2    vf4,0x0(%1)
	vftoi0.xyzw	vf5,vf4
	sqc2    vf5,0x0(%0)
	": : "r" (v0) , "r" (v1));
}


//==========================================================================
// ps2_vu0_itof4_vector	ITOF4
// SYNOPSIS
//	void ps2_vu0_itof4_vector(ps2_vu0_fvector v0, ps2_vu0_ivector v1);
// ARGUMENTS
// 	v1		IN:vector
// 	v0		OUT:vector
// DESCRIPTION
// 	与えられたベクトルの要素を整数に変換する
// RETURN_VALUE
// 	なし
// 
//-------------------------------------------------------------------------- 	

void ps2_vu0_itof4_vector(ps2_vu0_fvector v0, ps2_vu0_ivector v1)
{
	asm __volatile__("
	lqc2    vf4,0x0(%1)
	vitof4.xyzw	vf5,vf4
	sqc2    vf5,0x0(%0)
	": : "r" (v0) , "r" (v1));
}

//==========================================================================
//ps2_vu0_itof0_vector 	ITOF0
// SYNOPSIS
//	void ps2_vu0_itof0_vector(ps2_vu0_fvector v0, ps2_vu0_ivector v1);
// ARGUMENTS
// 	v1		IN:vector
// 	v0		OUT:vector
// DESCRIPTION
// 	与えられたベクトルの要素を整数に変換する
// RETURN_VALUE
// 	なし
// 
//-------------------------------------------------------------------------- 	
void ps2_vu0_itof0_vector(ps2_vu0_fvector v0, ps2_vu0_ivector v1)
{
	asm __volatile__("
	lqc2    vf4,0x0(%1)
	vitof0.xyzw	vf5,vf4
	sqc2    vf5,0x0(%0)
	": : "r" (v0) , "r" (v1));
}

//==========================================================================
//ps2_vu0_unit_matrix - Obtain unit matrix
// SYNOPSIS
//	void ps2_vu0_unit_matrix(ps2_vu0_fmatrix m0);
// ARGUMENTS
// 	m1		IN:matrix
// 	m0		OUT:matrix
// DESCRIPTION
// 	与えられた行列を単位行列に変換する
// RETURN_VALUE
// 	なし
// 
//-------------------------------------------------------------------------- 	

void ps2_vu0_unit_matrix(ps2_vu0_fmatrix m0)
{
	asm __volatile__("
	vsub.xyzw	vf4,vf0,vf0 #vf4.xyzw=0;
	vadd.w	vf4,vf4,vf0
	vmr32.xyzw	vf5,vf4
	vmr32.xyzw	vf6,vf5
	vmr32.xyzw	vf7,vf6
	sqc2    vf4,0x30(%0)
	sqc2    vf5,0x20(%0)
	sqc2    vf6,0x10(%0)
	sqc2    vf7,0x0(%0)
	": : "r" (m0));
}


//==========================================================================
// SYNOPSIS
//	void _ps2_vu0_ecossin(float x)
// ARGUMENTS
// DESCRIPTION
// 	sin,cosを求める
//	有効範囲:-PI/2 ... PI/2
//	Helper of RotMatrix?()
// RETURN_VALUE
// 
//-------------------------------------------------------------------------- 	

asm ("
	.rdata
 	.align 4
	.local ps2_vu0_ecossin_data
	.type    ps2_vu0_ecossin_data@object
        .size    ps2_vu0_ecossin_data,16
vu0ecossin_data:
 	.word	0x362e9c14, 0xb94fb21f, 0x3c08873e, 0xbe2aaaa4
	.previous
");


inline
static void _ps2_vu0_ecossin(void)
{	
	asm __volatile__("
	la	$8,vu0ecossin_data	# S5-S2 の係数を VF05 へ転送
	lqc2	vf05,0x0($8)	#
	vmr32.w vf06,vf06	# VF06.x(v)をVF06.w にコピー
	vaddx.x vf04,vf00,vf06	# VF06.x(v)をVF04.x にコピー
	vmul.x vf06,vf06,vf06	# VF06.x を自乗して v^2 にする
	vmulx.yzw vf04,vf04,vf00# VF04.yzw=0
	vmulw.xyzw vf08,vf05,vf06# S2-S5 に VF06.w(v) をかける
	vsub.xyzw vf05,vf00,vf00 #0,0,0,0 |x,y,z,w
	vmulx.xyzw vf08,vf08,vf06# VF06.x(v^2) をかける
	vmulx.xyz vf08,vf08,vf06# VF06.x(v^2) をかける
	vaddw.x vf04,vf04,vf08	# s+=k2
	vmulx.xy vf08,vf08,vf06	# VF06.x(v^2) をかける
	vaddz.x vf04,vf04,vf08	# s+=z
	vmulx.x vf08,vf08,vf06	# VF06.x(v^2) をかける
	vaddy.x vf04,vf04,vf08	# s+=y
	vaddx.x vf04,vf04,vf08	# s+=x (sin 完了)

	vaddx.xy vf04,vf05,vf04	# .xy=s 追加 

	vmul.x vf07,vf04,vf04	# VF07.x=s*s
	vsubx.w vf07,vf00,vf07	# VF07.w = 1-s*s

	vsqrt Q,vf07w		# Q= √|1-s*s| (cos 完了)
	vwaitq

	vaddq.x vf07,vf00,Q     # vf07.x=c

	bne	$7,$0,1f
	vaddx.x vf04,vf05,vf07	# VF04.x=s
	b	2f
1:	
	vsubx.x vf04,vf05,vf07	# VF04.x=s
2:	

	": : :"$7","$8");
}

//==========================================================================
//ps2_vu0_rot_matrix_z	Ｚ軸を中心とした行列の回転 
// SYNOPSIS
//	void ps2_vu0_rot_matrix_z(ps2_vu0_fmatrix m0, ps2_vu0_fmatrix m1, float rz);
// ARGUMENTS
// 	rz		入力:回転角(有効範囲:-PI ... PI)
// 	m1		IN:matrix
// 	m0		OUT:matrix
// DESCRIPTION
// 	回転角rtよりＸ軸を中心とした回転マトリックスを求めて、
//	マトリックスm1に左側から乗算して、その結果をマトリックス
//	m0に与える
// RETURN_VALUE
// 	なし
// 
//-------------------------------------------------------------------------- 	
void ps2_vu0_rot_matrix_z(ps2_vu0_fmatrix m0, ps2_vu0_fmatrix m1, float rz)
{
	asm __volatile__("
	mtc1	$0,$f4
	c.olt.s %2,$f4
	li.s    $f4,1.57079637050628662109e0	
	bc1f    1f
	add.s   %2,$f4,%2			#rx=rx+PI/2
	li 	$7,1				#cos(rx)=sin(rx+PI/2)
	b	2f
1:
	sub.s   %2,$f4,%2			#rx=PI/2-rx
	move	$7,$0
2:

        mfc1    $8,%2
        qmtc2    $8,vf6
	# %0 and %1 are dummy
	": : "r" (m0) , "r" (m1), "f" (rz):"$7","$8","$f4");

	_ps2_vu0_ecossin( /* arg: vf6 */ );

	asm __volatile__("
			#vf05:0,0,0,0 |x,y,z,w
	vmove.xyzw vf06,vf05
	vmove.xyzw vf07,vf05
	vmove.xyzw vf09,vf00
	vsub.xyz vf09,vf09,vf09 	#0,0,0,1 |x,y,z,w
	vmr32.xyzw vf08,vf09	#0,0,1,0 |x,y,z,w

	vsub.zw vf04,vf04,vf04 #vf04.zw=0 s,c,0,0 |x,y,z,w
	vaddx.y vf06,vf05,vf04 #vf06 0,s,0,0 |x,y,z,w
	vaddy.x vf06,vf05,vf04 #vf06 c,s,0,0 |x,y,z,w
	vsubx.x vf07,vf05,vf04 #vf07 -s,0,0,0 |x,y,z,w
	vaddy.y vf07,vf05,vf04 #vf07 -s,c,0,0 |x,y,z,w

	li    $7,4
10:
	lqc2    vf4,0x0(%1)
	vmulax.xyzw	ACC,   vf6,vf4
	vmadday.xyzw	ACC,   vf7,vf4
	vmaddaz.xyzw	ACC,   vf8,vf4
	vmaddw.xyzw	vf5,  vf9,vf4
	sqc2    vf5,0x0(%0)
	addi    $7,-1
	addi    %1,0x10
	addi    %0,0x10
	bne    $0,$7,10b
	# %2 is dummy
	": : "r" (m0) , "r" (m1), "f" (rz):"$7");
}


//==========================================================================
//ps2_vu0_rot_matrix_x	Ｘ軸を中心とした行列の回転 
// SYNOPSIS
//	void ps2_vu0_rot_matrix_x(ps2_vu0_fmatrix m0, ps2_vu0_fmatrix m1, float ry);
// ARGUMENTS
// 	rx		入力:回転角(有効範囲:-PI ... PI)
// 	m1		IN:matrix
// 	m0		OUT:matrix
// DESCRIPTION
// 	回転角rtよりＸ軸を中心とした回転マトリックスを求めて、
//	マトリックスm1に左側から乗算して、その結果をマトリックス
//	m0に与える
// RETURN_VALUE
// 	なし
// 
//-------------------------------------------------------------------------- 	
void ps2_vu0_rot_matrix_x(ps2_vu0_fmatrix m0, ps2_vu0_fmatrix m1, float rx)
{
	asm __volatile__("
	mtc1	$0,$f4
	c.olt.s %2,$f4
	li.s    $f4,1.57079637050628662109e0	
	bc1f    1f
	add.s   %2,$f4,%2			#rx=rx+PI/2
	li 	$7,1				#cos(rx)=sin(rx+PI/2)
	b	2f
1:
	sub.s   %2,$f4,%2			#rx=PI/2-rx
	move	$7,$0
2:

	mfc1    $8,%2
	qmtc2    $8,$vf6
	# %0 and %1 are dummy
	": : "r" (m0) , "r" (m1), "f" (rx):"$7","$8","$f4");

	_ps2_vu0_ecossin( /* arg: vf6 */ );

	asm __volatile__("
			#vf05:0,0,0,0 |x,y,z,w
	vmove.xyzw vf06,vf05	
	vmove.xyzw vf07,vf05
	vmove.xyzw vf08,vf05
	vmove.xyzw vf09,vf05
	vaddw.x vf06,vf05,vf00 #vf06 1,0,0,0 |x,y,z,w
	vaddw.w vf09,vf05,vf00 #vf09 0,0,0,1 |x,y,z,w

	vsub.zw vf04,vf04,vf04 #vf04.zw=0 s,c,0,0 |x,y,z,w
	vaddx.z vf07,vf05,vf04 #vf07.zw=0 0,0,s,0 |x,y,z,w
	vaddy.y vf07,vf05,vf04 #vf07.zw=0 0,c,s,0 |x,y,z,w
	vsubx.y vf08,vf05,vf04 #vf08.zw=0 0,-s,0,0 |x,y,z,w
	vaddy.z vf08,vf05,vf04 #vf08.zw=0 0,-s,c,0 |x,y,z,w

	li    $7,4
10:
	lqc2    vf4,0x0(%1)
	vmulax.xyzw	ACC,   vf6,vf4
	vmadday.xyzw	ACC,   vf7,vf4
	vmaddaz.xyzw	ACC,   vf8,vf4
	vmaddw.xyzw	vf5,  vf9,vf4
	sqc2    vf5,0x0(%0)
	addi    $7,-1
	addi    %1,0x10
	addi    %0,0x10
	bne    $0,$7,10b
	# %2 is dummy
	": : "r" (m0) , "r" (m1), "f" (rx):"$7");
}
//==========================================================================
//ps2_vu0_rot_matrix_y	Ｙ軸を中心とした行列の回転 
// SYNOPSIS
//	void ps2_vu0_rot_matrix_y(ps2_vu0_fmatrix m0, ps2_vu0_fmatrix m1, float ry);
// ARGUMENTS
// 	ry		入力:回転角(有効範囲:-PI ... PI)
// 	m1		IN:matrix
// 	m0		OUT:matrix
// DESCRIPTION
// 	回転角rtよりＹ軸を中心とした回転マトリックスを求めて、
//	マトリックスm1に左側から乗算して、その結果をマトリックス
//	m0に与える
// RETURN_VALUE
// 	なし
// 
//-------------------------------------------------------------------------- 	
void ps2_vu0_rot_matrix_y(ps2_vu0_fmatrix m0, ps2_vu0_fmatrix m1, float ry)
{
	asm __volatile__("
	mtc1	$0,$f4
	c.olt.s %2,$f4
	li.s    $f4,1.57079637050628662109e0	
	bc1f    1f
	add.s   %2,$f4,%2			#rx=rx+PI/2
	li 	$7,1				#cos(rx)=sin(rx+PI/2)
	b	2f
1:
	sub.s   %2,$f4,%2			#rx=PI/2-rx
	move	$7,$0
2:

	mfc1    $8,%2
	qmtc2    $8,vf6
	# %0 and %1 are dummy
	": : "r" (m0) , "r" (m1), "f" (ry):"$7","$8","$f4");

	_ps2_vu0_ecossin( /* arg: vf6 */ );

	asm __volatile__("
			#vf05:0,0,0,0 |x,y,z,w
	vmove.xyzw vf06,vf05   
	vmove.xyzw vf07,vf05
	vmove.xyzw vf08,vf05
	vmove.xyzw vf09,vf05
	vaddw.y vf07,vf05,vf00 #vf07 0,1,0,0 |x,y,z,w
	vaddw.w vf09,vf05,vf00 #vf09 0,0,0,1 |x,y,z,w

	vsub.zw vf04,vf04,vf04 #vf04.zw=0 s,c,0,0 |x,y,z,w
	vsubx.z vf06,vf05,vf04 #vf06 0,0,-s,0 |x,y,z,w
	vaddy.x vf06,vf05,vf04 #vf06 c,0,-s,0 |x,y,z,w
	vaddx.x vf08,vf05,vf04 #vf08 s,0,0,0 |x,y,z,w
	vaddy.z vf08,vf05,vf04 #vf08 s,0,c,0 |x,y,z,w

	li    $7,4
10:
	lqc2    vf4,0x0(%1)
	vmulax.xyzw	ACC,   vf6,vf4
	vmadday.xyzw	ACC,   vf7,vf4
	vmaddaz.xyzw	ACC,   vf8,vf4
	vmaddw.xyzw	vf5,  vf9,vf4
	sqc2    vf5,0x0(%0)
	addi    $7,-1
	addi    %1,0x10
	addi    %0,0x10
	bne    $0,$7,10b
	# %2 is dummy
	": : "r" (m0) , "r" (m1), "f" (ry):"$7");
}

//==========================================================================
//ps2_vu0_rot_matrix	行列の回転 
// SYNOPSIS
//	void ps2_vu0_rot_matrix(ps2_vu0_fmatrix m0, ps2_vu0_fmatrix m1, ps2_vu0_fvector rot);
// ARGUMENTS
// 	m0		OUT:matrix
// 	m1		IN:matrix
// 	rot		入力:x,y,z軸の回転角(有効範囲:-PI ... PI)
// DESCRIPTION
//	Z軸を中心とする回転マトリックスをrot[2]から、Y軸を中心とする回転マ
//	トリックスをrot[1]から、X軸を中心とする回転マトリックスをrot[0]から
//	それぞれ求め、順にマトリックスm1に左側から乗算して、その結果をマト
//	リックスm0に返します。
// RETURN_VALUE
// 	なし
// 
//-------------------------------------------------------------------------- 	
void ps2_vu0_rot_matrix(ps2_vu0_fmatrix m0, ps2_vu0_fmatrix m1, ps2_vu0_fvector rot)
{
	ps2_vu0_rot_matrix_z(m0, m1, rot[2]);
	ps2_vu0_rot_matrix_y(m0, m0, rot[1]);
	ps2_vu0_rot_matrix_x(m0, m0, rot[0]);
}

//==========================================================================
//ps2_vu0_clamp_vector	ベクトルのクランプ
// SYNOPSIS
//	void ps2_vu0_clamp_vector(ps2_vu0_fvector v0, ps2_vu0_FFVECTOR v1, float min, float max)
// ARGUMENTS
// 	v0		OUT:vector
// 	v1		IN:vector
// 	min		入力:最小値
// 	max		入力:最大値
// DESCRIPTION
//	ベクトルv1の各要素を、最小値min、最大値maxでクランプし、結果をベク
//	トルv0に返します。
// RETURN_VALUE
// 	なし
// 
//-------------------------------------------------------------------------- 	
void ps2_vu0_clamp_vector(ps2_vu0_fvector v0, ps2_vu0_fvector v1, float min, float max)
{
	asm __volatile__("
        mfc1    $8,%2
        mfc1    $9,%3
	lqc2    vf6,0x0(%1)
        qmtc2    $8,vf4
        qmtc2    $9,vf5
	vmaxx.xyzw $vf06,$vf06,$vf04
	vminix.xyzw $vf06,$vf06,$vf05
	sqc2    vf6,0x0(%0)
	": : "r" (v0) , "r" (v1), "f" (min), "f" (max):"$8","$9");
}

//==========================================================================
//ps2_vu0_camera_matrix	ワールドビュー行列の生成
// SYNOPSIS
//	void ps2_vu0_camera_matrix(ps2_vu0_fmatrix m, ps2_vu0_fvector p, ps2_vu0_fvector zd, ps2_vu0_fvector yd);
// ARGUMENTS
// 	m		OUT:matrix(ビューワールド座標)
// 	p		IN:matrix(視点)
// 	zd		IN:matrix(視線)
// 	yd		IN:matrix(垂直方向)
// DESCRIPTION
//	視点pを(0,0,0)に、視線zdを(0,0,1)に、垂直方向ydを(0,1,0)にと変換す
//	るような行列を求め、mに返します。
// RETURN_VALUE
// 	なし
// 
//-------------------------------------------------------------------------- 	
//  yake world screen matrix
//	p:  camera position (in the world)
//	zd: eye direction
//	yd: vertical direction
//
//	zd = zd/|zd|
//	xd = zd x yd; xd = xd/|xd|	(wing direction)
//	yd = yd x xd; yd = yd/|yd|	(real veritical direction)
//	
//	        | xd0 yd0 zd0 p0 |
//	M = inv | xd1 yd1 zd1 p1 |
//	        | xd2 yd2 zd2 p2 |
//	        |   0   0   0  1 |
//-------------------------------------------------------------------------- 	

void ps2_vu0_camera_matrix(ps2_vu0_fmatrix m, ps2_vu0_fvector p, ps2_vu0_fvector zd, ps2_vu0_fvector yd)
{
	ps2_vu0_fmatrix	m0;
	ps2_vu0_fvector	xd;

	ps2_vu0_unit_matrix(m0);			
	ps2_vu0_outer_product(xd, yd, zd);
	ps2_vu0_normalize(m0[0], xd);
	ps2_vu0_normalize(m0[2], zd);
	ps2_vu0_outer_product(m0[1], m0[2], m0[0]);
	ps2_vu0_trans_matrix(m0, m0, p);
	ps2_vu0_inverse_matrix(m, m0);
}
	

//==========================================================================
//ps2_vu0_normal_light_matrix         ノーマルライト行列の生成
// SYNOPSIS
//	void ps2_vu0_normal_light_matrix(ps2_vu0_fmatrix m, ps2_vu0_fvector l0, ps2_vu0_fvector l1, ps2_vu0_fvector l2);
// ARGUMENTS
// 	m		OUT:matrix
// 	l0		IN:matrix(光源0の方向)
// 	l1		IN:matrix(光源1の方向)
// 	l2		IN:matrix(光源2の方向)
// DESCRIPTION
//	光源l0、光源l1、光源l2からノーマルライト行列を求め、mに返します。
// RETURN_VALUE
// 	なし
// 
//-------------------------------------------------------------------------- 	
// make normal light matrix
//	l0-l2:	light direction
//	    | l0x l0y l0z 0 |   | 10r 10g 10b 0 |
//	M = | l1x l1y l1z 0 | = | 11r 11g 11b 0 |
//	    | l2x l2y l2z 0 |   | 12r 12g 12b 0 |
//	    |   0   0   0 1 |   |   0   0   0 1 |
//-------------------------------------------------------------------------- 	

void ps2_vu0_normal_light_matrix(ps2_vu0_fmatrix m, ps2_vu0_fvector l0, ps2_vu0_fvector l1, ps2_vu0_fvector l2)
{
        ps2_vu0_fvector  t;
        ps2_vu0_scale_vector(t, l0, -1); ps2_vu0_normalize(m[0], t);
        ps2_vu0_scale_vector(t, l1, -1); ps2_vu0_normalize(m[1], t);
        ps2_vu0_scale_vector(t, l2, -1); ps2_vu0_normalize(m[2], t);

	m[3][0] = m[3][1] = m[3][2] = 0.0f;
	m[3][3] = 1.0f;

	ps2_vu0_transpose_matrix(m, m);
}

//==========================================================================
//ps2_vu0_light_color_matrix		ライトカラー行列の生成
// SYNOPSIS
//	void ps2_vu0_light_color_matrix(ps2_vu0_fmatrix m,  ps2_vu0_fvector c0, ps2_vu0_fvector c1, ps2_vu0_fvector c2, ps2_vu0_fvector a)
// ARGUMENTS
// 	m		OUT:matrix
// 	c0		IN:vector(光源色0)
// 	c1		IN:vector(光源色1)
// 	c2		IN:vector(光源色2)
// 	a		IN:vector(アンビエント)
// DESCRIPTION
//	光源色c0,c1,c2と環境色aからライトカラー行列を求め、mに返します。
// RETURN_VALUE
// 	なし
// 
//-------------------------------------------------------------------------- 	
// make normal light matrix
//	c0-c2:	light color
//	a:	ambient color
//
//	    | c0x c1x c2x ax |   | c0r c1r c2r ar |
//	M = | c0y c1y c2y ay | = | c0g c1g c2g ag |
//	    | c0z c1z c2z az |   | c0b c1b c2b ab |
//	    |   0   0   0  0 |   |   0   0   0  0 |
//-------------------------------------------------------------------------- 	
void ps2_vu0_light_color_matrix(ps2_vu0_fmatrix m, ps2_vu0_fvector c0, ps2_vu0_fvector c1, ps2_vu0_fvector c2, ps2_vu0_fvector a)
{
	ps2_vu0_copy_vector(m[0], c0);
	ps2_vu0_copy_vector(m[1], c1);
	ps2_vu0_copy_vector(m[2], c2);
	ps2_vu0_copy_vector(m[3],  a);
}

//==========================================================================
//ps2_vu0_view_screen_matrix          ビュースクリーン行列の生成
// SYNOPSIS
//	void ps2_vu0_view_screen_matrix(ps2_vu0_fmatrix m, float scrz, float ax, float ay, float cx, float cy, float zmin,  float zmax,  float nearz, float farz)
// ARGUMENTS
// 	m		OUT:matrix
//	scrz            入力:(スクリーンまでの距離)
//	ax              入力:(Ｘ方向アスペクト比)
//	ay              入力:(Ｙ方向アスペクト比)
//	cx              入力:(スクリーンの中心Ｘ座標)
//	cy              入力:(スクリーンの中心Ｙ座標)
//	zmin            入力:(Ｚバッファ最小値)
//	zmax            入力:(Ｚバッファ最大値)
//	nearz           入力:(ニアクリップ面のＺ)
//	farz            入力:(ファークリップ面のＺ)
// DESCRIPTION
//	指定された各パラメータに従ってビュースクリーン行列を求め、mに返しま
//	す。
// RETURN_VALUE
// 	なし
//-------------------------------------------------------------------------- 	
// make view-screen matrix
//	scrz:		distance to screen
//	ax, ay:		aspect ratio
//	cx, cy:		center
//	zmin, zmax:	Z-buffer range
//	nearz, farz:	near, far edge
//-------------------------------------------------------------------------- 	
void ps2_vu0_view_screen_matrix(ps2_vu0_fmatrix m, float scrz, float ax, float ay, 
	       float cx, float cy, float zmin, float zmax, float nearz, float farz)
{
	float	az, cz;
	ps2_vu0_fmatrix	mt;

	cz = (-zmax * nearz + zmin * farz) / (-nearz + farz);
	az  = farz * nearz * (-zmin + zmax) / (-nearz + farz);

	//     | scrz    0  0 0 |
	// m = |    0 scrz  0 0 | 
	//     |    0    0  0 1 |
	//     |    0    0  1 0 |
	ps2_vu0_unit_matrix(m);
	m[0][0] = scrz;
	m[1][1] = scrz;
	m[2][2] = 0.0f;
	m[3][3] = 0.0f;
	m[3][2] = 1.0f;
	m[2][3] = 1.0f;

	//     | ax  0  0 cx |
	// m = |  0 ay  0 cy | 
	//     |  0  0 az cz |
	//     |  0  0  0  1 |
	ps2_vu0_unit_matrix(mt);
	mt[0][0] = ax;
	mt[1][1] = ay;
	mt[2][2] = az;
	mt[3][0] = cx;
	mt[3][1] = cy;
	mt[3][2] = cz;

	ps2_vu0_mul_matrix(m, mt, m);
	return;
}

//==========================================================================
// 
// ps2_vu0_drop_shadow_matrix          ドロップシャドウ射影行列の生成
// SYNOPSIS
//        void ps2_vu0_drop_shadow_matrix(ps2_vu0_fmatrix m, ps2_vu0_fvector lp, float a, float b, float c, int mode);
// ARGUMENTS
// 	なし
//	m               OUT:matrix
//	lp              IN:vector(光源の位置)
//	a               入力:影の投影面
//	b               入力:影の投影面
//	c               入力:影の投影面
//	mode            入力:光源の種類
//			0: 平行光源
//			1: 点光源
// DESCRIPTION
//	ax+by+cz=1で表現される平面にlpとmodeで指定される光源からの影を投影
//	する行列を求め、mに返します。
// RETURN_VALUE
// 	なし
//-------------------------------------------------------------------------- 	
// drop shadow 
//	p:		light source postion
//	a, b, c:	shadow surface (ax+by+cz = 1)
//
//	    | 1 0 0 px |   | d 0 0 0 |   | 1 0 0 -px |
//	M = | 0 1 0 py | * | 0 d 0 0 | * | 0 1 0 -py |
//	    | 0 0 1 pz |   | 0 0 d 0 |   | 0 0 1 -pz |
//	    | 0 0 0  1 |   | a b c 0 |   | 0 0 0   1 |
//
//	    | ax+d bx   cx   -x |			
//	  = | ay   by+d cy   -y |			
//	    | az   bz   cz+d -z |			
//	    | a    b    c    d-1| 
//		(d = 1-(ax+by+cz))			
//
// drop shadow (paralle light
//	d:	light direction
//	a,b,c:	shadow surface
//
//	(x,y,z) =  (pt, qt, rt), then t -> infinity
//
//	    | ap-n bp   cp   -p |
//	M = | aq   bp-n cp   -q |
//	    | ar   bp   cp-n -r |
//	    |  0   0   0    -n  | (n = ap+bq+cr)
//-------------------------------------------------------------------------- 	

void ps2_vu0_drop_shadow_matrix(ps2_vu0_fmatrix m, ps2_vu0_fvector lp, float a, float b, float c, int mode)
{
    if (mode) {	// spot light
	float x = lp[0], y = lp[1], z = lp[2];
	float d = (float)1-(a*x+b*y+c*z);

	m[0][0] = a*x+d, m[1][0] = b*x,   m[2][0] = c*x,   m[3][0] = -x;
	m[0][1] = a*y,   m[1][1] = b*y+d, m[2][1] = c*y,   m[3][1] = -y;
	m[0][2] = a*z,   m[1][2] = b*z,   m[2][2] = c*z+d, m[3][2] = -z;
	m[0][3] = a,     m[1][3] = b,     m[2][3] = c,     m[3][3] = d-(float)1;
    }
    else {		// parallel light
	float p  = lp[0], q = lp[1], r = lp[2];
	float n  = a*p+b*q+c*r;
	float nr = -(float)1.0/n;

	m[0][0] = nr*(a*p-n), m[1][0] = nr*(b*p),   m[2][0] = nr*(c*p),   m[3][0] = nr*(-p);
	m[0][1] = nr*(a*q),   m[1][1] = nr*(b*q-n), m[2][1] = nr*(c*q),   m[3][1] = nr*(-q);
	m[0][2] = nr*(a*r),   m[1][2] = nr*(b*r),   m[2][2] = nr*(c*r-n), m[3][2] = nr*(-r);
	m[0][3] = (float)0,          m[1][3] = (float)0,          m[2][3] =(float) 0,          m[3][3] = nr*(-n);
    }
}



//==========================================================================
//ps2_vu0_rot_trans_pers_n	透視変換
// SYNOPSIS
//	void ps2_vu0_rot_trans_pers_n(ps2_vu0_ivector *v0, ps2_vu0_fmatrix m0, ps2_vu0_fvector v1, int n, int mode);
// ARGUMENTS
// 	v0		OUT:vector(スクリーン座標)
// 	m0		IN:matrix
// 	v1		IN:vector
// 	n		入力:頂点数
// 	mode		入力:v0[2],v0[3]の値の切り替え
// 			     0:32bit符号なし固定小数点(28.4)
// 			     1:32bit符号なし固定小数点(32.0)
// DESCRIPTION
// 	与えられたn個の頂点をスクリーン座標に透視変換する。
//	出力v0の値は、v0[0],v0[1]は、32bit符号付き固定小数点(28.4)、
//	v0[2],v0[3]はmode=1のときは、32bit符号なし固定小数点(28.4)、
//	mode=0のときは、32bit符号なし固定小数点(32.0)を返す。
//	GIFのPACKEDモードでXYZF2,XYZF3を用いる場合には、mode=0にす
//	ると、PACK時に整数部が切り出されるので有用である。
//	
// RETURN_VALUE
// 	なし
// 
//-------------------------------------------------------------------------- 	

void ps2_vu0_rot_trans_pers_n(ps2_vu0_ivector *v0, ps2_vu0_fmatrix m0, ps2_vu0_fvector *v1, int n, int mode)
{
	asm __volatile__("
	lqc2	vf4,0x0(%1)
	lqc2	vf5,0x10(%1)
	lqc2	vf6,0x20(%1)
	lqc2	vf7,0x30(%1)
1:
	lqc2	vf8,0x0(%2)
	vmulax.xyzw     ACC, vf4,vf8
	vmadday.xyzw    ACC, vf5,vf8
	vmaddaz.xyzw    ACC, vf6,vf8
	vmaddw.xyzw      vf9,vf7,vf8
	vdiv    Q,vf0w,vf9w
	vwaitq
	vmulq.xyz	vf9,vf9,Q
	vftoi4.xyzw	vf10,vf9
	beqz	%4,2f
	vftoi0.zw	vf10,vf9	
2:
	sqc2	vf10,0x0(%0)

	addi	%3,-1
	addi	%2,0x10
	addi	%0,0x10
	bne	$0,%3,1b
	": : "r" (v0) , "r" (m0) ,"r" (v1), "r" (n) ,"r" (mode));
}


//==========================================================================
//ps2_vu0_rot_trans_pers	透視変換
// SYNOPSIS
//	void ps2_vu0_rot_trans_pers(ps2_vu0_ivector v0, ps2_vu0_fmatrix m0, ps2_vu0_fvector v1);
// ARGUMENTS
// 	v0		OUT:vector(スクリーン座標)
// 	m0		IN:matrix
// 	v1		IN:vector
// 	mode		入力:v0[2],v0[3]の値の切り替え
// 			     0:32bit符号なし固定小数点(28.4)
// 			     1:32bit符号なし固定小数点(32.0)
// DESCRIPTION
// 	与えられた頂点をスクリーン座標に透視変換する。
//	出力v0の値は、v0[0],v0[1]は、32bit符号付き固定小数点(28.4)、
//	v0[2],v0[3]はmode=1のときは、32bit符号なし固定小数点(28.4)、
//	mode=0のときは、32bit符号なし固定小数点(32.0)を返す。
//	GIFのPACKEDモードでXYZF2,XYZF3を用いる場合には、mode=0にす
//	ると、PACK時に整数部が切り出されるので有用である。
//	
// RETURN_VALUE
// 	なし
// 
//-------------------------------------------------------------------------- 	

void ps2_vu0_rot_trans_pers(ps2_vu0_ivector v0, ps2_vu0_fmatrix m0, ps2_vu0_fvector v1, int mode)
{
	asm __volatile__("
	lqc2	vf4,0x0(%1)
	lqc2	vf5,0x10(%1)
	lqc2	vf6,0x20(%1)
	lqc2	vf7,0x30(%1)
	lqc2	vf8,0x0(%2)
	vmulax.xyzw     ACC, vf4,vf8
	vmadday.xyzw    ACC, vf5,vf8
	vmaddaz.xyzw    ACC, vf6,vf8
	vmaddw.xyzw      vf9,vf7,vf8
	vdiv    Q,vf0w,vf9w
	vwaitq
	vmulq.xyz	vf9,vf9,Q
	vftoi4.xyzw	vf10,vf9
	beqz	%3,1f
	vftoi0.zw	vf10,vf9	
1:
	sqc2	vf10,0x0(%0)

	": : "r" (v0) , "r" (m0) ,"r" (v1), "r" (mode));
}

//==========================================================================
// 
// ps2_vu0_copy_vector_xyz	ベクトルの複写
// SYNOPSIS
//	void ps2_vu0_copy_vector_xyz(ps2_vu0_fvector v0,ps2_vu0_fvector v1);
// ARGUMENTS
//	v0		OUT:vector
//	v1		IN:vector
// DESCRIPTION
//	ベクトルv1のx,y,z要素をベクトルv0にコピーします。
//	ベクトルv0のw要素は、そのままベクトルv0に返す。 
// RETURN_VALUE
// 	なし
// 
//-------------------------------------------------------------------------- 	
void  ps2_vu0_copy_vector_xyz(ps2_vu0_fvector v0, ps2_vu0_fvector v1)
{
	v0[0]=v1[0];
	v0[1]=v1[1];
	v0[2]=v1[2];
}


//==========================================================================
// 
// ps2_vu0_inter_vector_xyz	内挿ベクトルの生成
// SYNOPSIS
//	void ps2_vu0_inter_vector_xyz(ps2_vu0_fvector v0,ps2_vu0_fvector v1,ps2_vu0_fvector v2, float t)
// ARGUMENTS
//	v0		OUT:vector
//	v1.v2		IN:vector
//	t		入力:内挿パラメータ
// DESCRIPTION
//	ベクトルv1,v2およびパラメータtから内挿ベクトルを求め、v0に返します。
//	式で表現すると次のとおりです。
//	 v0 = v1*t + v2*(1-t)
//	ただしベクトルv0のw要素は、そのままベクトルv0に返します。
// RETURN_VALUE
// 	なし
// 
//-------------------------------------------------------------------------- 	
void ps2_vu0_inter_vector_xyz(ps2_vu0_fvector v0, ps2_vu0_fvector v1, ps2_vu0_fvector v2, float r)
{
	asm __volatile__("
	lqc2    vf4,0x0(%1)
	lqc2    vf5,0x0(%2)
	mfc1    $8,%3
	qmtc2    $8,vf6

	vmove.w	vf9,vf4
	vaddw.x    vf7,vf0,vf0	#vf7.x=1;
	vsub.x    vf8,vf7,vf6	#vf8.x=1-t;
	vmulax.xyz	ACC,   vf4,vf6
	vmaddx.xyz	vf9,vf5,vf8

	sqc2    vf9,0x0(%0)
	": : "r" (v0) , "r" (v1), "r" (v2), "f" (r) : "$8");
}

//==========================================================================
// 
// ps2_vu0_scale_vector_xyz	スカラー値とベクトルの乗算 (MULx/xyz)
// SYNOPSIS
//	void ps2_vu0_scale_vector(ps2_vu0_fvector v0,ps2_vu0_fvector v1,float t)
// ARGUMENTS
// 	なし
//	v0		OUT:vector
//	v1		IN:vector
//	t		IN:scalar
// DESCRIPTION
//	ベクトルv1のx,y,z要素にスカラーtを乗算し、結果をベクトルv0に返しま
//	す。ベクトルv0のw要素は、そのままベクトルv0に返します。
// RETURN_VALUE
// 	なし
// 
//-------------------------------------------------------------------------- 	
void  ps2_vu0_scale_vector_xyz(ps2_vu0_fvector v0, ps2_vu0_fvector v1, float s)
{
	asm __volatile__("
	lqc2    vf4,0x0(%1)
	mfc1    $8,%2
	qmtc2    $8,vf5
	vmulx.xyz	vf4,vf4,vf5
	sqc2    vf4,0x0(%0)
	": : "r" (v0) , "r" (v1), "f" (s):"$8");
}

//==========================================================================
// 
// ps2_vu0_clip_screen	頂点のＧＳ描画範囲のクリッピング
// SYNOPSIS
// 	int ps2_vu0_clip_screen(ps2_vu0_fvector v0)
// ARGUMENTS
// 	v0		IN:vector
// DESCRIPTION
// 	頂点ベクトルv0がＧＳ描画範囲に入っているかを調べる。
// RETURN_VALUE
// 	0:頂点v0がＧＳ描画範囲に入っている。
// 
//-------------------------------------------------------------------------- 	

int ps2_vu0_clip_screen(ps2_vu0_fvector v0)
{
	register int ret;

	asm __volatile__("
	vsub.xyzw        vf04,vf00,vf00  #(0,0,0,0);
	.set push
	.set mips3
	li     %0,0x4580000045800000
	.set pop
	lqc2    vf07,0x0(%1)

	qmtc2  %0,vf06
	ctc2    $0,$vi16                #ステータスフラグのクリア

	vsub.xyw        vf05,vf07,vf04  #(z,ZBz,PPy,PPx)-(0,0,0,0);
	vsub.xy        vf05,vf06,vf07  #(Zmax,0,Ymax,Xmax) -(z,ZBz,PPy,PPx)

	vnop				
	vnop				
	vnop				
	vnop				
	vnop				
        cfc2    %0,$vi16                #ステータスフラグをREAD
	andi	%0,%0,0xc0

	":"=r"(ret): "r" (v0));
	return ret;
}

//==========================================================================
// 
// ps2_vu0_clip_screen3	３頂点のＧＳ描画範囲のクリッピング
// SYNOPSIS
// 	int ps2_vu0_clip_screen3(ps2_vu0_fvector v0, ps2_vu0_fvector v1, ps2_vu0_fvector v2)
// ARGUMENTS
// 	v0,v1,v2		IN:vector
// DESCRIPTION
// 	頂点ベクトルv0,v1,v2がすべてＧＳ描画範囲に入っているかを調べる。
// RETURN_VALUE
// 	0:すべての頂点がＧＳ描画範囲に入っている。
// 
//-------------------------------------------------------------------------- 	
 
int ps2_vu0_clip_screen3(ps2_vu0_fvector v0, ps2_vu0_fvector v1, ps2_vu0_fvector v2)
{
	register int ret;

	asm __volatile__("
	vsub.xyzw        vf04,vf00,vf00  #(0,0,0,0);
	.set push
	.set mips3
	li     %0,0x4580000045800000
	.set pop
	lqc2    vf06,0x0(%1)
	lqc2    vf08,0x0(%2)
	lqc2    vf09,0x0(%3)

	qmtc2  %0,vf07
	ctc2    $0,$vi16                #ステータスフラグのクリア

	vsub.xyw        vf05,vf06,vf04  #(z,ZBz,PPy,PPx)-(0,0,0,0);
	vsub.xy        vf05,vf07,vf06  #(Zmax,0,Ymax,Xmax) -(z,ZBz,PPy,PPx)
	vsub.xyw        vf05,vf08,vf04  #(z,ZBz,PPy,PPx)-(0,0,0,0);
	vsub.xy        vf05,vf07,vf08  #(Zmax,0,Ymax,Xmax) -(z,ZBz,PPy,PPx)
	vsub.xyw        vf05,vf09,vf04  #(z,ZBz,PPy,PPx)-(0,0,0,0);
	vsub.xy        vf05,vf07,vf09  #(Zmax,0,Ymax,Xmax) -(z,ZBz,PPy,PPx)

	vnop				
	vnop				
	vnop				
	vnop				
	vnop				
	cfc2    %0,$vi16                #ステータスフラグをREAD
	andi	%0,%0,0xc0

	":"=r"(ret): "r" (v0), "r" (v1), "r" (v2) );
	return ret;
}

#define UNDER_X		 1
#define UNDER_Y		 2
#define UNDER_W		 4
#define OVER_X		 8
#define OVER_Y		16
#define OVER_W		32
//==========================================================================
//
// ps2_vu0_clip_all           表示範囲によるクリッピング検査
// SYNOPSIS
// 	int ps2_vu0_clip_all(ps2_vu0_fvector minv, ps2_vu0_fvector maxv, ps2_vu0_fvector ms,ps2_vu0_fvector *vm,int n)
// ARGUMENTS
//	minv			入力:表示範囲の最小値
//	maxv			入力:表示範囲の最大値
//	ms			IN:matrix（モデル−スクリーン）
//	vm			入力:頂点ベクトルのポインタ
//	n			入力:頂点数
// DESCRIPTION
//	vmとnで指定されるn個の頂点がすべて表示範囲に入っていないかどうかを
//	調べます。
// RETURN_VALUE
//	すべての頂点が表示範囲に入っていないときは1を返します。
//
//-------------------------------------------------------------------------- 	
int ps2_vu0_clip_all(ps2_vu0_fvector minv, ps2_vu0_fvector maxv, ps2_vu0_fmatrix ms, ps2_vu0_fvector *vm, int n)
{
	register int ret;

	asm __volatile__("

	lqc2    vf8,0x0(%4)
	lqc2    vf4,0x0(%3)
	lqc2    vf5,0x10(%3)
	lqc2    vf6,0x20(%3)
	lqc2    vf7,0x30(%3)

	lqc2    vf9,0x0(%1)	#minv
	lqc2    vf10,0x0(%2)	#maxv
	lqc2    vf11,0x0(%1)	#minv
	lqc2    vf12,0x0(%2)	#maxv

1:
	vmulax.xyzw	ACC,vf4,vf8
	vmadday.xyzw	ACC,vf5,vf8
	vmaddaz.xyzw	ACC,vf6,vf8
	vmaddw.xyzw	vf8,vf7,vf8

	vmulw.xyz	vf11,vf9,vf8
	vmulw.xyz	vf12,vf10,vf8

	vnop				
	vnop				
	ctc2    $0,$vi16                #ステータスフラグのクリア
	vsub.xyw       vf11,vf8,vf11  #(z,ZBz,PPy,PPx)-(Zmin,0,Ymin,Xmin);
	vsub.xyw       vf12,vf12,vf8  #(Zmax,0,Ymax,Xmax) -(z,ZBz,PPy,PPx)
	vmove.w	vf11,vf9
	vmove.w	vf12,vf10
	vnop				
	addi	%4,0x10
	lqc2    vf8,0x0(%4)
	addi	%5,-1
	cfc2    %0,$vi16                #ステータスフラグをREAD
	andi	%0,$2,0xc0
	beqz	%0,2f

	bne	$0,%5,1b

	addi	%0,$0,1
2:

	":"=r"(ret): "r" (minv),"r" (maxv),"r" (ms),"r" (vm),"r" (n)  );
	return ret;
}
