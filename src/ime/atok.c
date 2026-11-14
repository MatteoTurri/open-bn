/*
 * ATOK engine client: loads the two engine modules (atokcodeconv.so and
 * atok12engine.so) with dlopen, configures a session and exchanges
 * messages with it for the front-end processor.
 */

#include <pwd.h>
#include <unistd.h>
#include <dlfcn.h>
#include "textim_debug.h"
#include "atok.h"
#include "fepbar.h"
#include "feplist.h"
#include "fepmain.h"
#include <string.h>

typedef void *AtokHandle;

static AtokHandle g_atokHandle = 0;
static int g_atokInited = 0;          /* atok_connect succeeded */
int atok_havepreedit, atok_preeditcurpos, atok_halfchar_flag;
static int g_atokModeActive;    /* kana mode already active */

/* Header of the candidate request, at +0x100 of the message buffer. */
typedef struct {
	/* 0x00 */ unsigned short type;
	unsigned char pad1[4];
	/* 0x06 */ unsigned short count;
	unsigned char pad2[28];
	/* 0x24 */ short *textPtr;
	/* 0x28 */ unsigned char *lenSrcPtr;
	/* 0x2C */ void *extraPtr;
} AtokMsgHdr;

/* Current candidate state, at +0x44 of the message buffer. */
typedef struct {
	unsigned short preeditLen;
	unsigned char pad_02[4];
	unsigned short cursorPos;
	unsigned char pad_08[16];
	unsigned char inputMode;
	unsigned char pad_19[1];
	unsigned short modeFlags;
} AtokCandState;

static union {
	unsigned char raw[352];
	struct {                       /* simple messages (atok_sendmsg) */
		unsigned short pad0;
		/* 0x02 */ unsigned short code;
		unsigned char pad4[14];
		/* 0x12 */ unsigned short arg;
	} key;
	struct {
		unsigned char pad0[68];
		/* 0x44 */ AtokCandState state;
		unsigned char pad1[256 - 68 - sizeof(AtokCandState)];
		/* 0x100 */ AtokMsgHdr header;
	} cand;
	struct {
		char pad[256];
		short mode;
	} kana;
} g_atokMsg;

#define CAND_MAX 300
static unsigned char g_candLenSrc[CAND_MAX];
static unsigned char g_candExtra[CAND_MAX];
static short g_candTextBuf[1450];
static int g_candLenOut[CAND_MAX];
static short *g_candPtrOut[CAND_MAX];

const char szAtokEnginePath[] = "/usr/lib/locale/ja/atokserver/";
const char szAtokUsersBasePath[] = "/var/locale/ja/atokserver/users/";

void *atok_handle = 0;
void *conv_handle = 0;
int (*pAtokEngineAttach)(void *, void *);
int (*pAtokEngineDettach)(void);
AtokHandle (*pAtokCreateSession)(void *);
void (*pAtokDestroySession)(AtokHandle);
int (*pAtokCommunicate)(int code, AtokHandle h, void *msg);
void *psjis2ucs;
void *pucs2sjis;
void *peuc2ucs;
void *pucs2euc;

/* Loads the code converter and engine modules and resolves their
 * symbols. */
static int atok_connect(void)
{
	char path[4416];

	strcpy(path, szAtokEnginePath);
	strcat(path, "atokcodeconv.so");
	conv_handle = dlopen(path, RTLD_LAZY);
	if (conv_handle == 0)
		return 0;
	psjis2ucs = dlsym(conv_handle, "sjis2ucs");
	pucs2sjis = dlsym(conv_handle, "ucs2sjis");
	peuc2ucs = dlsym(conv_handle, "euc2ucs");
	pucs2euc = dlsym(conv_handle, "ucs2euc");
	if (psjis2ucs == 0 || pucs2sjis == 0 || peuc2ucs == 0 || pucs2euc == 0) {
		dlclose(conv_handle);
		conv_handle = 0;
		return 0;
	}

	strcpy(path, szAtokEnginePath);
	strcat(path, "atok12engine.so");
	atok_handle = dlopen(path, RTLD_LAZY);
	if (atok_handle == 0) {
		dlclose(conv_handle);
		conv_handle = 0;
		return 0;
	}
	pAtokEngineAttach = dlsym(atok_handle, "AtokEngineAttach");
	pAtokEngineDettach = dlsym(atok_handle, "AtokEngineDettach");
	pAtokCreateSession = dlsym(atok_handle, "AtokCreateSession");
	pAtokDestroySession = dlsym(atok_handle, "AtokDestroySession");
	pAtokCommunicate = dlsym(atok_handle, "AtokCommunicate");
	if (pAtokEngineAttach == 0 || pAtokEngineDettach == 0 || pAtokCreateSession == 0 || pAtokDestroySession == 0
	    || pAtokCommunicate == 0) {
		dlclose(atok_handle);
		atok_handle = 0;
		dlclose(conv_handle);
		conv_handle = 0;
		return 0;
	}
	return pAtokEngineAttach(psjis2ucs, pucs2sjis);
}

