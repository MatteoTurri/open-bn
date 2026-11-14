/*
 * myUsbMouse: USB mouse device. Opens it read/write (read-only as fallback)
 * and, when writable, sends it a 5-byte init sequence.
 */
#include <sys/select.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

class myUsbMouse {
public:
	myUsbMouse();
	~myUsbMouse();
	void Init();

	static char *pDevice;

	int GetData();

private:
	/* 0x00 */ int m_pad0;
	/* 0x04 */ int m_pad4;
	/* 0x08 */ int m_count;
	/* 0x0C */ int m_prevButtons;
	/* 0x10 */ int m_buttons;
	/* 0x14 */ int m_prevDx;
	/* 0x18 */ int m_dx;
	/* 0x1C */ int m_prevDy;
	/* 0x20 */ int m_dy;
	/* 0x24 */ int m_prevDz;
	/* 0x28 */ int m_dz;
	/* 0x2C */ int m_fd;
	/* 0x30 */ int m_opened;
	/* 0x34 */ int m_writable;
	/* 0x38 */ unsigned char m_data[4];
};

/* select timeout: an initialised template and a working copy, rewritten on
 * every call because select consumes it. */
char *myUsbMouse::pDevice = "/dev/mouse";
static unsigned char s_initSeq[5] = { 0xe8, 0x03, 0xe6, 0xe6, 0xe6 };

static struct timeval s_selTimeoutTemplate = { 0, 0 };
static struct timeval s_selTimeout;

void myUsbMouse::Init()
{}

myUsbMouse::myUsbMouse()
{
	m_writable = 0;
	m_opened = 0;
	m_fd = open(pDevice, O_RDWR);
	if (m_fd < 0) {
		m_fd = open(pDevice, O_RDONLY);
		if (m_fd >= 0) {
			fprintf(stderr, "\"%s\" device file is readonly.\n", pDevice);
			m_opened = 1;
		} else {
			fprintf(stderr, "Can not open %s.\n", pDevice);
		}
	} else {
		m_writable = 1;
		m_opened = 1;
	}
	if (m_writable)
		write(m_fd, s_initSeq, 5);
}

myUsbMouse::~myUsbMouse()
{
	if (m_fd > 0)
		close(m_fd);
}

/* Reads 4-byte mouse packets until it has two, using select with a timeout
 * so as not to block. Byte 0 is unsigned (bit 3 = valid packet, bits 0-2 =
 * buttons), bytes 1-3 are signed; dy is stored negated. */
int myUsbMouse::GetData()
{
	fd_set fds;
	int count = 0;

	if (m_opened == 0)
		return 0;

	int dz = 0;
	int dy = 0;
	int dx = 0;
	int buttons = 0;

	do {
		int n;
		FD_ZERO(&fds);
		int fd = m_fd;
		FD_SET(fd, &fds);
		s_selTimeout = s_selTimeoutTemplate;
		n = select(fd + 1, &fds, 0, 0, &s_selTimeout);
		if (!FD_ISSET(m_fd, &fds))
			n = 0;
		if (n == 0)
			break;
		int len = read(m_fd, m_data, 4);
		if ((m_data[0] & 8) != 0) {
			count++;
			buttons = m_data[0] & 7;
			dx += (signed char)m_data[1];
			dy += (signed char)m_data[2];
			if (len >= 4)
				dz += (signed char)m_data[3];
			else
				dz = 0;
		}
	} while (count < 2);

	if (count != 0) {
		m_prevButtons = m_buttons;
		m_buttons = buttons;
		m_prevDx = m_dx;
		m_dx = dx;
		m_prevDy = m_dy;
		m_dy = -dy;
		m_prevDz = m_dz;
		m_dz = dz;
		m_count = count;
	}
	return count;
}
