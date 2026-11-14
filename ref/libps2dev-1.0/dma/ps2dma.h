#ifndef _PS2DMA_H_
#define _PS2DMA_H_

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include <ps2vpufile.h>
//----------------------------------------------------------------------
// prototypes
//----------------------------------------------------------------------
// for non-blocking
int ps2_dma_start(int fd, VPUFILE *vfd, ps2_dmatag *tag);
int ps2_dma_start_n(int fd, void *ptr, int qwc);
int ps2_dma_wait(int fd, int n);
// for blocking
int ps2_dma_send(int fd, VPUFILE *vfd, ps2_dmatag *tag);
int ps2_dma_send_n(int fd, void *ptr, int qwc);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif /* _PS2DMA_H_ */