/* Reads at most maxlen characters from the cursor *pp and parses them as
 * hex. Returns 0xFFFF if the cursor is null or at the end of the string. */
static int unicode_readhex(unsigned char **pp, int maxlen)
{
	unsigned char *p;
	int i;
	int val;

	p = *pp;
	if (p == 0 || *p == 0)
		return 0xffff;
	{
		char buf[maxlen + 1];

		i = 0;
		while (i < maxlen && *p != 0)
			buf[i++] = *p++;
		buf[i] = 0;
		sscanf(buf, "%x", &val);
	}
	*pp = p;
	return val;
}

/* Parses 16 hex pairs into keyIndex, then up to 800 groups of five into keyData,
 * with 4-digit fields; s is advanced as a cursor. */
static int unicode_parsetable(unsigned char *s, unsigned short *keyIndex, unsigned short *keyData)
{
	unsigned int i;
	unsigned short *d;

	if (s == 0)
		return 0;

	i = 0;
	d = keyIndex;
	do {
		if (*s == 0)
			return 0;
		d[0] = unicode_readhex(&s, 4);
		d[1] = unicode_readhex(&s, 4);
		d += 2;
		i++;
	} while (i < 16);

	i = 0;
	d = keyData;
	do {
		if (*s == 0)
			break;
		d[0] = unicode_readhex(&s, 4);
		d[1] = unicode_readhex(&s, 4);
		d[2] = unicode_readhex(&s, 4);
		d[3] = unicode_readhex(&s, 4);
		d[4] = unicode_readhex(&s, 4);
		d += 5;
		i++;
	} while (i < 800);

	return i;
}

/* Romaji table: count entries of ent, followed by tail shorts as many as
 * the three fields of the last entry add up to. */
struct UniBlob {
	int count;
	int unk_04;
	int w[96];
	struct {
		unsigned short code;
		unsigned char b1;
		unsigned char b2;
	} ent[550];
	unsigned short tail[3300];
};

static int unicode_parseblob(unsigned char *s, struct UniBlob *t)
{
	unsigned int i;
	unsigned int n;

	if (s == 0 || t == 0)
		return 0;

	t->count = unicode_readhex(&s, 8);
	t->unk_04 = unicode_readhex(&s, 8);

	i = 0;
	do {
		t->w[i] = unicode_readhex(&s, 8);
		i++;
	} while (i < 96);

	i = 0;
	n = t->count;
	if (n != 0) {
		do {
			t->ent[i].code = unicode_readhex(&s, 4);
			t->ent[i].b1 = unicode_readhex(&s, 2);
			t->ent[i].b2 = unicode_readhex(&s, 2);
			i++;
		} while (i < n);
	}

	n = t->ent[i - 1].code + t->ent[i - 1].b1 + t->ent[i - 1].b2;
	if (n != 0) {
		i = 0;
		do {
			t->tail[i] = unicode_readhex(&s, 4);
			i++;
		} while (i < n);
	}

	return t->count;
}

extern unsigned char ps2ime_key[];
extern unsigned char ps2ime_roma[];
extern unsigned char atok12n_key[];
extern unsigned char atok12n_roma[];

/* Engine configuration given to pAtokCreateSession. The engine is a separate
 * library, so most fields are known only by the defaults atok_bind writes. */
#define AOP12_OPT_PS2IME_TABLES 38 /* unk_40 entry: 1 with the ps2ime key and romaji tables */

