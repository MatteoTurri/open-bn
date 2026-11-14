/*
 * Physical keyboard of the IME front end, read from the Linux console in
 * medium-raw mode, and the pseudo pad that maps keys to PS2 pad buttons
 * (myUsbKeyboard_*).
 */

#include "textim_debug.h"
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/kd.h>
#include "textim_glob.h"
#include "conkbd.h"
#include "fepbar.h"
#include "fepmain.h"
#include "skbmain.h"


#define CONKBD_NOSUCHMAP  639   /* K_NOSUCHMAP  */
#define CONKBD_SPEC126    638   /* K(KT_SPEC,126) */


static int g_conOrigKbdMode;      /* keyboard mode before conkbd_init */
static struct termios g_conOrigTermios; /* termios before conkbd_init */
static unsigned char g_conKeyBuf[16];
static int g_keyToIndex[256];   /* scancode -> key index, -1 = unmapped */
static int g_indexToKey[256];   /* key index -> scancode */
static int g_keyCount;
static int g_keySpecCount;
static int g_conLedInit;       /* modifier and lock bits */
static int g_kanaLock;
static int g_kanaLockFix;
static char g_conKbdLed;       /* KDGKBLED state */

/* X11 half-width katakana keysym (0xFF61..0xFF9F) -> ASCII key of the
   romaji row, 62 entries and a {0, 0} terminator. */
struct conkbd_kanamap {
	int kana;
	int key;
};

/* clang-format off */
static struct conkbd_kanamap g_conKanaMap[63] = {
	{ 0xff87, 0x31 }, { 0xff8c, 0x32 }, { 0xff71, 0x33 }, { 0xff73, 0x34 },
	{ 0xff74, 0x35 }, { 0xff75, 0x36 }, { 0xff94, 0x37 }, { 0xff95, 0x38 },
	{ 0xff96, 0x39 }, { 0xff9c, 0x30 }, { 0xff8e, 0x2d }, { 0xff8d, 0x5e },
	{ 0xff80, 0x71 }, { 0xff83, 0x77 }, { 0xff72, 0x65 }, { 0xff7d, 0x72 },
	{ 0xff76, 0x74 }, { 0xff9d, 0x79 }, { 0xff85, 0x75 }, { 0xff86, 0x69 },
	{ 0xff97, 0x6f }, { 0xff7e, 0x70 }, { 0xff9e, 0x40 }, { 0xff9f, 0x5b },
	{ 0xff81, 0x61 }, { 0xff84, 0x73 }, { 0xff7c, 0x64 }, { 0xff8a, 0x66 },
	{ 0xff77, 0x67 }, { 0xff78, 0x68 }, { 0xff8f, 0x6a }, { 0xff89, 0x6b },
	{ 0xff98, 0x6c }, { 0xff9a, 0x3b }, { 0xff79, 0x3a }, { 0xff91, 0x5d },
	{ 0xff82, 0x7a }, { 0xff7b, 0x78 }, { 0xff7f, 0x63 }, { 0xff8b, 0x76 },
	{ 0xff7a, 0x62 }, { 0xff90, 0x6e }, { 0xff93, 0x6d }, { 0xff88, 0x2c },
	{ 0xff99, 0x2e }, { 0xff92, 0x2f }, { 0xff9b, 0x5c }, { 0xff67, 0x23 },
	{ 0xff69, 0x24 }, { 0xff6a, 0x25 }, { 0xff6b, 0x26 }, { 0xff6c, 0x27 },
	{ 0xff6d, 0x28 }, { 0xff6e, 0x29 }, { 0xff66, 0x7e }, { 0xff68, 0x45 },
	{ 0xff62, 0x7b }, { 0xff63, 0x7d }, { 0xff6f, 0x5a }, { 0xff64, 0x3c },
	{ 0xff61, 0x3e }, { 0xff65, 0x3f }, { 0, 0 },
};
/* clang-format on */

static int g_conKanaPending = 1;
static int g_conTtyFd = -1;
static int g_conOpenFlag = 0;     /* tty fd is ours to close */

