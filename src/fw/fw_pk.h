/*
 * fwPk*, fwHig*: the DMA chain of GS packets of the fw framework (fw_pk.c).
 */

#ifndef FW_PK_H
#define FW_PK_H

#ifdef __cplusplus
extern "C" {
#endif

int fwPkSetBufMode(int mode);
int fwPkCheckBufMode(void);
int fwHigIsAfterKick(void);
int fwPkCheckReady(void);
int fwPkWaitDma(void);
int fwPkKickChainBuf(void *handle, int flag);
int fwHigAddDmaPacketUsingCall(void *pkt, void *addr, int qwc, void (*call)(void));
int fwHigAddDmaPacketUsingRef(void *pkt, void *addr, int qwc);
int fwPkAddDmaChain_withCont(void *handle, int size);
int fwPkAddDmaChain_withCont_setVif(void *data, int nqw, unsigned int vif1, unsigned int vif2);
int fwPkAddDmaChain_withCall(void *addr);
int fwPkAddDmaChain_withCall_physAddr(unsigned int phys);
int fwPkAddDmaChain_withRef(void *handle, int size);
int fwPkAddDmaChain_withRef_setVif(void *data, unsigned int len, int ref, unsigned int vif);
int fwPkInitChainBuf(void *base, int size, int mode);
void fwPkReleaseChainBuf(void);
int fwPkFlipChainBuf(void);

#ifdef __cplusplus
}
#endif

#endif
