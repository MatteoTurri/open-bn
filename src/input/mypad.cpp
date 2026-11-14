/*
 * myPad: PS2 game pad through the Linux pad driver (/dev/ps2padNM): open
 * refcounts, the init/query state machine, button and analogue stick reading
 * with auto-repeat and smoothing, and the actuators.
 */

#include <math.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/ps2/pad.h>   /* PS2PAD_IOCSETACTALIGN */

#include "mypad.h"

int myPad::aOpenCount[2][8];

/* The driver struct ps2pad_act (len + 32 bytes of data); the driver uses
 * 6 of the 32 bytes (one per actuator). */
struct myPadActReq {
	int count;
	union {
		myPadAct6 act;
		unsigned char data[32];
	};
};

/* Pad driver request results, one per step of the init sequence. */
static int s_setModeStat;
static int s_pressModeInfo;
static int s_pressModeStat;
static int s_actAlignStat;
static myPadActReq s_initActReq;
static int s_setActStat;
static myPadActReq s_actReq;

/* Clears the 2x8 open refcount table. */
void myPad::Init()
{
	int i, j, next;

	i = 0;
	do {
		next = i + 1;
		j = 7;
		do {
			aOpenCount[i][j] = 0;
			j--;
		} while (j >= 0);
		i = next;
	} while (i < 2);
}

/* Opens the two devices ("/dev/ps2pad%d%d" RW/NONBLOCK, "/dev/ps2padstat"
 * NONBLOCK), increments the open refcount, resets the button values and
 * initialises the remaining fields. */
myPad::myPad(int port, int slot)
{
	char path[128];

	sprintf(path, "/dev/ps2pad%d%d", port, slot);
	m_fd = open(path, O_RDWR | O_NONBLOCK);
	if (m_fd >= 0) {
		sprintf(path, "/dev/ps2padstat");
		m_fdStat = open(path, O_RDONLY | O_NONBLOCK);
		if (m_fdStat < 0) {
			close(m_fd);
			m_fd = -1;
			return;
		}
		m_state = 0;
		m_lastMode = 0;
		m_analog = 0;
		m_pressMode = 0;
		aOpenCount[port][slot]++;
		m_port = port;
		m_slot = slot;
		_InitButtonValues();
		_InitButtonValues2();
		m_rightY = 0;
		m_rightX = 0;
		m_leftY = 0;
		m_leftX = 0;
		m_buttonHistory[5] = 0;
		m_buttonHistory[4] = 0;
		m_buttonHistory[3] = 0;
		m_buttonHistory[2] = 0;
		m_buttonHistory[1] = 0;
		m_buttonHistory[0] = 0;
		m_toggle = 0;
		m_togglePrev = 0;
		m_actAlignData[0] = 0;
		m_actAlignData[1] = 1;
		m_actAlignData[2] = 255;
		m_actAlignData[3] = 255;
		m_actAlignData[4] = 255;
		m_actAlignData[5] = 255;
		m_act.b[0] = 0;
		float z = 0.0f;
		m_floatArr[6] = z;
		m_act.b[1] = 0;
		m_floatArr[4] = z;
		m_floatArr[2] = z;
		m_floatArr[0] = z;
		m_floatArr[7] = z;
		m_floatArr[5] = z;
		m_floatArr[3] = z;
		m_floatArr[1] = z;
	}
}

/* Closes the two fds (if open) and decrements the open refcount. */
myPad::~myPad()
{
	if (m_fdStat >= 0)
		close(m_fdStat);
	if (m_fd >= 0)
		close(m_fd);
	aOpenCount[m_port][m_slot]--;
	m_fd = -1;
}

/*
 * State machine of the pad init/query sequence (m_state). Every state issues
 * an ioctl on /dev/ps2padNM and advances (or resets) the state; _MakeSmooth
 * always runs at the end.
 *  0        reset: _InitButtonValues, reads the port state (PS2PAD_IOCGETSTAT);
 *           port 0 means no pad (read anyway), 2 and 3 busy.
 *  1        waits for the 4 low bits of the current mode id.
 *  2        dispatches on the mode found: 4 -> 40, 7 -> 70, other -> 99.
 *  40/41/42 mode 4 configuration chain.
 *  70/76/77 mode 7 chain (76 is the only two-argument ioctl).
 *  80/81    sends the actuator alignment (PS2PAD_IOCSETACTALIGN).
 *  82/83    sends the actuators (PS2PAD_IOCSETACT), same packet as SetAct.
 *  99       error (no case: falls into default).
 * m_retry counts the retries, up to 31. default re-reads the port
 * state: 3 or 0 go back to 0, 1 reads the buttons and updates m_prevMode.
 */