struct AOP12Conf {
	short unk_00;
	short pad_02[3];
	int unk_08;
	unsigned short *userName;
	int unk_10;
	int pad_14[3];
	unsigned char unk_20[20];
	short speed;
	short unk_36;
	unsigned char unk_38[4];
	int pad_3C;
	short unk_40[74];
	char pad_D4[44];
	unsigned short *dicNames[7][16];
	unsigned char unk_2C0[3][16];
	unsigned char pad_2F0[32];
	short unk_310[3];
	char pad_316[26];
	unsigned char dicAttr[7][16]; /* one byte per dictionary slot of dicNames */
	char pad_3A0[96];
};

struct AOP12Conf stAOP12_conf;
unsigned short stAOP12_keyindex[32];
unsigned short stAOP12_keydata[4000];
struct UniBlob stAOP12_romainfo;
char szUserName[4095];
unsigned short wzUserName[4095];
char szAtokDicPath[4095];
char szUsrDicName1[4095];
char szUsrDicName2[4095];
char szUsrDicName3[4095];
char szUsrDicName4[4095];
char szSystemDicName1[4095];
char szSystemDicName2[4095];
char szSystemDicName3[4095];
char szSystemDicName4[4095];
char szSystemDicName5[4095];
unsigned short szDummyName[1] = { 0 };

/* Fills the engine configuration stAOP12_conf: wide user name, user and
 * system dictionary paths, key and romaji tables. mode selects the tables
 * (0 = ps2ime, else atok12n); speed goes to +52, clamped to 30..100. */
