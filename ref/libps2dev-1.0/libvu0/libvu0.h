
#ifndef _LIB_VU0_H_
#define _LIB_VU0_H_

#ifdef __cplusplus
extern "C" {
#endif

// basic type
typedef int		ps2_vu0_ivector[4] __attribute__((aligned (16)));
typedef float	ps2_vu0_fvector[4] __attribute__((aligned (16)));
typedef float	ps2_vu0_fmatrix[4][4] __attribute__((aligned (16)));

// prototypes
void  ps2_vu0_copy_vector(ps2_vu0_fvector v0, ps2_vu0_fvector v1);
void  ps2_vu0_copy_vector_xyz(ps2_vu0_fvector v0, ps2_vu0_fvector v1);
void  ps2_vu0_ftoi0_vector(ps2_vu0_ivector v0, ps2_vu0_fvector v1);
void  ps2_vu0_ftoi4_vector(ps2_vu0_ivector v0, ps2_vu0_fvector v1);
void  ps2_vu0_itof0_vector(ps2_vu0_fvector v0, ps2_vu0_ivector v1);
void  ps2_vu0_itof4_vector(ps2_vu0_fvector v0, ps2_vu0_ivector v1);
void  ps2_vu0_scale_vector(ps2_vu0_fvector v0, ps2_vu0_fvector v1, float s);
void  ps2_vu0_Scale_vectorXYZ(ps2_vu0_fvector v0, ps2_vu0_fvector v1, float s);
void  ps2_vu0_add_vector(ps2_vu0_fvector v0, ps2_vu0_fvector v1, ps2_vu0_fvector v2);
void  ps2_vu0_sub_vector(ps2_vu0_fvector v0, ps2_vu0_fvector v1, ps2_vu0_fvector v2);
void  ps2_vu0_mul_vector(ps2_vu0_fvector v0, ps2_vu0_fvector v1, ps2_vu0_fvector v2);
void  ps2_vu0_inter_Vector(ps2_vu0_fvector v0, ps2_vu0_fvector v1, ps2_vu0_fvector v2, float r);
void  ps2_vu0_inter_VectorXYZ(ps2_vu0_fvector v0, ps2_vu0_fvector v1, ps2_vu0_fvector v2, float r);
void  ps2_vu0_div_vector(ps2_vu0_fvector v0, ps2_vu0_fvector v1, float q);
void  ps2_vu0_div_vector_xyz(ps2_vu0_fvector v0, ps2_vu0_fvector v1, float q);
float ps2_vu0_inner_product(ps2_vu0_fvector v0, ps2_vu0_fvector v1);
void  ps2_vu0_outer_product(ps2_vu0_fvector v0, ps2_vu0_fvector v1, ps2_vu0_fvector v2);
void  ps2_vu0_normalize(ps2_vu0_fvector v0, ps2_vu0_fvector v1);
void  ps2_vu0_apply_matrix(ps2_vu0_fvector v0, ps2_vu0_fmatrix m, ps2_vu0_fvector v1);
void  ps2_vu0_unit_matrix(ps2_vu0_fmatrix m);
void  ps2_vu0_copy_matrix(ps2_vu0_fmatrix m0, ps2_vu0_fmatrix m1);
void  ps2_vu0_transpose_matrix(ps2_vu0_fmatrix m0, ps2_vu0_fmatrix m1);
void  ps2_vu0_mul_matrix(ps2_vu0_fmatrix m0, ps2_vu0_fmatrix m1, ps2_vu0_fmatrix m2);
void  ps2_vu0_invers_matrix(ps2_vu0_fmatrix m0, ps2_vu0_fmatrix m1);
void  ps2_vu0_rot_matrix_x(ps2_vu0_fmatrix m0, ps2_vu0_fmatrix m1, float rx);
void  ps2_vu0_rot_matrix_y(ps2_vu0_fmatrix m0, ps2_vu0_fmatrix m1, float ry);
void  ps2_vu0_rot_matrix_z(ps2_vu0_fmatrix m0, ps2_vu0_fmatrix m1, float rz);
void  ps2_vu0_rot_matrix(ps2_vu0_fmatrix m0, ps2_vu0_fmatrix m1, ps2_vu0_fvector rot);
void  ps2_vu0_trans_matrix(ps2_vu0_fmatrix m0, ps2_vu0_fmatrix m1, ps2_vu0_fvector tv);
void  ps2_vu0_camera_matrix(ps2_vu0_fmatrix m, ps2_vu0_fvector p, ps2_vu0_fvector zd, ps2_vu0_fvector yd);
void  ps2_vu0_normal_light_matrix(ps2_vu0_fmatrix m, ps2_vu0_fvector l0, ps2_vu0_fvector l1, ps2_vu0_fvector l2);
void  ps2_vu0_light_color_matrix(ps2_vu0_fmatrix m, ps2_vu0_fvector c0, 
						ps2_vu0_fvector c1, ps2_vu0_fvector c2, ps2_vu0_fvector a);
void  ps2_vu0_view_screen_matrix(ps2_vu0_fmatrix m, float scrz, float ax, float ay, 
	float cx, float cy, float zmin, float zmax, float nearz, float farz);
void  ps2_vu0_drop_shadow_matrix(ps2_vu0_fmatrix m, 
				ps2_vu0_fvector lp, float a, float b, float c, int mode);
int   ps2_vu0_clip_all(ps2_vu0_fvector minv, ps2_vu0_fvector maxv, ps2_vu0_fmatrix ms, ps2_vu0_fvector *vm, int n);
void ps2_vu0_clamp_vector(ps2_vu0_fvector v0, ps2_vu0_fvector v1, float min, float max);
int ps2_vu0_clip_screen(ps2_vu0_fvector v0);
int ps2_vu0_clip_screen3(ps2_vu0_fvector v0, ps2_vu0_fvector v1, ps2_vu0_fvector v2);
void ps2_vu0_rot_trans_pers_n(ps2_vu0_ivector *v0, ps2_vu0_fmatrix m0, ps2_vu0_fvector *v1, int n, int mode);
void ps2_vu0_rot_trans_pers(ps2_vu0_ivector v0, ps2_vu0_fmatrix m0, ps2_vu0_fvector v1, int mode);

#ifdef __cplusplus
}
#endif

#endif /*  _LIB_VU0_H_ */