void myPad::GetData(unsigned short force)
{
	if (m_fd < 0)
		return;

	switch (m_state) {
	case 0:
		_InitButtonValues();
		m_analog = 0;
		m_pressMode = 0;
		ioctl(m_fd, PS2PAD_IOCGETSTAT, &m_readMode);
		if (m_readMode == 0) {
			pad_button_read(force);
			break;
		}
		if (m_readMode == 3)
			break;
		if (m_readMode == 2)
			break;
		m_retry = 0;
		m_modeInfo.term = 1;
		ioctl(m_fd, PS2PAD_IOCMODEINFO, &m_modeInfo);
		m_state = m_state + 1;
		break;
	case 1:
		m_padId = 0;
		if ((m_modeInfo.result & 0xf) == 0) {
			m_state = 0;
			break;
		}
		m_padId = m_modeInfo.result & 0xff;
		m_modeInfo.term = 2;
		ioctl(m_fd, PS2PAD_IOCMODEINFO, &m_modeInfo);
		m_state = m_state + 1;
		break;
	case 2:
		if (m_modeInfo.result > 0)
			m_padId = m_modeInfo.result;
		switch (m_padId) {
		case 4:
			m_state = 40;
			break;
		case 7:
			m_state = 70;
			break;
		default:
			m_state = 99;
			break;
		}
		break;
	case 40: {
		struct ps2pad_modeinfo req;

		req.term = 4;
		req.offs = -1;
		ioctl(m_fd, PS2PAD_IOCMODEINFO, &req);
		if (req.result == 0) {
			m_state = 99;
			break;
		}
		m_state = m_state + 1;
		break;
	}
	case 41:
		m_mode.offs = 1;
		m_mode.lock = 3;
		ioctl(m_fd, PS2PAD_IOCSETMODE, &m_mode);
		m_state = m_state + 1;
		break;
	case 42:
		ioctl(m_fd, PS2PAD_IOCGETREQSTAT, &s_setModeStat);
		if (s_setModeStat == 1)
			m_state = 0;
		if (s_setModeStat != 0)
			break;
		m_analog = 1;
		m_state = 70;
		break;
	case 70:
		m_analog = 1;
		ioctl(m_fd, PS2PAD_IOCPRESSMODEINFO, &s_pressModeInfo);
		if (s_pressModeInfo != 1) {
			m_state = 99;
			break;
		}
		m_state = 76;
		break;
	case 76:
		ioctl(m_fd, PS2PAD_IOCENTERPRESSMODE);
		m_state = m_state + 1;
		break;
	case 77:
		ioctl(m_fd, PS2PAD_IOCGETREQSTAT, &s_pressModeStat);
		if (s_pressModeStat == 1) {
			m_state = 0;
			break;
		}
		if (s_pressModeStat == 0) {
			m_pressMode = 1;
			m_state = 80;
			break;
		}
		m_retry = m_retry + 1;
		if (m_retry < 31)
			break;
		m_state = 0;
		break;
	case 80:
		m_actAlign.len = 6;
		memset(m_actAlign.data, 0, sizeof m_actAlign.data);
		memcpy(m_actAlign.data, m_actAlignData, sizeof m_actAlignData);
		ioctl(m_fd, PS2PAD_IOCSETACTALIGN, &m_actAlign);
		m_state = m_state + 1;
		break;
	case 81:
		ioctl(m_fd, PS2PAD_IOCGETREQSTAT, &s_actAlignStat);
		if (s_actAlignStat == 1) {
			m_state = 0;
			break;
		}
		if (s_actAlignStat != 0) {
			m_retry = m_retry + 1;
			if (m_retry < 31)
				break;
			m_state = 99;
			break;
		}
		m_state = m_state + 1;
		break;
	case 82:
		s_initActReq.count = 6;
		s_initActReq.act = m_act;
		ioctl(m_fd, PS2PAD_IOCSETACT, &s_initActReq);
		m_state = m_state + 1;
		break;
	case 83:
		ioctl(m_fd, PS2PAD_IOCGETREQSTAT, &s_setActStat);
		if (s_setActStat == 1) {
			m_state = m_state - 1;
			break;
		}
		if (s_setActStat == 0) {
			m_state = 99;
			break;
		}
		m_retry = m_retry + 1;
		if (m_retry < 31)
			break;
		m_state = 99;
		break;
	default:
		ioctl(m_fd, PS2PAD_IOCGETSTAT, &m_readMode);
		if (m_readMode == 3)
			m_state = 0;
		if (m_readMode == 0)
			m_state = 0;
		if (m_readMode != 1)
			break;
		pad_button_read(force);
		if (m_lastMode != 0 && m_lastMode != m_prevMode) {
			m_lastMode = 0;
			m_state = 0;
		}
		m_lastMode = m_prevMode;
		break;
	}
	_MakeSmooth();
}