static int atok_bind(int mode, int speed)
{
	struct passwd *pw;
	struct passwd pwbuf; /* unused, but part of the layout */
	char buf[1024];  /* unused, but part of the layout */
	int i;

	pw = getpwuid(getuid());
	printf(" <UserName=%s>\n", pw->pw_name);
	strcpy(szUserName, pw->pw_name);
	for (i = 0;; i++) {
		wzUserName[i] = szUserName[i];
		if (szUserName[i] == 0)
			break;
	}
	strcpy(szAtokDicPath, szAtokUsersBasePath);
	strcat(szAtokDicPath, szUserName);
	strcat(szAtokDicPath, "/");

	memset(&stAOP12_conf, 255, 1024);
	stAOP12_conf.unk_08 = 0;
	stAOP12_conf.userName = wzUserName;
	stAOP12_conf.unk_10 = 0;
	stAOP12_conf.unk_00 = 2;
	stAOP12_conf.unk_40[20] = 0;
	stAOP12_conf.unk_20[0] = 0;
	stAOP12_conf.unk_20[1] = 1;
	stAOP12_conf.unk_20[2] = 0;
	stAOP12_conf.unk_20[3] = 15;
	stAOP12_conf.unk_20[4] = 0;
	stAOP12_conf.unk_20[5] = 2;
	stAOP12_conf.unk_20[12] = 1;
	stAOP12_conf.unk_20[14] = 0;
	stAOP12_conf.unk_20[15] = 7;
	stAOP12_conf.unk_20[16] = 7;
	stAOP12_conf.unk_20[18] = 31;
	stAOP12_conf.unk_20[19] = 1;
	stAOP12_conf.unk_36 = 2;
	stAOP12_conf.unk_38[2] = 0;
	stAOP12_conf.unk_38[3] = 7;
	stAOP12_conf.unk_40[0] = 15;
	stAOP12_conf.unk_40[2] = 0;
	stAOP12_conf.unk_40[3] = 0;
	stAOP12_conf.unk_40[4] = 0;
	stAOP12_conf.unk_40[5] = 13;
	stAOP12_conf.unk_40[6] = 0;
	stAOP12_conf.unk_40[7] = 15;
	stAOP12_conf.unk_40[8] = 1;
	stAOP12_conf.unk_40[9] = 0;
	stAOP12_conf.unk_40[10] = 1;
	stAOP12_conf.unk_40[18] = 0;
	stAOP12_conf.unk_40[21] = 0;
	stAOP12_conf.unk_40[34] = 1;
	stAOP12_conf.unk_40[40] = 1;
	stAOP12_conf.unk_40[44] = 1;
	stAOP12_conf.unk_40[45] = 0;
	stAOP12_conf.unk_40[46] = 0;
	stAOP12_conf.unk_40[48] = 0;
	stAOP12_conf.unk_40[49] = 0;
	stAOP12_conf.unk_40[50] = 1;
	stAOP12_conf.unk_40[51] = 1;
	stAOP12_conf.unk_40[56] = 0;
	stAOP12_conf.unk_40[57] = 0;
	stAOP12_conf.unk_40[58] = 0;
	stAOP12_conf.unk_40[59] = 0;
	stAOP12_conf.unk_40[60] = 19;
	stAOP12_conf.unk_40[61] = 0;
	stAOP12_conf.unk_40[64] = 1;
	stAOP12_conf.unk_40[65] = 0;
	stAOP12_conf.unk_40[66] = 1;
	stAOP12_conf.unk_40[67] = 2;
	stAOP12_conf.unk_40[70] = 0;
	stAOP12_conf.unk_40[71] = 0;
	stAOP12_conf.unk_40[72] = 0;
	stAOP12_conf.unk_40[73] = 0;
	if (speed > 0) {
		if (speed < 30)
			speed = 30;
		if (speed > 100)
			speed = 100;
		stAOP12_conf.speed = speed;
	}

	strcpy(szUsrDicName1, szAtokDicPath);
	strcpy(szUsrDicName2, szUsrDicName1);
	strcpy(szUsrDicName3, szUsrDicName1);
	strcpy(szUsrDicName4, szUsrDicName1);
	strcpy(szSystemDicName1, szUsrDicName1);
	strcpy(szSystemDicName2, szUsrDicName1);
	strcpy(szSystemDicName3, szUsrDicName1);
	strcpy(szSystemDicName4, szUsrDicName1);
	strcpy(szSystemDicName5, szUsrDicName1);
	strcat(szUsrDicName1, "atok12u1.dic");
	strcat(szUsrDicName2, "atok12u2.dic");
	strcat(szUsrDicName3, "atok12u3.dic");
	strcat(szUsrDicName4, "atok12u4.dic");
	strcat(szSystemDicName1, "atok12.dic");
	strcat(szSystemDicName2, "atok12tk.dic");
	strcat(szSystemDicName3, "atok12y7.dic");
	strcat(szSystemDicName4, "facemi12.dic");
	strcat(szSystemDicName5, "atok12yc.dic");

	stAOP12_conf.unk_310[1] = 0;
	stAOP12_conf.unk_310[2] = 0;
	stAOP12_conf.unk_310[0] = 0;
	for (i = 0; i < 10; i++) {
		stAOP12_conf.dicNames[0][i] = szDummyName;
		stAOP12_conf.dicNames[1][i] = szDummyName;
		stAOP12_conf.dicNames[2][i] = szDummyName;
		stAOP12_conf.dicNames[3][i] = szDummyName;
		stAOP12_conf.dicNames[4][i] = szDummyName;
		stAOP12_conf.dicNames[5][i] = szDummyName;
		stAOP12_conf.dicNames[6][i] = szDummyName;
		stAOP12_conf.unk_2C0[0][i] = 1;
		stAOP12_conf.unk_2C0[1][i] = 1;
		stAOP12_conf.unk_2C0[2][i] = 2;
		stAOP12_conf.dicAttr[0][i] = 3;
		stAOP12_conf.dicAttr[1][i] = 1;
		stAOP12_conf.dicAttr[2][i] = 1;
		stAOP12_conf.dicAttr[3][i] = 1;
		stAOP12_conf.dicAttr[4][i] = 1;
		stAOP12_conf.dicAttr[5][i] = 1;
		stAOP12_conf.dicAttr[6][i] = 3;
	}
	stAOP12_conf.dicNames[1][0] = (unsigned short *)szUsrDicName1;
	stAOP12_conf.dicNames[1][1] = (unsigned short *)szUsrDicName2;
	stAOP12_conf.dicNames[1][2] = (unsigned short *)szUsrDicName3;
	stAOP12_conf.dicNames[1][3] = (unsigned short *)szUsrDicName4;
	stAOP12_conf.dicNames[2][0] = (unsigned short *)szSystemDicName1;
	stAOP12_conf.dicNames[2][1] = (unsigned short *)szSystemDicName2;
	stAOP12_conf.dicNames[2][2] = (unsigned short *)szSystemDicName3;
	stAOP12_conf.dicNames[2][3] = (unsigned short *)szSystemDicName4;
	stAOP12_conf.dicNames[3][2] = (unsigned short *)szSystemDicName5;

	if (mode == 0) {
		stAOP12_conf.unk_40[AOP12_OPT_PS2IME_TABLES] = 1;
		memset(stAOP12_keydata, 0, 8000);
		unicode_parsetable(ps2ime_key, stAOP12_keyindex, stAOP12_keydata);
		memset(&stAOP12_romainfo, 0, 9192);
		unicode_parseblob(ps2ime_roma, &stAOP12_romainfo);
	} else {
		stAOP12_conf.unk_40[AOP12_OPT_PS2IME_TABLES] = 0;
		memset(stAOP12_keydata, 0, 8000);
		unicode_parsetable(atok12n_key, stAOP12_keyindex, stAOP12_keydata);
		memset(&stAOP12_romainfo, 0, 9192);
		unicode_parseblob(atok12n_roma, &stAOP12_romainfo);
	}
	return 1;
}