/* Special key handlers, indexed by keysym type (KT_*). */
static int conkbd_key_latin(int key, int up, int mod);
static int conkbd_key_fn(int key, int up, int mod);
static int conkbd_key_led(int key, int up, int mod);
static int conkbd_key_pad(int key, int up, int mod);
static int conkbd_key_dead(int key, int up, int mod);
static int conkbd_key_cons(int key, int up, int mod);
static int conkbd_key_fnrow(int key, int up, int mod);
static int conkbd_key_setbit(int key, int up, int mod);
static int conkbd_key_space(int key, int up, int mod);
static int conkbd_key_ascii(int key, int up, int mod);
static int conkbd_key_lock(int key, int up, int mod);
static int conkbd_key_letter(int key, int up, int mod);

typedef int (*conkbd_keyfn)(int key, int up, int mod);

static conkbd_keyfn g_conKeyHandlers[12] = {
	conkbd_key_latin,
	conkbd_key_fn,
	conkbd_key_led,
	conkbd_key_pad,
	conkbd_key_dead,
	conkbd_key_cons,
	conkbd_key_fnrow,
	conkbd_key_setbit,
	conkbd_key_space,
	conkbd_key_ascii,
	conkbd_key_lock,
	conkbd_key_letter,
};

unsigned short myUsbKeyboard_LastPseudoPadData = 0;

/* PS2 pad button masks, in the order of the g_padKeys lists. */
static unsigned short g_padBit[16] = {
	TEXTIM_PAD_L1,
	TEXTIM_PAD_L2,
	TEXTIM_PAD_L3,
	TEXTIM_PAD_UP,
	TEXTIM_PAD_DOWN,
	TEXTIM_PAD_RIGHT,
	TEXTIM_PAD_LEFT,
	TEXTIM_PAD_SELECT,
	TEXTIM_PAD_START,
	TEXTIM_PAD_R1,
	TEXTIM_PAD_R2,
	TEXTIM_PAD_R3,
	TEXTIM_PAD_TRIANGLE,
	TEXTIM_PAD_CROSS,
	TEXTIM_PAD_CIRCLE,
	TEXTIM_PAD_SQUARE,
};

/* One NUL-terminated list of keycodes per pad button. */
/* clang-format off */
static unsigned short g_padKeys[34] = {
	FEP_KEY_F(1), 0,
	FEP_KEY_F(2), 0,
	FEP_KEY_F(3), 0,
	FEP_KEY_UP, 0,
	FEP_KEY_DOWN, 0,
	FEP_KEY_RIGHT, 0,
	FEP_KEY_LEFT, 0,
	FEP_KEY_F(5), 0,
	FEP_KEY_F(8), 0,
	FEP_KEY_F(12), 0,
	FEP_KEY_F(11), 0,
	FEP_KEY_F(10), 0,
	'+', FEP_KEY_SHIFT | '+', 0,
	FEP_KEY_ESCAPE, 0,
	FEP_KEY_ENTER, '\r', 0,
	'-', 0,
};
/* clang-format on */

/* key lists in use, set by myUsbKeyboard_Init */
static unsigned short *myUsbKeyboard_ctrl = g_padKeys;

#define KDGKBTYPE 0x4B33
#define KB_84  1
#define KB_101 2

static __inline__ int is_a_console(int fd)
{
	char arg;

	arg = 0;
	return ioctl(fd, KDGKBTYPE, &arg) == 0 && (arg == KB_101 || arg == KB_84);
}

static __inline__ int open_a_console(const char *fnam)
{
	int fd;

	fd = open(fnam, O_NONBLOCK);
	if (fd < 0 && errno == EACCES)
		fd = open(fnam, O_NONBLOCK | O_WRONLY);
	if (fd < 0)
		return -1;
	if (!is_a_console(fd)) {
		close(fd);
		return -1;
	}
	return fd;
}

/* Opens the console tty, as getfd() of kbd/console-tools: tries /dev/tty,
   /dev/tty0 and /dev/console, keeping the first one KDGKBTYPE accepts as a
   keyboard, and falls back to fds 0..2, in which case *opened = 0 (the fd
   must not be closed by conkbd_shutdown). */
static int conkbd_opentty(int *opened)
{
	int fd;

	*opened = 1;
	fd = open_a_console("/dev/tty");
	if (fd >= 0)
		return fd;
	fd = open_a_console("/dev/tty0");
	if (fd >= 0)
		return fd;
	fd = open_a_console("/dev/console");
	if (fd >= 0)
		return fd;
	*opened = 0;
	for (fd = 0; fd < 3; fd++)
		if (is_a_console(fd))
			return fd;
	return -1;
}