/* Sets the force of actuator actNo (clamped to 255) and always sends the whole
 * m_act array with an ioctl (PS2PAD_IOC_SETACTALIGN); ignores actNo >= 6. */
void myPad::SetAct(int actNo, int strength)
{
	if (actNo < 6) {
		if (strength > 255)
			strength = 255;
		m_act.b[actNo] = strength;
		s_actReq.count = 6;
		s_actReq.act = m_act;
		ioctl(m_fd, PS2PAD_IOCSETACT, &s_actReq);
	}
}

/* Angle of the stick from its centre: atan2(x - 128, 128 - y). */
float myPad::CalcStickAngle(unsigned char x, unsigned char y)
{
	float fx = (float)((int)x - 128);
	float fy = (float)(128 - (int)y);
	return (float)atan2(fx, fy);
}

/* Stick tilt: sqrt(dx*dx + dy*dy) / 128. */
float myPad::CalcStickSlantAngle(unsigned char x, unsigned char y)
{
	int dx = x - 128;
	int dy = y - 128;

	dx *= dx;
	dy *= dy;
	return (float)(sqrt((double)(float)(dx + dy)) * 0.0078125);
}

/*
 * Reads a pad packet (32 bytes at this+24), merges the buttons read with the
 * caller's `force` mask, computes the press/release edges, runs the
 * auto-repeat state machine and integrates the two analogue sticks into a
 * floating-point position.
 * Packet (mode 0x79): +0 status (0 = valid), +1 mode, +2..3 buttons (active
 * low), +4 RX +5 RY +6 LX +7 LY, +8..19 pressures of right, left, up, down,
 * triangle, circle, cross, square, L1, R1, L2, R2.
 * Button bits: 0x10 up, 0x20 right, 0x40 down, 0x80 left, 0x100 L2,
 * 0x200 R2, 0x400 L1, 0x800 R1, 0x1000 triangle, 0x2000 circle,
 * 0x4000 cross, 0x8000 square. A button forced by `force` gets pressure 0xff.
 * Stick integration: pos += sign(d) * d * d * 0.002f, clamped to [0, 640] on
 * X and [0, 896] on Y; the integer copy is (int)(pos + 0.5f).
 */