/* Init message of pAtokCommunicate(178, ...); the same local is reused for
 * both calls, so fields not rewritten keep the first call's values. */
typedef struct {
	unsigned char pad0[256];
	/* 0x100 */ unsigned short type;
	/* 0x102 */ unsigned short table;
	/* 0x104 */ unsigned short indexCount;
	/* 0x106 */ unsigned short dataCount;
	/* 0x108 */ unsigned char pad1[24];
	/* 0x120 */ void *p[16];
} AtokOpenMsg;

/* Engine event, passed by fepmain to atok_cmmu and atok_setcandlist. */
typedef struct {
	/* 0x00 */ unsigned short type;
	/* 0x02 */ unsigned char pad2[10];
	/* 0x0C */ unsigned short keyCode;  /* subtype when type == 1 */
	/* 0x0E */ unsigned char unk_0E;
	/* 0x0F */ unsigned char pad3[3];
	/* 0x12 */ unsigned short ucs2;
	/* 0x14 */ unsigned short candCount;
	/* 0x16 */ unsigned short candIndex;
	/* 0x18 */ unsigned char pad1[40];
	/* 0x40 */ unsigned short update;  /* 0 = nothing to do */
	/* 0x42 */ unsigned short flags;
	/* 0x44 */ unsigned short preeditLen;
	/* 0x46 */ unsigned char pad4[8];
	/* 0x4E */ unsigned short commitLen;
	/* 0x50 */ unsigned char pad5[48];
	/* 0x80 */ unsigned short *preedit;  /* preedit characters */
	/* 0x84 */ unsigned short *preeditAttr;  /* preedit attributes */
	/* 0x88 */ unsigned char pad6[16];
	/* 0x98 */ unsigned short *commit;  /* committed text */
	/* 0x9C */ unsigned char pad7[132];
	/* 0x120 */ int unk_120;
	/* 0x124 */ int unk_124;
} AtokEvent;


/* Fetches the candidate list from the engine into the g_cand* tables and
 * hands it to feplist, or closes or moves the list. */
__inline__ void atok_setcandlist(AtokHandle h, AtokEvent *info)
{
	unsigned short flags = info->flags;

	if (flags & 0x5000) {
		AtokMsgHdr *hdr = &g_atokMsg.cand.header;
		int i, off;

		feplist_open();

		hdr->type = 0xffff;
		hdr->textPtr = g_candTextBuf;
		hdr->lenSrcPtr = g_candLenSrc;
		hdr->extraPtr = g_candExtra;
		pAtokCommunicate(822, h, g_atokMsg.raw);

		off = 0;
		for (i = 0; i < g_atokMsg.cand.header.count; i++) {
			g_candLenOut[i] = g_candLenSrc[i];
			g_candPtrOut[i] = g_candTextBuf + off;
			off += g_candLenSrc[i];
		}
		feplist_setstring(info->candIndex, info->candCount, g_candLenOut, g_candPtrOut);
	} else if (flags & 0x2000) {
		feplist_close();
	} else {
		feplist_setcurrent(info->candIndex, info->candCount);
	}
}