/* Romaji to katakana: with the kana lock on, looks the ASCII key up in
   g_conKanaMap and returns kana - 0xFEC0; otherwise applies six (key,
   modifier) exceptions and passes the key through. */
static int conkbd_key_latin(int key, int up, int mod)
{
	if (!up) {
		if ((g_conLedInit & 4) || g_kanaLock == 0 || key < 33 || key == 127) {
			if (key == 0 && mod == 26)
				return 0xfff7;
			if (key == 27 && mod == 41)
				return 0xfff0;
			if (key == 32) {
				if (mod == 94)
					return 0xfff1;
				if (mod == 92)
					return 0xfff2;
			}
			if (key == 8 && mod == 14)
				return CONKBD_KEY_BACKSPACE;
			if (key == 9 && mod == 15)
				return CONKBD_KEY_TAB;
			if (key == 27 && mod == 1)
				return CONKBD_KEY_ESCAPE;
			return key;
		}

		{
			int i;
			int k;

			for (i = 0; g_conKanaMap[i].key != 0; i++) {
				if (g_conKanaMap[i].key == key) {
					k = g_conKanaMap[i].kana;
					if (k == 0xff66)
						k = (mod != 0x0d) ? k : 0;
					if (k == 0xff9b)
						k = (mod == 0x59) ? k : 0xff70;
					return k ? k - 0xfec0 : 0;
				}
			}
			return 0;
		}
	}
	return 0;
}

/* "Kana" keys -> negative internal codes. */
static int conkbd_key_fn(int key, int up, int mod)
{
	if (up)
		return 0;
	switch (key) {
	case 29:
		return 0;
	case 0:
	case 12:
	case 34:
		return CONKBD_KEY_F(1);
	case 1:
	case 13:
	case 35:
		return (mod != 0x29) ? CONKBD_KEY_F(2) : 0xfff0;
	case 2:
	case 14:
	case 36:
		return CONKBD_KEY_F(3);
	case 3:
	case 15:
	case 37:
		return CONKBD_KEY_F(4);
	case 4:
	case 16:
	case 38:
		return CONKBD_KEY_F(5);
	case 5:
	case 17:
	case 39:
		return CONKBD_KEY_F(6);
	case 6:
	case 18:
	case 40:
		return CONKBD_KEY_F(7);
	case 7:
	case 19:
	case 41:
		return CONKBD_KEY_F(8);
	case 8:
	case 30:
	case 42:
		return (mod != 0x5e) ? CONKBD_KEY_F(9) : 0xfff1;
	case 9:
	case 31:
	case 43:
		return (mod != 0x5c) ? CONKBD_KEY_F(10) : 0xfff2;
	case 10:
	case 32:
	case 44:
		return CONKBD_KEY_F(11);
	case 11:
	case 33:
	case 45:
		return CONKBD_KEY_F(12);
	case 20:
		return CONKBD_KEY_HOME;
	case 21:
		return CONKBD_KEY_INSERT;
	case 22:
		return 8;
	case 23:
		return CONKBD_KEY_END;
	case 24:
		return CONKBD_KEY_PAGE_UP;
	case 25:
		return CONKBD_KEY_PAGE_DOWN;
	}
	return 0;
}

/* Numeric keypad: keys 10..14 are the operators; the others map to the
   cursor or digit table depending on the NumLock LED. */
static int conkbd_key_pad(int key, int up, int mod)
{
	int v;

	if (up)
		return 0;

	switch (key) {
	case 10:
		return 43;
	case 11:
		return 45;
	case 12:
		return 42;
	case 13:
		return CONKBD_KEY_KP_DIVIDE;
	case 14:
		return CONKBD_KEY_ENTER;
	}

	if (ioctl(g_conTtyFd, KDGKBLED, &g_conKbdLed) != 0)
		g_conKbdLed = 0;

	v = (((unsigned char)g_conKbdLed >> 1) ^ 1) & 1;
	if (g_conLedInit & 1) {
		v ^= 1;
		if (v)
			goto cursors;
		goto digits;
	}
	if (!v)
		goto digits;

cursors:
	{
		switch (key) {
		case 0:
			return CONKBD_KEY_INSERT;
		case 1:
			return CONKBD_KEY_END;
		case 2:
			return CONKBD_KEY_DOWN;
		case 3:
			return CONKBD_KEY_PAGE_DOWN;
		case 4:
			return CONKBD_KEY_LEFT;
		case 6:
			return CONKBD_KEY_RIGHT;
		case 7:
			return CONKBD_KEY_HOME;
		case 8:
			return CONKBD_KEY_UP;
		case 9:
			return CONKBD_KEY_PAGE_UP;
		case 16:
			return 127;
		}
		return 0;
	}
digits:
	{
		switch (key) {
		case 0:
			return 48;
		case 1:
			return 49;
		case 2:
			return 50;
		case 3:
			return 51;
		case 4:
			return 52;
		case 5:
			return 53;
		case 6:
			return 54;
		case 7:
			return 55;
		case 8:
			return 56;
		case 9:
			return 57;
		case 16:
			return CONKBD_KEY_KP_PERIOD;
		}
	}
	return 0;
}

