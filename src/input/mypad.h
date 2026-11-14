/*
 * myPad: PS2 game pad through the Linux pad driver.
 */

#ifndef MYPAD_H
#define MYPAD_H

#include <linux/ps2/pad.h>

/* The six actuator bytes, copied as one block. */
union myPadAct6 {
	unsigned char b[6];
	short s[3];
};
union myPadBtn {
	unsigned short w;
	unsigned char b[2];
};

class myPad {
public:
	/* Open refcount per (m_port, m_slot): incremented by the ctor, decremented
	   by the dtor, cleared by Init. */
	static int aOpenCount[2][8];

	myPad(int port, int slot);
	~myPad();
	static void Init();
	float CalcStickAngle(unsigned char x, unsigned char y);
	float CalcStickSlantAngle(unsigned char x, unsigned char y);
	void SetAct(int actNo, int strength);
	void _InitButtonValues();
	void _InitButtonValues2();
	void _MakeSmooth();
	void pad_button_read(unsigned short force);
	void GetData(unsigned short force);
	int GetFlags() { return m_buttons[0].w; }

	/* 0x00 */ int m_fd;
	/* 0x04 */ struct ps2pad_modeinfo m_modeInfo;
	/* 0x10 */ int m_padId;  /* 4 digital, 7 analogue (DualShock 2) */
	/* 0x14 */ int m_fdStat;
	/* 0x18 */ unsigned char m_bufStat;  // buf[0]: 0 = valid packet
	/* 0x19 */ unsigned char m_bufMode;  // buf[1]: mode byte
	/* 0x1A */ unsigned short m_bufBtn;  // buf[2..3]: buttons, active low
	/* 0x1C */ unsigned char m_neutral[16];  // raw analogue values, one block (see _InitButtonValues)
	/* 0x2C */ char pad_2C[12];
	/* 0x38 */ int m_readMode;
	/* 0x3C */ unsigned char m_prevMode;
	/* 0x3D */ unsigned char m_lastMode;
	/* 0x3E */ char pad_3E[2];
	/* 0x40 */ struct ps2pad_mode m_mode;
	/* 0x48 */ struct ps2pad_act m_actAlign;  // actuator alignment (len + data[32])
	/* 0x6C */ int m_analog;
	/* 0x70 */ int m_pressMode;
	/* 0x74 */ int m_port;
	/* 0x78 */ int m_slot;
	/* 0x7C */ int m_state;
	/* 0x80 */ int m_retry;
	/* 0x84 */ int m_repeatState;
	/* 0x88 */ int m_repeatCount;
	/* 0x8C */ unsigned short m_repeatBtn;
	/* 0x8E */ unsigned short m_prevBtn;
	/* 0x90 */ int m_axisSmooth[4];  /* filter state of m_axis */
	/* 0xA0 */ int m_pressSmooth[12];  /* filter state of m_press */
	/* 0xD0 */ unsigned char m_actAlignData[6];
	/* 0xD6 */ myPadAct6 m_act;
	/* 0xDC */ myPadBtn m_buttons[4];  /* held, auto-repeat, pressed, released */
	/* 0xE4 */ int m_axis[4];  /* smoothed stick axes (128 = centre), read by PadAnalog */
	/* 0xF4 */ int m_press[12];  /* smoothed button pressures (0..255), read by PadAnalog */
	/* 0x124 */ int m_leftXPrev;
	/* 0x128 */ int m_leftX;
	/* 0x12C */ int m_leftYPrev;
	/* 0x130 */ int m_leftY;
	/* 0x134 */ int m_rightXPrev;
	/* 0x138 */ int m_rightX;
	/* 0x13C */ int m_rightYPrev;
	/* 0x140 */ int m_rightY;
	/* 0x144 */ float m_floatArr[8];
	/* 0x164 */ int m_buttonHistory[6];  /* previous and current state of three buttons */
	/* 0x17C */ int m_togglePrev;
	/* 0x180 */ int m_toggle;
};

/* The pads of port 0 and port 1 (jobinit2.cpp); 0 when not opened. */
extern myPad *pPad0;
extern myPad *pPad1;

#endif
