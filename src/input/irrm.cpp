/*
 * IRrm: the IR remote control of the BB Navigator (not polymorphic).
 * m_deviceId is the remote model (2362 or 7002); m_index (0..127) indexes the
 * scancode tables.
 */

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include "input/scerm2.h"
#include "app/syscnf.h"
#include "irrm.h"

/* Port descriptor passed to sceRm2PortOpen, 64-byte aligned like the DMA
   buffers of the kit pad/remote ports. */
static int g_rmPort[4] __attribute__((aligned(64)));

/* Reads the first 45 bytes of the system configuration (EEPROM) from
   /dev/ps2sysconf and returns bit 6 of byte 19; 0 if the device cannot be
   opened or read. */
static int rmBypassNeeded()
{
	unsigned char conf[45];
	int fd;

	fd = open("/dev/ps2sysconf", O_RDWR);
	if (fd < 0) {
		perror("Could not open device file at IRrm\n");
		return 0;
	}
	if (read(fd, conf, 45) < 0) {
		perror("Could not read EEPROM at IRrm \n");
		close(fd);
		return 0;
	}
	close(fd);
	return (conf[19] >> 6) & 1;
}

/* Scancode -> logical code translation, one table per remote model.
   Entries after the last non-zero value are zero. */
/* clang-format off */
static int g_rmCode2362[128] = {
	 1,  2,  3,  4,  5,  6,  7,  8,  9, 10,  0, 11,  0,  0, 12, 13,
	 0,  0,  0,  0,  0, 14,  0,  0,  0,  0, 15, 16,  0,  0,  0, 17,
	 0,  0,  0,  0,  0,  0,  0,  0, 18,  0, 19,  0, 20,  0, 21, 22,
	23, 24, 25, 26, 27, 28,  0,  0, 29, 30,  0,  0,  0,  0,  0,  0,
	 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	 0,  0,  0,  0, 31,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	32, 33,  0, 34, 35, 36,
};

static int g_rmCode7002[128] = {
	37, 38, 39, 40, 41, 42, 43, 44, 45, 46,  0, 47,  0,  0, 48, 49,
	51, 50,  0,  0,  0, 52, 53,  0,  0,  0, 54, 55,  0, 56,  0, 57,
	 0,  0,  0,  0,  0,  0,  0,  0, 58,  0, 59,  0, 60,  0, 61, 62,
	63, 64, 65, 66, 67, 68,  0,  0, 69, 70,  0,  0,  0,  0,  0,  0,
	 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 71,
	72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87,
	88, 89,  0, 90, 91, 92, 93,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	94, 95,
};
/* clang-format on */

/* 1 << i: GetPadCode maps RM codes 72..87 to the corresponding pad button bit. */
/* clang-format off */
static const unsigned short g_padCode[16] = {
	1, 2, 4, 8, 16, 32, 64, 128,
	256, 512, 1024, 2048, 4096, 8192, 16384, 32768,
};
/* clang-format on */

/* Clears the state, starts the library, programs the bypass and opens the
   port; any failure leaves m_stat = -1. m_rmCondition bits:
   0x1 = bypass refused by the CD/DVD driver (-4 from BypassSwitch),
   0x2 = the remote has the MENU/CMD button (syscnf_getRMMenuCMD_C),
   0x4 = it has the system menu (syscnf_getRMSysMenu_C). */
IRrm::IRrm()
{
	m_stat = 0;
	sceRm2Lib_debug = 0;
	m_deviceId = 0;
	m_index = 0;
	m_rmCondition = 0;
	sceRm2Init(0);
	if (!rmBypassNeeded())
		m_bypass = 1;
	else
		m_bypass = 0;
	switch (BypassSwitch(m_bypass)) {
	case 0:
		break;
	case -4:
		m_stat = -1;
		m_rmCondition |= 1;
		return;
	default:
		m_stat = -1;
		m_rmCondition = 0;
		return;
	}
	if (sceRm2PortOpen(g_rmPort) != 1) {
		m_stat = -1;
		m_rmCondition = 0;
		return;
	}
	{
		sceRm2IRFeature feature;

		sceRm2GetIRFeature(&feature);
		if ((feature.flags ^ 1) & 1) {
			m_stat = -1;
			return;
		}
	}
	if (syscnf_getRMMenuCMD_C())
		m_rmCondition |= 2;
	if (syscnf_getRMSysMenu_C())
		m_rmCondition |= 4;
}

/* Closes the port and shuts down the library. */
IRrm::~IRrm()
{
	sceRm2PortClose();
	sceRm2End();
}

/* Programs the remote "bypass" through the CD/DVD driver: opens
   /dev/ps2cdvdstat O_RDWR, sends an 8-byte _IOWR ioctl (0xC0085604) with the
   (command, state) pair and reads two error bits in the second word.
   Returns -1 open failed, -2 ioctl failed, -3 status bit 0x80, -4 status bit
   0x100, 0 = done (only then is m_bypass updated). */
int IRrm::BypassSwitch(char enable)
{
	int req[2];
	int fd;

	if (enable)
		req[0] = 1;
	else
		req[0] = 0;
	fd = open("/dev/ps2cdvdstat", O_RDWR);
	if (fd == -1)
		return -1;
	if (ioctl(fd, 0xc0085604, req) < 0) {
		close(fd);
		return -2;
	}
	if (req[1] & 0x80) {
		close(fd);
		return -3;
	}
	if (req[1] & 0x100) {
		close(fd);
		return -4;
	}
	m_bypass = req[0];
	close(fd);
	return 0;
}

int IRrm::GetStat()
{
	return m_stat;
}

/* Driver state -> m_stat; in state 3 (ready) reads an event.
   State 2 -> 1, state 0 -> -3, state 4 -> -4, any other -> 0; state 3 leaves
   m_stat unchanged. */
int IRrm::GetSignal()
{
	unsigned short deviceId;
	unsigned char index;

	int state = sceRm2GetState();

	if (state == 3) {
		if (sceRm2Read(&deviceId, &index) > 0) {
			if (m_deviceId != deviceId || (unsigned char)m_index != index) {
				m_deviceId = deviceId;
				m_index = index;
			}
		} else {
			m_deviceId = 0;
			m_index = 0;
		}
	} else
		switch (state) {
		case 0:
			m_stat = -3;
			break;
		case 2:
			m_stat = 1;
			break;
		case 4:
			m_stat = -4;
			break;
		default:
			m_stat = 0;
			break;
		}
	return 0;
}

int IRrm::GetRMCode()
{
	if (m_index < 0)
		return 0;
	switch (m_deviceId) {
	case 2362:
		return g_rmCode2362[(unsigned char)m_index];
	case 7002:
		return g_rmCode7002[(unsigned char)m_index];
	}
	return 0;
}

int IRrm::GetRMCondition()
{
	return m_rmCondition;
}

unsigned short IRrm::GetPadCode()
{
	int code = GetRMCode() - 72;

	if ((unsigned int)code < 16)
		return g_padCode[code];
	return 0;
}
