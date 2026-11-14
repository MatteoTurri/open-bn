/*
 * Userland driver of the PS2 IPU (/dev/ps2ipu): reset and parameters, DMA
 * stop, hardware decoding of BTM2 (MPEG-2 intra) pictures, and the TIM2/BTM2
 * texture loader.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/ps2/dev.h>
#include "tm.h"
#include "texture.h"
#include "bt2ipu.h"
#include "tim2.h"
#include "fs/bburl.h"
#include "fs/bbfile.h"

/* IPU_CTRL: bit 31 = BUSY, bit 14 = IDP (command error / waiting for data),
   bits 4..7 = OFC, the number of qwords in the output FIFO. */
#define IPU_CTRL_BUSY  0x80000000
#define IPU_CTRL_IDP   0x00004000
#define IPU_CTRL_OFC   0x000000F0
#define IPU_CTRL_RST   0x40000000

#define IPU_CMD   0
/* IPU commands (bits 28..31): 0x4 FDEC (read bits; the low 6 bits skip that
   many first), 0x5 SETIQ (bit 27: 0 intra, 1 non-intra matrix), 0x6 SETVQ,
   0x9 SETTH, 0x1 IDEC (intra decode). */
#define IPU_CMD_OFS  0x00
#define IPU_CTRL_OFS 0x10
#define IPU_REG(ofs) (*(int *)((char *)bt2ipu_regs + (ofs)))
#define IPU_CTRL  4

/* DMA environment towards the IPU, as used by bt2IpuStopDMA (0x20 and 0x24
   are not used there). */
struct bt2IpuDmaEnv {
	/* 0x00 */ void *curPtr;
	/* 0x04 */ int curQwc;
	/* 0x08 */ void *nextPtr;
	/* 0x0C */ int nextQwc;
	/* 0x10 */ void *stopPtr;
	/* 0x14 */ int stopQwc;
	/* 0x18 */ void *prevPtr;
	/* 0x1C */ int prevQwc;
	/* 0x20 */ int opaque20;
	/* 0x24 */ int opaque24;
	/* 0x28 */ struct ps2_pstop stop;
	/* 0x30 */ unsigned int bp;
	/* 0x34 */ unsigned int ctrl;
}; /* size = 0x38 */

/* Pixel type to GS format (see TIM2_FORMAT): indexed by `imageType - 1`
   for the image and `(clutType & 0x3F) - 1` for the palette. */
static TIM2_FORMAT s_tim2Format[] = {
	{ 2, 2, 1 },        /* 1: 16 bit */
	{ 1, 3, 1 },        /* 2: 24 bit */
	{ 0, 4, 1 },        /* 3: 32 bit */
	{ 0x14, 1, 2 },     /* 4: 4 bit */
	{ 0x13, 1, 1 },     /* 5: 8 bit */
	{ -1, 0, 0 },
};

int bt2ipu_fd = -1;

static bt2IpuDmaEnv bt2ipu_dmaenv;

volatile int *bt2ipu_regs;

/* One qword of the IPU FIFO. */
struct bt2IpuFifoPacket {
	unsigned int w[4];
} __attribute__((aligned(16)));

/* The seven 16-byte packets sent to the IPU input FIFO by bt2IpuResetParam:
   the default intra quantizer matrix of MPEG (the same in MPEG-1 and MPEG-2)
   in zigzag order (packets 0-3), the flat non-intra matrix (packet 4, sent
   four times) and the VQCLUT (16 RGB555 colours, the second table). */
static bt2IpuFifoPacket bt2ipu_reset_fifo[5] = {
	{ { 0x13101008, 0x16161310, 0x16161616, 0x1b1a181a } },
	{ { 0x1a1a1b1b, 0x1b1b1a1a, 0x1d1d1d1b, 0x1d222222 } },
	{ { 0x1b1b1d1d, 0x20201d1d, 0x26252222, 0x22232325 } },
	{ { 0x28262623, 0x30302828, 0x38382e2e, 0x5345453a } },
	{ { 0x10101010, 0x10101010, 0x10101010, 0x10101010 } },
};
static bt2IpuFifoPacket bt2ipu_reset_fifo2[2] = {
	{ { 0x04210000, 0x03e00842, 0x14a51084, 0x1ce718c6 } },
	{ { 0x2529001f, 0x7c00294a, 0x35ad318c, 0x39ce7fff } },
};