int atok_cmmu(AtokHandle h, int code, AtokEvent *msg)
{
	unsigned int type;
	unsigned short flags;

	if (!pAtokCommunicate(code & 0xffff, h, msg))
		return 0;

	type = msg->type;
	switch (type) {
	case 0x8540:
		break;
	case 0x100:
	case 0x9000:
	case 0xa000:
	case 0xb000:  /* redundant as written */
		break;
	case 17:
		msg->unk_120 = 0;
		msg->unk_124 = 0;
		pAtokCommunicate(294, h, msg);
		break;
	case 1:
		if (msg->keyCode != 32763)
			fepmain_recvkey(msg->keyCode);
		else
			fepmain_recvucs2(msg->ucs2);
		break;
	}

	if (msg->update == 0)
		return 0;

	if (msg->flags & 0x10)
		fepmain_setcommit(msg->commitLen, msg->commit);
	if (msg->flags & 1)
		fepmain_setpreedit(msg->preeditLen, msg->preedit, msg->preeditAttr);

	flags = msg->flags;
	if ((flags & 0x7000) || msg->unk_0E == 128 || msg->unk_0E == 144)
		atok_setcandlist(h, msg);

	atok_havepreedit = g_atokMsg.cand.state.preeditLen != 0;
	atok_preeditcurpos = g_atokMsg.cand.state.cursorPos;

	if (msg->update == 0)
		return 0;

	switch (g_atokMsg.cand.state.inputMode) {
	case 0:
		atok_halfchar_flag = 1;
		fepbar_disp_mode(-1);
		return 1;
	case 2:
		atok_halfchar_flag = 1;
		fepbar_disp_mode(-1);
		return 1;
	case 1:
		atok_halfchar_flag = 0;
		if (g_atokMsg.cand.state.modeFlags & 1) {
			if (g_atokMsg.cand.state.modeFlags & 0x10)
				fepbar_disp_mode(5);
			else
				fepbar_disp_mode(0);
		}

		if (g_atokMsg.cand.state.modeFlags & 2) {
			if (g_atokMsg.cand.state.modeFlags & 8) {
				if (g_atokMsg.cand.state.modeFlags & 0x10)
					fepbar_disp_mode(7);
				else
					fepbar_disp_mode(2);
			} else {
				if (g_atokMsg.cand.state.modeFlags & 0x10)
					fepbar_disp_mode(6);
				else
					fepbar_disp_mode(1);
			}
		}

		if (g_atokMsg.cand.state.modeFlags & 4) {
			if (g_atokMsg.cand.state.modeFlags & 8) {
				if (g_atokMsg.cand.state.modeFlags & 0x10)
					fepbar_disp_mode(9);
				else
					fepbar_disp_mode(4);
			} else {
				if (g_atokMsg.cand.state.modeFlags & 0x10)
					fepbar_disp_mode(8);
				else
					fepbar_disp_mode(3);
			}
		}
		break;
	default:
		atok_halfchar_flag = 1;
		fepbar_disp_mode(-1);
		break;
	}
	return 1;
}

int atok_sendmsg(int type, int code)
{
	unsigned short c = (unsigned short)code;
	AtokHandle h = g_atokHandle;

	if (!h)
		return 0;

	switch (type) {
	case 0:
		g_atokMsg.key.code = c;
		g_atokMsg.key.arg = 0;
		atok_cmmu(h, 352, g_atokMsg.raw);
		break;
	case 1:
		g_atokMsg.key.code = 32763;
		g_atokMsg.key.arg = c;
		atok_cmmu(h, 352, g_atokMsg.raw);
		break;
	case 2:
		atok_sendmsg(0, 0xffff);
		while (atok_havepreedit) {
			g_atokMsg.key.code = FEP_KEY_ESCAPE;
			g_atokMsg.key.arg = 0;
			atok_cmmu(h, 352, g_atokMsg.raw);
		}
		atok_sendmsg(0, 0xffff);
		break;
	case 3:
		atok_sendmsg(0, 0xffff);
		g_atokMsg.cand.header.type = c;
		atok_cmmu(h, 822, g_atokMsg.raw);
		g_atokMsg.key.code = 795;
		g_atokMsg.key.arg = 0;
		atok_cmmu(h, 352, g_atokMsg.raw);
		atok_sendmsg(0, 0xffff);
		break;
	default:
		break;
	}
	return 1;
}