static __inline__ int conkbd_getent(unsigned char table, int index)
{
	struct kbentry ke;

	ke.kb_index = index;
	ke.kb_table = table;
	if (ioctl(g_conTtyFd, KDGKBENT, &ke) != 0)
		return -1;
	return ke.kb_value;
}

/* Translates one tty scancode (bit 7 = release) through the kernel keymap
   chosen by the modifiers in g_conLedInit; the keysym type picks the
   handler in g_conKeyHandlers. Key 126 toggles the kana lock. *state gets
   the scancode pressed (0 on release), *mod the modifiers. */
static int conkbd_translate(int code, unsigned short *state, short *mod)
{
	unsigned short v;
	unsigned short ret;
	int key;
	unsigned int type;
	int val;

	v = conkbd_getent(g_indexToKey[g_conLedInit], code & 0x7f);
	if (!(code >> 7))
		*state = code & 0x7f;
	else
		*state = 0;
	key = code & 0x7f;
	if (key == 126) {
		*mod = 0;
		if (*state != 0 && !g_kanaLockFix)
			g_kanaLock = !g_kanaLock;
		return 0;
	}
	type = v >> 8;
	val = v & 0xff;
	if (type == 11) {
		type = 0;
		if (ioctl(g_conTtyFd, KDGKBLED, &g_conKbdLed) != 0)
			g_conKbdLed = 0;
		if ((g_conKbdLed & 4) && !g_kanaLock)
			val = conkbd_getent(g_indexToKey[g_conLedInit ^ 1], key) & 0xff;
	}
	if (type < 11)
		ret = g_conKeyHandlers[type](val, code >> 7, key);
	else
		ret = 0;
	*mod = g_conLedInit;
	return ret;
}

static __inline__ int conkbd_readent(struct kbentry *ke, int table, int index)
{
	ke->kb_index = index;
	ke->kb_table = table;
	return ioctl(g_conTtyFd, KDGKBENT, ke);
}

/* Rebuilds the scancode -> key index table from the kernel keymap (KDGKBENT),
   after binding scancode 112 to keycode 126. An ioctl error other than
   EINVAL is fatal. */
void conkbd_mapupdate(void)
{
	struct kbkeycode kc;
	struct kbentry ke;
	int i;

	kc.scancode = 112;
	kc.keycode = 126;
	ioctl(g_conTtyFd, KDSETKEYCODE, &kc);

	g_keySpecCount = 0;
	g_keyCount = 0;

	for (i = 0; i < 256; i++) {
		/* NOTE: bug: reads entry 0 of keymap i, not entry i of the plain keymap */
		if (conkbd_readent(&ke, i, 0) != 0) {
			if (errno != EINVAL)
				exit(1);
		} else if (ke.kb_value != CONKBD_NOSUCHMAP) {
			g_keyToIndex[i] = g_keyCount;
			g_indexToKey[g_keyCount] = i;
			g_keyCount++;
			if (ke.kb_value == CONKBD_SPEC126)
				g_keySpecCount++;
			continue;
		}
		g_keyToIndex[i] = -1;
	}
}