/* Reloads the IPU defaults: the default intra matrix (packets 0..3) and the
   flat non-intra matrix (packet 4, four times) with the two SETIQ commands,
   the VQ colour table of packets 5..6 with SETVQ, then SETTH 0. */
void bt2IpuResetParam()
{
	bt2IpuFifoPacket fifo;
	unsigned int cmd;

	fifo = bt2ipu_reset_fifo[0];
	ioctl(bt2ipu_fd, PS2IOC_SIPUFIFO, &fifo);
	fifo = bt2ipu_reset_fifo[1];
	ioctl(bt2ipu_fd, PS2IOC_SIPUFIFO, &fifo);
	fifo = bt2ipu_reset_fifo[2];
	ioctl(bt2ipu_fd, PS2IOC_SIPUFIFO, &fifo);
	fifo = bt2ipu_reset_fifo[3];
	ioctl(bt2ipu_fd, PS2IOC_SIPUFIFO, &fifo);
	fifo = bt2ipu_reset_fifo[4];
	ioctl(bt2ipu_fd, PS2IOC_SIPUFIFO, &fifo);
	ioctl(bt2ipu_fd, PS2IOC_SIPUFIFO, &fifo);
	ioctl(bt2ipu_fd, PS2IOC_SIPUFIFO, &fifo);
	ioctl(bt2ipu_fd, PS2IOC_SIPUFIFO, &fifo);

	cmd = 0x50000000;
	ioctl(bt2ipu_fd, PS2IOC_SIPUCMD, &cmd);
	while (bt2ipu_regs[IPU_CTRL] < 0)
		;
	cmd = 0x58000000;
	ioctl(bt2ipu_fd, PS2IOC_SIPUCMD, &cmd);
	while (bt2ipu_regs[IPU_CTRL] < 0)
		;

	fifo = bt2ipu_reset_fifo2[0];
	ioctl(bt2ipu_fd, PS2IOC_SIPUFIFO, &fifo);
	fifo = bt2ipu_reset_fifo2[1];
	ioctl(bt2ipu_fd, PS2IOC_SIPUFIFO, &fifo);

	cmd = 0x60000000;
	ioctl(bt2ipu_fd, PS2IOC_SIPUCMD, &cmd);
	while (bt2ipu_regs[IPU_CTRL] < 0)
		;
	cmd = 0x90000000;
	ioctl(bt2ipu_fd, PS2IOC_SIPUCMD, &cmd);
	while (bt2ipu_regs[IPU_CTRL] < 0)
		;
}

void bt2IpuResetFifo()
{
	unsigned int ctrl = IPU_CTRL_RST;

	ioctl(bt2ipu_fd, PS2IOC_SIPUCTRL, &ctrl);
	while (bt2ipu_regs[IPU_CTRL] < 0)
		;
	IPU_REG(IPU_CMD_OFS) = 0;
	while (bt2ipu_regs[IPU_CTRL] < 0)
		;
}

void bt2IpuInit()
{
	if (bt2ipu_fd < 0) {
		bt2ipu_fd = open(PS2_DEV_IPU, O_RDWR);
		if (bt2ipu_fd < 0) {
			perror(PS2_DEV_IPU);
			bt2ipu_fd = -1;
			return;
		}
		bt2ipu_regs = (volatile int *)mmap(0, 8192, PROT_READ | PROT_WRITE, MAP_SHARED, bt2ipu_fd, 0);
		if (bt2ipu_regs == (volatile int *)-1) {
			perror(PS2_DEV_IPU);
			close(bt2ipu_fd);
			bt2ipu_fd = -1;
			return;
		}
	}
	fcntl(bt2ipu_fd, F_SETFD, FD_CLOEXEC);
	bt2IpuResetFifo();
	bt2IpuResetParam();
	bt2IpuResetFifo();
}

void bt2IpuTerm()
{
	munmap((void *)bt2ipu_regs, 8192);
	bt2ipu_regs = 0;
	close(bt2ipu_fd);
	bt2ipu_fd = -1;
}

