/*
 * IPU driver and TIM2/BTM2 texture loader (bt2ipu.cpp).
 */

#ifndef BT2IPU_H
#define BT2IPU_H

void bt2IpuResetParam();
void bt2IpuResetFifo();
void bt2IpuInit();
void bt2IpuTerm();
int _ipu_store_block(void *dst, int stride);
int Tim2_GetTexID(const char *path, bool alpha);
int Tim2_GetTexID(void *data, unsigned int size, bool alpha);
void Tim2_ReleaseTexID(int texid);

#endif