int conkbd_init(void)
{
	struct termios t;
	int opened;
	int err;

	if (g_conTtyFd >= 0)
		return 1;

	g_conTtyFd = conkbd_opentty(&opened);
	if (g_conTtyFd < 0)
		return 0;

	tcdrain(g_conTtyFd);
	tcflush(g_conTtyFd, 2);
	g_conOpenFlag = opened;
	conkbd_mapupdate();

	err = -1;
	if (ioctl(g_conTtyFd, KDGKBMODE, &g_conOrigKbdMode) != 0)
		return 0;
	if (ioctl(g_conTtyFd, KDSKBMODE, K_MEDIUMRAW) != 0)
		return 0;
	if (tcgetattr(g_conTtyFd, &g_conOrigTermios) == err)
		return 0;

	t = g_conOrigTermios;
	t.c_iflag = 0;
	t.c_cc[VMIN] = 16;
	t.c_cc[VTIME] = 0;
	t.c_lflag &= ~(ISIG | ICANON | ECHO);
	if (tcsetattr(g_conTtyFd, TCSANOW, &t) == -1)
		return 0;

	g_conLedInit = 0;
	g_kanaLockFix = 0;
	g_kanaLock = 0;
	if (ioctl(g_conTtyFd, KDGKBLED, &g_conKbdLed) != 0)
		g_conKbdLed = 0;
	g_conKbdLed &= 0xfe;
	if (g_conKanaPending != 0) {
		g_conKanaPending = 0;
		g_conKbdLed |= 2;
	}
	ioctl(g_conTtyFd, KDSKBLED, g_conKbdLed);
	tcdrain(g_conTtyFd);
	tcflush(g_conTtyFd, 2);
	return 1;
}

/* Reads up to 16 scancodes from the tty and translates them into the
 * three parallel outputs (code, decoder state, modifiers); returns how many
 * keys were produced. A code 0 with state 0 is a scancode consumed without
 * output. */
static __inline__ int conkbd_readkeys(unsigned short *out, unsigned short *state, unsigned short *mod)
{
	int n;
	int i;
	int cnt;

	cnt = 0;
	n = read(g_conTtyFd, g_conKeyBuf, 16);
	for (i = 0; i < n; i++) {
		out[cnt] = conkbd_translate(g_conKeyBuf[i], &state[cnt], &mod[cnt]);
		if (out[cnt] == 0 && state[cnt] == 0)
			continue;
		cnt++;
	}
	return cnt;
}

/* Polls the keyboard: while the IME is shown and the USB keyboard is
   enabled, a key closes the software keyboard if it is open, otherwise it
   goes to the front-end processor. */
int conkbd_exec(void)
{
	unsigned short out[16];
	unsigned short state[16];
	unsigned short mod[16];
	int cnt;
	int i;

	if (textim_SetUsbkbdStatus(-1) == 0)
		tcflush(g_conTtyFd, 2);
	tcdrain(g_conTtyFd);

	cnt = conkbd_readkeys(out, state, mod);
	for (i = 0; i < cnt; i++) {
		if (textim_SkbCheckShown() != 0 && textim_SetUsbkbdStatus(-1) != 0) {
			if (skbmain_checkshown() != 0) {
				skbmain_close();
				fepbar_select_mode(fepbar_get_external_select_mode());
			} else {
				fepmain_sendkey_from_kbd(out[i], mod[i]);
			}
		}
	}

	return 1;
}

static int myUsbKeyboard__SetPad(int key);

static int myUsbKeyboard__SendKey(int code, int mods);

/* Same read and translate loop as conkbd_exec, but every key goes to
   myUsbKeyboard__SendKey. */
static int myUsbKeyboard__ReadPad(void)
{
	unsigned short out[16];
	unsigned short state[16];
	unsigned short mod[16];
	int cnt;
	int i;

	cnt = conkbd_readkeys(out, state, mod);
	for (i = 0; i < cnt; i++)
		myUsbKeyboard__SendKey(out[i], mod[i]);

	return 1;
}

/* Same filter, modifier mask and special key remapping as
   fepmain_sendkey_from_kbd, but the code goes to the pseudo pad. */