void bt2IpuStopDMA(bt2IpuDmaEnv *env)
{
	ioctl(bt2ipu_fd, PS2IOC_SENDSTOP, &env->stop);
	while (bt2ipu_regs[IPU_CTRL] & IPU_CTRL_OFC) {
		struct ps2_fifo fifo;

		ioctl(bt2ipu_fd, PS2IOC_GIPUFIFO, &fifo);
	}
	ioctl(bt2ipu_fd, PS2IOC_GIPUBP, &env->bp);
	ioctl(bt2ipu_fd, PS2IOC_GIPUCTRL, &env->ctrl);
	env->stopPtr = env->nextPtr;
	env->stopQwc = env->nextQwc;
	env->prevPtr = env->curPtr;
	env->prevQwc = env->curQwc;
}

/* Drains the IPU output FIFO into a 16x16 RGBA32 macroblock (64 qwords, 4 per
   row): reads OFC from IPU_CTRL and copies the available qwords from the FIFO
   window (offset 4096), advancing by `stride` bytes every 4 qwords, with a
   timeout of 5000 rounds on an empty FIFO and an early exit on IDP. Returns -1
   if IDP is set, else the BUSY bit. The FIFO can only be read 128 bits at a
   time (lq/sq), hence the asm block. */
int _ipu_store_block(void *dst, int stride)
{
	volatile int *regs = bt2ipu_regs;

	/* clang-format off */
	__asm__ __volatile__ (
		".set push\n"
		".set noreorder\n"
		".set mips3\n"
		"	move	$16, %0\n"
		"	move	$17, %1\n"
		"	move	$19, %2\n"
		"	addiu	$19, $19, -64\n"
		"	li	$18, 64\n"
		"	li	$4, 5000\n"
		"	lw	$2, 16($16)\n"
		"1:	andi	$3, $2, 0xf0\n"
		"	beqz	$3, 3f\n"
		"	srl	$5, $3, 4\n"
		"2:	lq	$2, 4096($16)\n"
		"	addiu	$18, $18, -1\n"
		"	addiu	$5, $5, -1\n"
		"	beqz	$18, 4f\n"
		"	sq	$2, 0($17)\n"
		"	andi	$3, $18, 3\n"
		"	bnez	$3, 5f\n"
		"	addiu	$17, $17, 16\n"
		"	addu	$17, $17, $19\n"
		"5:	bnez	$5, 2b\n"
		"	nop\n"
		"	beqz	$5, 1b\n"
		"	lw	$2, 16($16)\n"
		"3:	andi	$3, $2, 0x4000\n"
		"	bnez	$3, 4f\n"
		"	nop\n"
		"	addiu	$4, $4, -1\n"
		"	bgtz	$4, 1b\n"
		"	lw	$2, 16($16)\n"
		"4:\n"
		".set pop\n"
		:
		: "r" (regs), "r" (dst), "r" (stride)
		: "$2", "$3", "$4", "$5", "$16", "$17", "$18", "$19", "memory");
	/* clang-format on */

	if (regs[IPU_CTRL] & IPU_CTRL_IDP)
		return -1;
	if (regs[IPU_CTRL] < 0)
		return 1;
	return 0;
}

/* Decodes an MPEG-2 intra-frame bitstream with the IPU: sends the packet to
   the input FIFO with PS2IOC_SENDA, drives the IPU command machine by hand
   (write to IPU_CMD, wait on BUSY) to read the sequence header, then pulls
   the decoded macroblocks out of the output FIFO with _ipu_store_block.
   Header bit 0x4000 selects the scan order: columns (dst = buf + row * 4,
   step (width >> 4) << 10) or rows (dst = buf + row * ((width >> 4) << 6),
   step 64). Returns -2 (IPU not initialized), the negative ioctl result,
   -1 (decoding interrupted or incomplete) or 0. */
