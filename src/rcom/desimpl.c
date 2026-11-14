/*
 * DES_EDE: triple DES of one block; the three key schedules sit at ctx,
 * ctx + 128 and ctx + 256.
 */

#include "descore.h"
#include "desimpl.h"

void DES_EDE(void *ctx, void *in, void *out)
{
	DES(ctx, in, out);
	DES((char *)ctx + 128, in, in);
	DES((char *)ctx + 256, in, in);
}