static int myUsbKeyboard__SendKey(int code, int mods)
{
	unsigned int i;
	int mask;
	int ok;

	if (code == 0)
		return 1;

	mask = (mods & 1) << 10;
	if (mods & 4)
		mask |= FEP_KEY_CTRL;
	if (mods & 8)
		mask |= FEP_KEY_ALT;

	if (enableChar != 0) {
		ok = 0;
		if ((unsigned int)code < 32 || code == 127 || (unsigned int)code > 0xffe0) {
			ok = 1;
		} else {
			for (i = 0; i < enableChar->count; i++) {
				if (enableChar->seq[i][0] == code) {
					if (enableChar->seq[i][1] == 0)
						ok = 1;
				}
			}
		}
		if (!ok)
			return 1;
	}

	switch (code) {
	case CONKBD_KEY_ENTER:
		code = FEP_KEY_ENTER;
		break;
	case CONKBD_KEY_BACKSPACE:
		code = FEP_KEY_BACKSPACE;
		break;
	case CONKBD_KEY_TAB:
		code = FEP_KEY_TAB;
		break;
	case CONKBD_KEY_ESCAPE:
		code = FEP_KEY_ESCAPE;
		break;
	case 127:
		code = FEP_KEY_DELETE;
		break;
	case CONKBD_KEY_F(1):
		code = FEP_KEY_F(1);
		break;
	case CONKBD_KEY_F(2):
		code = FEP_KEY_F(2);
		break;
	case CONKBD_KEY_F(3):
		code = FEP_KEY_F(3);
		break;
	case CONKBD_KEY_F(4):
		code = FEP_KEY_F(4);
		break;
	case CONKBD_KEY_F(5):
		code = FEP_KEY_F(5);
		break;
	case CONKBD_KEY_F(6):
		code = FEP_KEY_F(6);
		break;
	case CONKBD_KEY_F(7):
		code = FEP_KEY_F(7);
		break;
	case CONKBD_KEY_F(8):
		code = FEP_KEY_F(8);
		break;
	case CONKBD_KEY_F(9):
		code = FEP_KEY_F(9);
		break;
	case CONKBD_KEY_F(10):
		code = FEP_KEY_F(10);
		break;
	case CONKBD_KEY_F(11):
		code = FEP_KEY_F(11);
		break;
	case CONKBD_KEY_F(12):
		code = FEP_KEY_F(12);
		break;
	case CONKBD_KEY_HOME:
		code = FEP_KEY_HOME;
		break;
	case CONKBD_KEY_END:
		code = 335;
		break;
	case CONKBD_KEY_INSERT:
		code = FEP_KEY_INSERT;
		break;
	case 0xfff2:
		code = 352;
		break;
	case 0xfff1:
		code = 363;
		break;
	case CONKBD_KEY_PAGE_UP:
		code = FEP_KEY_PAGE_UP;
		break;
	case CONKBD_KEY_PAGE_DOWN:
		code = FEP_KEY_PAGE_DOWN;
		break;
	case CONKBD_KEY_LEFT:
		code = FEP_KEY_LEFT;
		break;
	case CONKBD_KEY_RIGHT:
		code = FEP_KEY_RIGHT;
		break;
	case CONKBD_KEY_UP:
		code = FEP_KEY_UP;
		break;
	case CONKBD_KEY_DOWN:
		code = FEP_KEY_DOWN;
		break;
	default:
		if (code > 0xffcf)
			return 1;
		break;
	case 28:
		return 1;
	case 32:
		break;
	}

	return myUsbKeyboard__SetPad(code | mask);
}

int conkbd_setkbdrate(int rate, int delay)
{
	struct kbd_repeat rep;
	int period, del;

	if (!(0 < rate))
		rate = 30;
	period = rate;
	if (!(0 < delay))
		delay = 250;
	del = delay;

	rep.rate = -1;
	rep.delay = -1;
	if (ioctl(g_conTtyFd, KDKBDREP, &rep) != 0)
		return 0;
	rep.rate = period;
	rep.delay = del;
	return ioctl(g_conTtyFd, KDKBDREP, &rep) == 0;
}

int conkbd_setkanalock(int lock)
{
	if (lock >= 0)
		g_kanaLock = (lock != 0);
	return g_kanaLock;
}

int conkbd_setkanalock_fix(int fix)
{
	if (fix >= 0)
		g_kanaLockFix = (fix != 0);
	return g_kanaLockFix;
}

void conkbd_shutdown(void)
{
	if (g_conTtyFd < 0)
		return;
	tcdrain(g_conTtyFd);
	ioctl(g_conTtyFd, KDSKBMODE, g_conOrigKbdMode);
	tcsetattr(g_conTtyFd, TCSADRAIN, &g_conOrigTermios);
	if (g_conOpenFlag) {
		close(g_conTtyFd);
	}
	g_conTtyFd = -1;
	g_conOpenFlag = 0;
}

/* Reads up to 16 scancodes from the tty and translates them. The decoder
   is stateful and may consume a scancode without output (incomplete escape
   sequence): code 0 with state 0. */