int btm2_decode(TIM2_PIC_HEAD *head, void *buf)
{
	struct ps2_packet pkt;
	struct ps2_plist plist;  /* unused, but part of the layout */
	int ret;
	int width;
	int height;
	int row;
	int col = 0;
	int hdr;

	width = head->width;
	height = head->height;
	if (bt2ipu_fd < 0)
		return -2;
	if (bt2ipu_regs == 0)
		return -2;
	bt2IpuResetFifo();
	pkt.ptr = (char *)head + head->headerSize;
	pkt.len = head->imageSize;
	ret = ioctl(bt2ipu_fd, PS2IOC_SENDA, &pkt);
	if (ret < 0)
		goto stop;

	IPU_REG(IPU_CMD_OFS) = 0x40000000; /* FDEC: header flags in the top byte */
	while (bt2ipu_regs[IPU_CTRL] < 0)
		;
	hdr = (int)((unsigned int)bt2ipu_regs[IPU_CMD] >> 24);
	while (bt2ipu_regs[IPU_CTRL] < 0)
		;
	IPU_REG(IPU_CMD_OFS) = 0x40000008;
	while (bt2ipu_regs[IPU_CTRL] < 0)
		;
	if (hdr & 0x8) {
		hdr |= ((unsigned int)bt2ipu_regs[IPU_CMD] >> 16) & 0xff00;
		while (bt2ipu_regs[IPU_CTRL] < 0)
			;
		IPU_REG(IPU_CMD_OFS) = 0x40000008; /* skip 8 bits */
		while (bt2ipu_regs[IPU_CTRL] < 0)
			;
		if (hdr & 0x100) {
			int q = ((unsigned int)bt2ipu_regs[IPU_CMD] >> 16) & 0xff;

			while (bt2ipu_regs[IPU_CTRL] < 0)
				;
			IPU_REG(IPU_CMD_OFS) = 0x40000010;
			while (bt2ipu_regs[IPU_CTRL] < 0)
				;
			/* SETTH: TH1 in bits 16..24, TH0 in bits 0..8 (always 0, q is a byte). */
			IPU_REG(IPU_CMD_OFS) =
			    0x90000000 | ((q & 0xff) << 16) | ((q >> 8) & 0x1ff);  /* redundant as written */
			while (bt2ipu_regs[IPU_CTRL] < 0)
				;
		}
		if (hdr & 0x200) { /* intra matrix in the stream */
			IPU_REG(IPU_CMD_OFS) = 0x50000000;
			while (bt2ipu_regs[IPU_CTRL] < 0)
				;
		}
		if (hdr & 0x400) { /* VQ colour table in the stream */
			IPU_REG(IPU_CMD_OFS) = 0x60000000;
			while (bt2ipu_regs[IPU_CTRL] < 0)
				;
		}
	}
	IPU_REG(IPU_CTRL_OFS) = (hdr & 0xf3) << 16;
	IPU_REG(IPU_CMD_OFS) = (((hdr >> 2) & 1) << 24) | 0x10010000; /* IDEC, QSC 1, DTD from bit 2 */

	if (hdr & 0x4000) {
		for (row = 0; row < width; row += 16) {
			int stride = ((unsigned int)width >> 4) << 10;
			char *dst = (char *)buf + row * 4;

			for (col = 0; col < height; col += 16) {
				ret = _ipu_store_block(dst, width * 4);
				if (ret <= 0)
					break;
				dst += stride;
			}
			if (ret <= 0)
				break;
		}
		if (ret != 0 || row + 16 < width || col + 16 < height) {
			ret = -1;
			goto stop;
		}
		goto finish;
	} else {
		for (row = 0; row < height; row += 16) {
			int stride = ((unsigned int)width >> 4) << 6;
			char *dst = (char *)buf + row * stride;

			for (col = 0; col < width; col += 16) {
				ret = _ipu_store_block(dst, width * 4);
				if (ret <= 0)
					break;
				dst += 64;
			}
			if (ret <= 0)
				break;
		}
		/* Complete only if the IPU finished (ret == 0) on the last macroblock. */
		if (ret != 0 || row + 16 < height || col + 16 < width) {
			ret = -1;
			goto stop;
		}
		goto finish;
	}

finish:
	if (!(((unsigned int)bt2ipu_regs[IPU_CTRL] >> 15) & 1)) {
		ret = -1;
	} else {
		while (bt2ipu_regs[IPU_CTRL] < 0)
			;
		ret = 0;
	}
stop:
	bt2IpuStopDMA(&bt2ipu_dmaenv);
	return ret;
}

/* TIM2/BTM2 texture loader: reads a file whole into a 16-byte aligned buffer,
   checks the extension against the magic (".tm2" with 'TIM2', ".bt2"
   with 'BTM2') and registers the first picture in the texture manager. A
   negative id becomes 0 (no texture). */


