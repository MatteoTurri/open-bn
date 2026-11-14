/*
 * Builds the GIFtag of an IMAGE-mode GIF packet.
 */

struct GifPacketHeader {
	/* 0x00 */ unsigned int tag_lo;      /* NLOOP in bits 0-14, EOP in bit 15 */
	/* 0x04 */ unsigned int tag_hi;      /* PRE/PRIM/FLG/NREG (bits 32-63) */
	/* 0x08 */ unsigned long long regs;  /* REGS list */
}; /* size = 0x10 */

void gifMakeGifPacketHeader_IMAGE(struct GifPacketHeader *pkt, unsigned int nloop)
{
	pkt->tag_hi = 0x08000000;  /* FLG = 2 (IMAGE) */
	pkt->tag_lo = nloop | 0x8000;  /* EOP */
	pkt->regs = 0;
}