int conkbd_getkey(unsigned short *out, unsigned short *state, short *mod, unsigned int max)
{
	int n;
	int i;
	int cnt;

	if (max > 16)
		max = 16;
	n = read(g_conTtyFd, g_conKeyBuf, max);
	cnt = 0;
	for (i = 0; i < n; i++) {
		out[cnt] = conkbd_translate(g_conKeyBuf[i], &state[cnt], &mod[cnt]);
		if (out[cnt] == 0 && state[cnt] == 0)
			continue;
		cnt++;
	}
	return cnt;
}

/* Lock keys: cases 7 and 8 toggle the CapsLock and NumLock LEDs,
   rereading their state from the kernel. */
static int conkbd_key_led(int key, int up, int mod)
{
	if (up)
		return 0;
	switch (key) {
	case 1:
		return CONKBD_KEY_ENTER;
	case 0:
		if (mod == 41)
			return 0xfff0;
		break;
	case 7:
		if (ioctl(g_conTtyFd, KDGKBLED, &g_conKbdLed) != 0)
			g_conKbdLed = 0;
		g_conKbdLed ^= 4;
		ioctl(g_conTtyFd, KDSKBLED, g_conKbdLed);
		break;
	case 8:
		if (ioctl(g_conTtyFd, KDGKBLED, &g_conKbdLed) != 0)
			g_conKbdLed = 0;
		g_conKbdLed ^= 2;
		ioctl(g_conTtyFd, KDSKBLED, g_conKbdLed);
		break;
	case 12:  /* redundant as written */
		return 0;
	}
	return 0;
}

/* Handlers that ignore the key. */
static int conkbd_key_dead(int key, int up, int mod)
{
	return 0;
}

static int conkbd_key_cons(int key, int up, int mod)
{
	return 0;
}

/* The four cursor keys (KT_CUR). */
static int conkbd_key_fnrow(int key, int up, int mod)
{
	if (up)
		return 0;
	switch (key) {
	case 0:
		return CONKBD_KEY_DOWN;
	case 1:
		return CONKBD_KEY_LEFT;
	case 2:
		return CONKBD_KEY_RIGHT;
	case 3:
		return CONKBD_KEY_UP;
	}
	return 0;
}

/* Sets or clears bit `key` of g_conLedInit. */
static int conkbd_key_setbit(int key, int up, int mod)
{
	if (up)
		g_conLedInit &= ~(1 << key);
	else
		g_conLedInit |= (1 << key);
	return 0;
}

/* Space bar (96) with modifier 41. */
static int conkbd_key_space(int key, int up, int mod)
{
	if (up)
		return 0;
	if (key == 96 && mod == 41)
		return 0xfff0;
	return 0;
}

/* Handlers that ignore the key. */
static int conkbd_key_ascii(int key, int up, int mod)
{
	return 0;
}

static int conkbd_key_lock(int key, int up, int mod)
{
	return 0;
}

static int conkbd_key_letter(int key, int up, int mod)
{
	return 0;
}

/* Rebuilds the pseudo pad state from the keyboard, under textim_recv_mutex. */
void myUsbKeyboard_Run(void)
{
	pthread_mutex_lock(&textim_recv_mutex);
	myUsbKeyboard_LastPseudoPadData = 0;
	myUsbKeyboard__ReadPad();
	pthread_mutex_unlock(&textim_recv_mutex);
}

void myUsbKeyboard_Clear(void)
{
	myUsbKeyboard_LastPseudoPadData = 0;
}

void myUsbKeyboard_Init(void *ctrl)
{
	myUsbKeyboard_ctrl = (unsigned short *)ctrl;
}

void myUsbKeyboard_Terminate(void)
{}

/* Sets the pad bits of the buttons whose keycode list in
   *myUsbKeyboard_ctrl contains key. */
static int myUsbKeyboard__SetPad(int key)
{
	unsigned int i;
	unsigned int n;
	int j;
	unsigned short *p;
	unsigned short *b;
	unsigned short c;
	int m;

	if (!key)
		return 1;

	for (i = 0, j = 0, p = myUsbKeyboard_ctrl; i < 16; i = n) {
		n = i + 1;
		b = g_padBit + i;
		m = myUsbKeyboard_LastPseudoPadData;
		while (p[j] != 0) {
			c = p[j];
			j++;
			if (c == key) {
				m |= *b;
				break;
			}
		}
		myUsbKeyboard_LastPseudoPadData = m;
		j++;
	}
	return 1;
}
