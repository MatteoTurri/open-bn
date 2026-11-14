/*
 * DNAS1 message layer, sending side; header bits of a DNAS1 message.
 */

#ifndef DNAS1MSG_H
#define DNAS1MSG_H

/* The 16-bit field at offset 2 of a message. */
struct dnas1_msg_bits {
	unsigned short low7 : 7; /* bit 6..0   */
	unsigned short flag : 1; /* bit 7      */
	unsigned short code : 4; /* bit 11..8  */
	unsigned short pad4 : 4; /* bit 15..12 */
};

/* Header of a DNAS1 message, request and reply. */
struct dnas1_msg_hdr {
	/* 0x00 */ unsigned char action[2]; /* the action block */
	/* 0x02 */ struct dnas1_msg_bits bits;
	/* 0x04 */ unsigned char session_id[32];
	/* 0x24 */ unsigned int len; /* big-endian */
}; /* size = 0x28 */

int _sce_dnas1_send_message(int type, unsigned char *msg, int flag);

#endif