int atok_set_inputmode(int mode, int sub)
{
	AtokHandle h = g_atokHandle;
	int errFlag;

	if (!h)
		return 0;

	atok_sendmsg(0, 0xffff);

	if (!g_atokModeActive) {
		if (mode > 0) {
			atok_cmmu(h, 1, g_atokMsg.raw);
			g_atokModeActive = 1;
		}
	} else {
		if (mode == 0 || mode == 2) {
			atok_cmmu(h, 2, g_atokMsg.raw);
			g_atokModeActive = 0;
		}
	}

	atok_sendmsg(0, 0xffff);

	if (sub < 0) {
		int inactive = !g_atokModeActive;
		atok_halfchar_flag = inactive;
		if (inactive)
			fepbar_disp_mode(-1);
		return -1;
	}

	if (sub == 0) {
		g_atokMsg.cand.header.type = 2;
		atok_cmmu(h, 37, g_atokMsg.raw);
	} else {
		g_atokMsg.cand.header.type = (unsigned short)sub;
		atok_cmmu(h, 47, g_atokMsg.raw);
		g_atokMsg.cand.header.type = 1;
		atok_cmmu(h, 37, g_atokMsg.raw);
	}

	atok_sendmsg(0, 0xffff);

	errFlag = 0;
	if (sub == 0 || g_atokModeActive == 0)
		errFlag = 1;
	atok_halfchar_flag = errFlag;
	if (errFlag)
		fepbar_disp_mode(-1);
	return 1;
}

__inline__ int atok_set_kanamode(int mode)
{
	AtokHandle h = g_atokHandle;
	if (!h)
		return 0;
	atok_sendmsg(0, 0xffff);
	g_atokMsg.kana.mode = (short)mode;
	atok_cmmu(h, 28, &g_atokMsg.kana);
	atok_sendmsg(0, 0xffff);
	return 1;
}

/* Creates the engine session with the atok_bind configuration and sends it
 * the key tables and the romaji table. */
int atok_open(void)
{
	AtokOpenMsg msg;
	void *handle;
	void *h;
	void *hh;
	unsigned short localbuf[2];

	localbuf[0] = 0;
	handle = pAtokCreateSession(&stAOP12_conf);
	if (!handle) {
		h = 0;
	} else {

		msg.indexCount = 16;
		msg.dataCount = 800;
		msg.type = 0;
		msg.table = 0;
		msg.p[0] = stAOP12_keyindex;
		msg.p[1] = stAOP12_keydata;
		pAtokCommunicate(178, handle, &msg);

		msg.type = 0;
		msg.table = 2;
		msg.p[0] = &stAOP12_romainfo.count;
		msg.p[1] = &stAOP12_romainfo.unk_04;
		msg.p[2] = stAOP12_romainfo.w;
		msg.p[3] = stAOP12_romainfo.ent;
		msg.p[4] = stAOP12_romainfo.tail;
		pAtokCommunicate(178, handle, &msg);
		h = handle;
	}
	g_atokHandle = h;
	if (g_atokHandle) {
		atok_preeditcurpos = 0;
		atok_havepreedit = 0;
		fepmain_setpreedit(0, localbuf, localbuf);
		atok_set_inputmode(1, 1);
		atok_set_kanamode(0);
		return 1;
	}
	return 0;
}

__inline__ int atok_init(int mode, int speed)
{
	atok_havepreedit = 0;
	atok_preeditcurpos = 0;
	atok_halfchar_flag = 0;
	g_atokHandle = 0;
	{
		int inited = g_atokInited;
		g_atokModeActive = 0;
		if (inited)
			goto success;
	}
	if (atok_connect()) {
		g_atokInited = 1;
		atok_bind(mode, speed);
		goto success;
	}
	return 0;
success:
	return 1;
}

__inline__ void atok_close(void)
{
	if (g_atokHandle) {
		pAtokDestroySession(g_atokHandle);
		g_atokHandle = 0;
	}
}

__inline__ void atok_shutdown(void)
{
	atok_close();
	if (g_atokInited) {
		if (conv_handle) {
			dlclose(conv_handle);
			conv_handle = 0;
		}
		if (atok_handle) {
			pAtokEngineDettach();
			dlclose(atok_handle);
			atok_handle = 0;
		}
		g_atokInited = 0;
	}
}