/* Loads one picture and returns its texture id. */
static int Tim2_LoadPicture(void *data, unsigned int size, TIM2_PIC_HEAD *pic, bool alpha, int btm2)
{
	void *raw = 0;
	unsigned char *image;
	unsigned char *clut;
	unsigned char *end;
	int imgFmt;
	int clutFmt;
	int csm = 0;
	int w = pic->width;
	int h = pic->height;
	int id;

	if (btm2) {
		if (pic->pictFormat != 1)
			return -1;
		if (pic->imageType == TIM2_IDTEX8)
			return -1;
		raw = malloc(w * h * 4 + 15);
		if (!raw)
			return -1;
		image = (unsigned char *)(((int)raw + 15) & ~15);
		if (btm2_decode(pic, image) < 0) {
			free(raw);
			return -1;
		}
		clut = 0;
		clutFmt = -1;
		imgFmt = s_tim2Format[pic->imageType - 1].psm;
	} else {
		end = (unsigned char *)data + size;
		image = (unsigned char *)pic + pic->headerSize;
		imgFmt = s_tim2Format[pic->imageType - 1].psm;
		if (image) {
			if (end < image + Texture::CalcSizeofImage(imgFmt, w, h, pic->mipMapTextures))
				return -1;
		}
		if ((unsigned)(pic->imageType - 4) >= 2) {
			clut = 0;
			clutFmt = -1;
		} else {
			clut = image + pic->imageSize;
			csm = (pic->clutType & 0xc0) != 0;
			clutFmt = s_tim2Format[(pic->clutType & 0x3f) - 1].psm;
		}
		if (clut) {
			if (end < clut + Texture::CalcSizeofClut(imgFmt))
				return -1;
		}
	}

	id = Texture::Create(w, h, imgFmt, clutFmt, pic->mipMapTextures, image, clut, csm, alpha);
	if (raw)
		free(raw);
	return id;
}

/* Validates the TIM2/BTM2 header already in memory and loads its first
   picture; returns the texture id, 0 on error. */
static int Tim2_RegisterTexID(void *data, unsigned int size, bool alpha)
{
	TIM2_FILE_HEAD *file = (TIM2_FILE_HEAD *)data;
	TIM2_PIC_HEAD *pic;
	unsigned int hdrSize;
	int btm2 = 0;
	int type;
	int id;
	int texid;

	if (size < sizeof(TIM2_FILE_HEAD))
		return 0;
	if (file->fileId == TIM2_MAGIC) {
		if (file->formatVersion >= 5)
			return 0;
	} else {
		if (file->fileId != BTM2_MAGIC)
			return 0;
		btm2 = 1;
	}
	if (file->formatId >= 2)
		return 0;
	hdrSize = file->formatId == 1 ? 128 : 16;
	pic = (TIM2_PIC_HEAD *)((char *)data + hdrSize);
	if (size < hdrSize + sizeof(TIM2_PIC_HEAD))
		return 0;
	type = pic->imageType;
	if (type >= 6)
		return 0;
	if (type == 0)
		return 0;
	texid = 0;
	id = Tim2_LoadPicture(data, size, pic, alpha, btm2);
	if (id >= 1)
		texid = id;
	return texid;
}

int Tim2_GetTexID(const char *path, bool alpha)
{
	int fd;
	int size;
	void *buf;
	char *ext;
	int id;

	fd = BBopenCPP(path, 0);
	if (fd < 0)
		return 0;
	size = BBlseekCPP(fd, 0, 2);
	BBlseekCPP(fd, 0, 0);
	if (size > 0) {
		buf = memalign(16, size);
		if (buf != 0)
			goto loaded;
	}
	BBcloseCPP(fd);
	return 0;
loaded:
	if (BBreadCPP(fd, buf, size) < size) {
		BBcloseCPP(fd);
		free(buf);
		return 0;
	}
	BBcloseCPP(fd);
	ext = strrchr(path, '.');
	if (strcmp(ext, ".tm2") == 0 && *(int *)buf != TIM2_MAGIC) {
		free(buf);
		return 0;
	}
	if (strcmp(ext, ".bt2") == 0 && *(int *)buf != BTM2_MAGIC) {
		free(buf);
		return 0;
	}
	id = Tim2_RegisterTexID(buf, size, alpha);
	free(buf);
	if (id < 0)
		id = 0;
	return id;
}

int Tim2_GetTexID(void *data, unsigned int size, bool alpha)
{
	int id = Tim2_RegisterTexID(data, size, alpha);

	if (id < 0)
		id = 0;
	return id;
}

void Tim2_ReleaseTexID(int texid)
{
	tmRelease(texid);
}
