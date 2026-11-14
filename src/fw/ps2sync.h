/*
 * PS2_DMA_SYNC(): the R5900 `sync` that ends the building of a GIF packet,
 * so that its stores are visible to the DMA controller before it is kicked.
 */

#ifndef PS2SYNC_H
#define PS2SYNC_H

#ifdef __R5900
#define PS2_DMA_SYNC()	__asm__ __volatile__ ("sync")
#else
#define PS2_DMA_SYNC()	do { } while (0)
#endif

#endif /* PS2SYNC_H */