void myPad::pad_button_read(unsigned short force)
{
	myPadBtn cur;
	myPadBtn prevBtn;
	myPadBtn chg;
	unsigned short raw;
	int merged;
	short dx, dy;
	unsigned char c;

	if (m_readMode == 1) {
		if (read(m_fd, &m_bufStat, 32) == 0)
			return;
		if (m_bufStat != 0)
			return;
	} else {
		m_bufStat = 0;
		m_bufBtn = 0xffff;
		m_neutral[0] = 128;
		m_neutral[1] = 128;
		m_neutral[2] = 128;
		m_neutral[3] = 128;
		m_neutral[4] = 0;
		m_neutral[5] = 0;
		m_neutral[6] = 0;
		m_neutral[7] = 0;
		m_neutral[8] = 0;
		m_neutral[9] = 0;
		m_neutral[10] = 0;
		m_neutral[11] = 0;
		m_neutral[12] = 0;
		m_neutral[13] = 0;
		m_neutral[14] = 0;
		m_neutral[15] = 0;
		m_bufMode = m_prevMode;
	}
	raw = m_bufBtn;
	merged = force | ~raw;
	m_prevBtn = m_buttons[0].w;
	m_buttons[0].w = merged;
	m_prevMode = m_bufMode;
	if ((m_buttons[0].w & 0x80) && (m_buttons[0].w & 0x20))
		m_buttons[0].w = merged & 0xffdf;
	if ((m_buttons[0].w & 0x10) && (m_buttons[0].w & 0x40))
		m_buttons[0].w = m_buttons[0].w & 0xffbf;

	if (force != 0) {
		{
			int r = m_neutral[4];
			int t = r | 0xff;

			if ((force & 0x20) == 0)
				t = r;
			m_neutral[4] = t;
		}
		{
			int r = m_neutral[5];
			int t = r | 0xff;

			if ((force & 0x80) == 0)
				t = r;
			m_neutral[5] = t;
		}
		{
			int r = m_neutral[6];
			int t = r | 0xff;

			if ((force & 0x10) == 0)
				t = r;
			m_neutral[6] = t;
		}
		{
			int r = m_neutral[7];
			int t = r | 0xff;

			if ((force & 0x40) == 0)
				t = r;
			m_neutral[7] = t;
		}
		{
			int r = m_neutral[9];
			int t = r | 0xff;

			if ((force & 0x2000) == 0)
				t = r;
			m_neutral[9] = t;
		}
		{
			int r = m_neutral[11];
			int t = r | 0xff;

			if ((force & 0x8000) == 0)
				t = r;
			m_neutral[11] = t;
		}
		{
			int r = m_neutral[8];
			int t = r | 0xff;

			if ((force & 0x1000) == 0)
				t = r;
			m_neutral[8] = t;
		}
		{
			int r = m_neutral[10];
			int t = r | 0xff;

			if ((force & 0x4000) == 0)
				t = r;
			m_neutral[10] = t;
		}
		{
			int r = m_neutral[12];
			int t = r | 0xff;

			if ((force & 0x400) == 0)
				t = r;
			m_neutral[12] = t;
		}
		{
			int r = m_neutral[14];
			int t = r | 0xff;

			if ((force & 0x100) == 0)
				t = r;
			m_neutral[14] = t;
		}
		{
			int r = m_neutral[13];
			int t = r | 0xff;

			if ((force & 0x800) == 0)
				t = r;
			m_neutral[13] = t;
		}
		{
			int r = m_neutral[15];
			int t = r | 0xff;

			if ((force & 0x200) == 0)
				t = r;
			m_neutral[15] = t;
		}
	}

	cur = m_buttons[0];
	prevBtn.w = m_prevBtn;
	chg.w = cur.w ^ prevBtn.w;
	m_buttons[2].w = cur.w & chg.w;
	m_buttons[3].w = prevBtn.w & chg.w;
	switch (m_repeatState) {
	case 0:
		if (cur.w == 0)
			break;
		m_repeatBtn = cur.w;
		m_repeatCount = 10;
		m_repeatState = 1;
		goto emitRepeat;
	case 1:
		if ((m_repeatBtn & cur.w) != 0) {
			m_repeatCount--;
			if (m_repeatCount > 0)
				break;
			m_repeatCount = 2;
			m_repeatState = 2;
			break;
		}
		m_repeatState = 0;
		goto emitRepeat;
	case 2:
		if ((m_repeatBtn & cur.w) != 0) {
			m_repeatCount--;
			if (m_repeatCount != 0)
				break;
			m_repeatCount = 2;
			goto emitRepeat;
		}
		m_repeatState = 0;
		goto emitRepeat;
	default:
		goto repeatDone;
	}
	m_buttons[1].w = 0;
	goto repeatDone;
emitRepeat:
	m_buttons[1].w = cur.w;
repeatDone:

	if (m_analog == 0) {
		unsigned char *p = &m_bufStat;

		p[4] = 128;
		p[5] = 128;
		p[6] = 128;
		m_neutral[3] = 128;
	}
	m_buttonHistory[0] = m_buttonHistory[1];
	m_buttonHistory[1] = m_buttons[0].w & 2;
	m_buttonHistory[2] = m_buttonHistory[3];
	m_buttonHistory[3] = m_buttons[0].w & 0x400;
	m_buttonHistory[4] = m_buttonHistory[5];
	m_buttonHistory[5] = m_buttons[0].w & 0x100;
	m_togglePrev = m_toggle;
	if (m_buttons[2].w & 2)
		m_toggle = m_toggle ^ 1;

	dx = 0;
	dy = 0;
	c = m_neutral[2];
	if (c < 108)
		dx = c - 108;
	else if (c >= 149)
		dx = c - 148;
	c = m_neutral[3];
	if (c < 108)
		dy = c - 108;
	else if (c >= 149)
		dy = c - 148;

	{
		float fx = (float)dx;
		float sx = 1.0f;

		m_floatArr[0] = m_floatArr[1];
		if (fx < 0.0f)
			sx = -1.0f;
		m_floatArr[1] = m_floatArr[1] + sx * fx * fx * 0.002f;
		if (m_floatArr[1] < 0.0f)
			m_floatArr[1] = 0.0f;
		else if (m_floatArr[1] > 640.0f)
			m_floatArr[1] = 640.0f;
		m_leftXPrev = m_leftX;
		m_leftX = (int)(m_floatArr[1] + 0.5f);
	}

	{
		float fy = (float)dy;
		float sy = 1.0f;

		m_floatArr[2] = m_floatArr[3];
		if (fy < 0.0f)
			sy = -1.0f;
		m_floatArr[3] = m_floatArr[3] + sy * fy * fy * 0.002f;
		if (m_floatArr[3] < 0.0f)
			m_floatArr[3] = 0.0f;
		else if (m_floatArr[3] > 896.0f)
			m_floatArr[3] = 896.0f;
		m_leftYPrev = m_leftY;
		m_leftY = (int)(m_floatArr[3] + 0.5f);
	}

	dx = 0;
	dy = 0;
	c = m_neutral[0];
	if (c < 108)
		dx = c - 108;
	else if (c >= 149)
		dx = c - 148;
	c = m_neutral[1];
	if (c < 108)
		dy = c - 108;
	else if (c >= 149)
		dy = c - 148;

	{
		float fx = (float)dx;
		float sx = 1.0f;

		m_floatArr[4] = m_floatArr[5];
		if (fx < 0.0f)
			sx = -1.0f;
		m_floatArr[5] = m_floatArr[5] + sx * fx * fx * 0.002f;
		if (m_floatArr[5] < 0.0f)
			m_floatArr[5] = 0.0f;
		else if (m_floatArr[5] > 640.0f)
			m_floatArr[5] = 640.0f;
		m_rightXPrev = m_rightX;
		m_rightX = (int)(m_floatArr[5] + 0.5f);
	}

	{
		float fy = (float)dy;
		float sy = 1.0f;

		m_floatArr[6] = m_floatArr[7];
		if (fy < 0.0f)
			sy = -1.0f;
		m_floatArr[7] = m_floatArr[7] + sy * fy * fy * 0.002f;
		if (m_floatArr[7] < 0.0f)
			m_floatArr[7] = 0.0f;
		else if (m_floatArr[7] > 896.0f)
			m_floatArr[7] = 896.0f;
		m_rightYPrev = m_rightY;
		m_rightY = (int)(m_floatArr[7] + 0.5f);
	}
}

/* Exponential moving average over 16 channels: the first 4 (analogue axes)
 * with alpha 1/4, the other 12 (pressure-sensitive buttons) with alpha 1/16.
 * The filtered values accumulate in m_axisSmooth/m_pressSmooth and are mirrored in
 * m_axis/m_press; the raw values are read from m_neutral in the order
 * 3,2,1,0,4,5,6,7,9,11,8,10,12,14,13,15. */
void myPad::_MakeSmooth()
{
	m_axisSmooth[0] += (m_neutral[3] - m_axisSmooth[0]) >> 2;
	m_axis[0] = m_axisSmooth[0];
	m_axisSmooth[1] += (m_neutral[2] - m_axisSmooth[1]) >> 2;
	m_axis[1] = m_axisSmooth[1];
	m_axisSmooth[2] += (m_neutral[1] - m_axisSmooth[2]) >> 2;
	m_axis[2] = m_axisSmooth[2];
	m_axisSmooth[3] += (m_neutral[0] - m_axisSmooth[3]) >> 2;
	m_axis[3] = m_axisSmooth[3];
	m_pressSmooth[0] += (m_neutral[4] - m_pressSmooth[0]) >> 4;
	m_press[0] = m_pressSmooth[0];
	m_pressSmooth[1] += (m_neutral[5] - m_pressSmooth[1]) >> 4;
	m_press[1] = m_pressSmooth[1];
	m_pressSmooth[2] += (m_neutral[6] - m_pressSmooth[2]) >> 4;
	m_press[2] = m_pressSmooth[2];
	m_pressSmooth[3] += (m_neutral[7] - m_pressSmooth[3]) >> 4;
	m_press[3] = m_pressSmooth[3];
	m_pressSmooth[4] += (m_neutral[9] - m_pressSmooth[4]) >> 4;
	m_press[4] = m_pressSmooth[4];
	m_pressSmooth[5] += (m_neutral[11] - m_pressSmooth[5]) >> 4;
	m_press[5] = m_pressSmooth[5];
	m_pressSmooth[6] += (m_neutral[8] - m_pressSmooth[6]) >> 4;
	m_press[6] = m_pressSmooth[6];
	m_pressSmooth[7] += (m_neutral[10] - m_pressSmooth[7]) >> 4;
	m_press[7] = m_pressSmooth[7];
	m_pressSmooth[8] += (m_neutral[12] - m_pressSmooth[8]) >> 4;
	m_press[8] = m_pressSmooth[8];
	m_pressSmooth[9] += (m_neutral[14] - m_pressSmooth[9]) >> 4;
	m_press[9] = m_pressSmooth[9];
	m_pressSmooth[10] += (m_neutral[13] - m_pressSmooth[10]) >> 4;
	m_press[10] = m_pressSmooth[10];
	m_pressSmooth[11] += (m_neutral[15] - m_pressSmooth[11]) >> 4;
	m_press[11] = m_pressSmooth[11];
}

/* Resets the buttons, the auto-repeat state and the analogue values of the
 * packet (sticks to 128 = centre, pressures to 0). */
void myPad::_InitButtonValues()
{
	unsigned char *p = &m_bufStat;
	m_buttons[3].w = 0;
	m_buttons[2].w = 0;
	m_buttons[1].w = 0;
	m_buttons[0].w = 0;
	m_repeatState = 0;
	m_repeatBtn = 0;
	m_repeatCount = 0;
	m_prevBtn = 0;
	p[4] = 128;
	p[5] = 128;
	p[6] = 128;
	m_neutral[3] = 128;
	p[19] = 0;
	p[17] = 0;
	p[18] = 0;
	p[16] = 0;
	p[14] = 0;
	p[12] = 0;
	p[15] = 0;
	p[13] = 0;
	p[11] = 0;
	p[10] = 0;
	p[9] = 0;
	m_neutral[4] = 0;
}

/* Two identical blocks {int[4] = 128; int[12] = 0}, one per analogue axis pair. */
void myPad::_InitButtonValues2()
{
	m_axisSmooth[3] = 128;
	m_axisSmooth[2] = 128;
	m_axisSmooth[1] = 128;
	m_axisSmooth[0] = 128;
	m_pressSmooth[11] = 0;
	m_pressSmooth[10] = 0;
	m_pressSmooth[9] = 0;
	m_pressSmooth[8] = 0;
	m_pressSmooth[7] = 0;
	m_pressSmooth[6] = 0;
	m_pressSmooth[5] = 0;
	m_pressSmooth[4] = 0;
	m_pressSmooth[3] = 0;
	m_pressSmooth[2] = 0;
	m_pressSmooth[1] = 0;
	m_pressSmooth[0] = 0;
	m_axis[3] = 128;
	m_axis[2] = 128;
	m_axis[1] = 128;
	m_axis[0] = 128;
	m_press[11] = 0;
	m_press[10] = 0;
	m_press[9] = 0;
	m_press[8] = 0;
	m_press[7] = 0;
	m_press[6] = 0;
	m_press[5] = 0;
	m_press[4] = 0;
	m_press[3] = 0;
	m_press[2] = 0;
	m_press[1] = 0;
	m_press[0] = 0;
}
