/*
 * Small/amx natives "snd*"/"sound*": audio of the PSBBN application.
 * Opens /dev/dsp2, boots and shuts down the sound driver, and loads BGM/SE
 * banks (local files or URLs) in background thJob threads.
 */

#include "amx.h"
#include "natives/smallext.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/soundcard.h>
#include <string>
#include "xml/xml_filedescriptor.h"
#include "thread/thposixthread.h"
#include "audio/sound.h"
#include "audio/scesd.h"
#include "app/systemparam.h"
#include "script/script.h"
#include "script/script_abort_entry.h"
#include "sndextension.h"
#include "thread/thjob.h"

/* Driver setup/shutdown helpers of bnDspInit and bnDspExit. */
static void dspSetupPre();
static void dspSetupPost();
static void dspShutdownPre();

/* User data tags: 'PSCR' is the Script registered on its amx, '_SNL' the audio loading state. */
static const long TAG_RCSP = 0x52435350;
static const unsigned int TAG_LNS = 0x4C4E535F;

/*
 * Loading thread (thJob): m_str[0..2] are the resource URLs (SE uses two, BGM three),
 * m_int[SND_JOB_RESULT] the result (0 = ok), m_int[SND_JOB_DECODE] one bit per
 * resource that must go through the decoder.
 */
enum {
	SND_JOB_RESULT = 0,
	SND_JOB_DECODE = 1
};

/* Audio loading state the Script keeps under the '_SNL' tag: BGM and SE threads and the result. */
class SoundLoadData {
public:
	/* 0x00 */ thJob *bgmThread;
	/* 0x04 */ thJob *seThread;
	/* 0x08 */ int state;  /* 0 = none, 1 = loaded, 2 = failed */
	/* 0x0C */ string unk_0C;
	/* 0x10 */ string unk_10;
	/* 0x14 */ string unk_14;

	static int instance_count;

	SoundLoadData() : bgmThread(0), seThread(0), state(0) { instance_count++; }

	void ReleaseThread();
	~SoundLoadData()
	{
		instance_count--;
		ReleaseThread();
	}
};

inline void SoundLoadData::ReleaseThread()
{
	if (bgmThread) {
		bgmThread->abort();
		bgmThread->release();
		bgmThread = 0;
	}
	if (seThread) {
		seThread->abort();
		seThread->release();
		seThread = 0;
	}
}

/* File descriptor of /dev/dsp2, -1 while closed. */
int g_dspFd = -1;

int SoundLoadData::instance_count = 0;

/* Last soundBgmChange request, used by soundBgmSeek to restart the player. */
static int s_bgmChanged = 0;
static int s_bgmChangeArg = 0;

/* Paths of the last loaded banks: 0-2 BGM (bd, hd, sq), 3-4 SE (bd, hd); read by soundGetInfo. */
static string s_bgmBd;
static string s_bgmHd;
static string s_bgmSq;
static string s_seBd;
static string s_seHd;

#include "script/scenewindow.h"

static void SndBgmLoad(thJob *th);
static int SndBgmLoadDone(thJob *th, AMX *amx);
static void SndSeLoad(thJob *th);
static int SndSeLoadDone(thJob *th, AMX *amx);

/*
 * OSS setup of the DSP, used by both bnDspInit and bnDspExit: mutes the three
 * mixer channels, writes 8 blocks of silence, resets the device, sets
 * format/channels/rate, the trigger and the fragments (8 x 4 KB), then sets
 * the mixers back to 0x8080. Ioctl codes are from <sys/soundcard.h>. Stops at
 * the first failing ioctl; the last one is not checked.
 */
static void dspSetupPre()
{
	int level;
	int bits;
	int channels;
	int rate;
	int trigger;
	int frag;
	int i;
	/* Silence written to the DSP before reconfiguring it. */
	static char s_dspSilence[512];

	rate = 48000;
	bits = AFMT_S16_LE;
	channels = 2;
	if (g_dspFd < 0)
		goto out;
	level = 0;
	if (ioctl(g_dspFd, SOUND_MIXER_WRITE_VOLUME, &level) < 0)
		goto out;
	if (ioctl(g_dspFd, SOUND_MIXER_WRITE_PCM, &level) < 0)
		goto out;
	if (ioctl(g_dspFd, SOUND_MIXER_WRITE_ALTPCM, &level) < 0)
		goto out;
	memset(s_dspSilence, 0, sizeof(s_dspSilence));
	for (i = 0; i < 8; i++)
		write(g_dspFd, s_dspSilence, sizeof(s_dspSilence));
	if (ioctl(g_dspFd, SNDCTL_DSP_RESET, 0) < 0)
		goto out;
	if (ioctl(g_dspFd, SNDCTL_DSP_SETFMT, &bits) < 0)
		goto out;
	if (ioctl(g_dspFd, SNDCTL_DSP_CHANNELS, &channels) < 0)
		goto out;
	if (ioctl(g_dspFd, SNDCTL_DSP_SPEED, &rate) < 0)
		goto out;
	trigger = 1;
	if (ioctl(g_dspFd, 0x80045602, &trigger) < 0)
		goto out;
	frag = 0x0008000c;
	if (ioctl(g_dspFd, SNDCTL_DSP_SETFRAGMENT, &frag) < 0)
		goto out;
	level = 0x8080;
	if (ioctl(g_dspFd, SOUND_MIXER_WRITE_VOLUME, &level) < 0)
		goto out;
	if (ioctl(g_dspFd, SOUND_MIXER_WRITE_PCM, &level) < 0)
		goto out;
	ioctl(g_dspFd, SOUND_MIXER_WRITE_ALTPCM, &level);
out:
	return;
}

void bnDspInit()
{
	dspSetupPre();
	soundinit();
	sceSdRemote(1, 4496, 0, 5160, 5160);
	dspSetupPost();
}

void bnDspExit()
{
	dspShutdownPre();
	soundquit();
	sceSdRemoteEnd();
	dspSetupPre();
}

void SndExtension_Init()
{
	if (g_dspFd >= 0) {
		close(g_dspFd);
		g_dspFd = -1;
	}

	g_dspFd = open("/dev/dsp2", O_WRONLY);
	if (g_dspFd < 0)
		return;

	if (fcntl(g_dspFd, F_SETFD, FD_CLOEXEC) < 0) {
		close(g_dspFd);
		g_dspFd = -1;
		return;
	}

	bnDspInit();
}

void SndExtension_Exit()
{
	bnDspExit();

	if (g_dspFd >= 0) {
		close(g_dspFd);
		g_dspFd = -1;
	}
}

static cell AMX_NATIVE_CALL n_sndSePlay(AMX *amx, cell *params)
{
	if (SystemParam::enable_se)
		sceSdRemote(1, 5424, 1, params[1]);
	return 0;
}

static cell AMX_NATIVE_CALL n_sndBgmPlay(AMX *amx, cell *params)
{
	if (SystemParam::enable_bgm)
		sceSdRemote(1, 5008, params[1]);
	return 0;
}

/* sndBgmStop: unlike sndBgmPlay, ignores enable_bgm. */
static cell AMX_NATIVE_CALL n_sndBgmStop(AMX *amx, cell *params)
{
	sceSdRemote(1, 4928, params[1], 0, 0);
	return 0;
}

/* The sound load state of a script, user data '_SNL'. */
static inline SoundLoadData *ScriptSoundData(Script *owner)
{
	return (SoundLoadData *)owner->GetUserData(TAG_LNS);
}

/* The script that runs amx, user data 'RCSP' of the AMX. */
static inline Script *AmxScript(AMX *amx)
{
	Script *s;

	amx_GetUserData(amx, TAG_RCSP, (void **)&s);
	return s;
}

/* Script callback: deletes the '_SNL' loading state (the destructor stops both threads). */
static void SoundLoadDataFree(AMX *amx)
{
	SoundLoadData *d;

	d = ScriptSoundData(AmxScript(amx));
	delete d;
}

/* Tail of bnDspInit: driver command 0x01008010 with 16384, and clears the five bank paths. */
static void dspSetupPost()
{
	sceSdRemote(1, 0x01008010, 16384);
	s_bgmBd.erase();
	s_bgmHd.erase();
	s_bgmSq.erase();
	s_seBd.erase();
	s_seHd.erase();
}

/* Shutdown helper of bnDspExit: quits MIDI SE/BGM and sends driver command 0x010000D0. */
static void dspShutdownPre()
{
	midiQuitSE();
	midiQuitBGM();
	s_bgmChanged = 0;
	sceSdRemote(1, 0x010000D0);
}

#include "http/httpfile.h"
#include "app/strutil.h"

/*
 * Normalizes the extension of an audio path: returns 0 if it ends with `a`,
 * 1 if it ends with `b`; otherwise (no extension too) appends `b` and returns 1.
 */
static int SndFixExt(string &path, const string &a, const string &b)
{
	unsigned int p = path.find_last_of(".");

	if (p != string::npos) {
		string ext = path.substr(p);

		if (ext == a)
			return 0;
		if (ext == b)
			return 1;
	}
	path.append(b);
	return 1;
}

/*
 * soundBgmLoadStart(bd, hd, sq): stops the BGM player and its eight channels,
 * then starts a thJob loading the three banks (see soundSeLoadStart). The
 * paths are kept in bank paths 0-2.
 */
static cell AMX_NATIVE_CALL n_soundBgmLoadStart(AMX *amx, cell *params)
{
	int i;

	s_bgmChanged = 0;
	s_bgmChangeArg = 0;
	sceSdRemote(1, 0x01000020, 0);
	for (i = 0; i < 8; i++)
		sceSdRemote(1, 4928, i, 0, 0);

	string bd = CellToString(amx, params[1]);
	string hd = CellToString(amx, params[2]);
	string sq = CellToString(amx, params[3]);
	int bde = SndFixExt(bd, ".bd", ".bde");
	int hde = SndFixExt(hd, ".hd", ".hde");
	int sqe = SndFixExt(sq, ".sq", ".sqe");
	SoundLoadData *d;

	if (bde == 0 || hde == 0 || sqe == 0) {
		amx_RaiseError(amx, 10);
		return 0;
	}
	GetSceneWindow(amx)->MakeFullPath(bd);
	GetSceneWindow(amx)->MakeFullPath(hd);
	GetSceneWindow(amx)->MakeFullPath(sq);
	d = ScriptSoundData(AmxScript(amx));
	if (d != 0) {
		if (d->bgmThread != 0) {
			d->bgmThread->abort();
			d->bgmThread->release();
			d->bgmThread = 0;
		}
		if (d->seThread != 0) {
			d->seThread->abort();
			d->seThread->release();
			d->seThread = 0;
		}
	} else {
		d = new SoundLoadData;
		AmxScript(amx)->SetUserData(TAG_LNS, d);
		AmxScript(amx)->SetCallback(Script::CALLBACK_ON_DESTROY, TAG_LNS, SoundLoadDataFree);
	}
	d->bgmThread = new thJob(amx, (void (*)(thJob *))SndBgmLoad, (bool (*)(thJob *, void *))SndBgmLoadDone, 0);
	d->bgmThread->m_str[0] = bd;
	d->bgmThread->m_str[1] = hd;
	d->bgmThread->m_str[2] = sq;
	d->bgmThread->m_int[SND_JOB_DECODE] = (bde != 0) | (hde != 0 ? 2 : 0) | (sqe != 0 ? 4 : 0);
	d->bgmThread->addThreadIdleList(5, 10000);
	d->state = 0;
	s_bgmBd = bd;
	s_bgmHd = hd;
	s_bgmSq = sq;
	return 0;
}

/* soundBgmLoadSync(): 0 while loading, 1 if loaded, -1 if failed or not started; resets the result. */
static cell AMX_NATIVE_CALL n_soundBgmLoadSync(AMX *amx, cell *params)
{
	SoundLoadData *d;

	d = ScriptSoundData(AmxScript(amx));
	if (d == 0)
		return -1;
	if (d->bgmThread != 0)
		return 0;
	if (d->state != 1) {
		d->state = 0;
		return -1;
	}
	d->state = 0;
	return 1;
}

/*
 * soundSeLoadStart(bd, hd): normalizes the two extensions (a plain ".bd" or
 * ".hd" raises AMX error 10), resolves them against the scene window, stops the loads in progress (or creates the
 * '_SNL' state) and starts a thJob with SndSeLoad/SndSeLoadDone. The paths
 * are kept in bank paths 3-4.
 */
static cell AMX_NATIVE_CALL n_soundSeLoadStart(AMX *amx, cell *params)
{
	string bd = CellToString(amx, params[1]);
	string hd = CellToString(amx, params[2]);
	int bde = SndFixExt(bd, ".bd", ".bde");
	int hde = SndFixExt(hd, ".hd", ".hde");
	SoundLoadData *d;

	if (bde == 0 || hde == 0) {
		amx_RaiseError(amx, 10);
		return 0;
	}
	GetSceneWindow(amx)->MakeFullPath(bd);
	GetSceneWindow(amx)->MakeFullPath(hd);
	d = ScriptSoundData(AmxScript(amx));
	if (d != 0) {
		if (d->bgmThread != 0) {
			d->bgmThread->abort();
			d->bgmThread->release();
			d->bgmThread = 0;
		}
		if (d->seThread != 0) {
			d->seThread->abort();
			d->seThread->release();
			d->seThread = 0;
		}
	} else {
		d = new SoundLoadData;
		AmxScript(amx)->SetUserData(TAG_LNS, d);
		AmxScript(amx)->SetCallback(Script::CALLBACK_ON_DESTROY, TAG_LNS, SoundLoadDataFree);
	}
	d->seThread = new thJob(amx, (void (*)(thJob *))SndSeLoad, (bool (*)(thJob *, void *))SndSeLoadDone, 0);
	d->seThread->m_str[0] = bd;
	d->seThread->m_str[1] = hd;
	d->seThread->m_int[SND_JOB_DECODE] = (bde != 0) | (hde != 0 ? 2 : 0);
	d->seThread->addThreadIdleList(5, 10000);
	d->state = 0;
	s_seBd = bd;
	s_seHd = hd;
	return 0;
}

/* soundSeLoadSync(): as soundBgmLoadSync, on the SE thread. */
static cell AMX_NATIVE_CALL n_soundSeLoadSync(AMX *amx, cell *params)
{
	SoundLoadData *d;

	d = ScriptSoundData(AmxScript(amx));
	if (d == 0)
		return -1;
	if (d->seThread != 0)
		return 0;
	if (d->state != 1) {
		d->state = 0;
		return -1;
	}
	d->state = 0;
	return 1;
}

static inline StringUtil Prefix(const string &s, unsigned int n)
{
	return s.substr(0, n);
}

static inline StringUtil Suffix(const string &s, unsigned int n)
{
	return s.substr(n, s.length() - n);
}

static inline int SchemeCmp(const StringUtil &pre, StringUtil pat)
{
	StringUtil head(pre);
	head.MakeUpper();
	pat.MakeUpper();
	return head.compare(pat);
}

/* `src` without its first `n` characters (the scheme), assigned to `dst`. */
static inline void StripScheme(string &dst, const string &src, unsigned int n)
{
	dst = Suffix(src, n);
}

/*
 * Resolves a resource URL to a local path and returns an HTTP status (200 =
 * ok): "http:"/"https:" are downloaded into the cache by HttpFile::Request,
 * "file:" loses its scheme, anything else is already a path. Scheme
 * comparisons are case-insensitive.
 */
static int SndFetchUrl(string *url, string *path)
{
	HttpFile hf;
	sceHttpFileRequestArg req;
	int cookie;
	int ret;

	memset(&req, 0, sizeof(req));
	req.done_flag = &cookie;
	req.timeout = 30000;
	string u = *url;
	if (SchemeCmp(Prefix(u, 5), "http:") == 0 || SchemeCmp(Prefix(u, 6), "https:") == 0) {
		ret = hf.Request(*url, 0, 0, &req);
		if (cookie)
			hf.save_cookie();
		*path = (const char *)hf;
	} else if (SchemeCmp(Prefix(u, 5), "file:") == 0) {
		ret = 200;
		StripScheme(*path, u, 5);
	} else {
		ret = 200;
		*path = *url;
	}
	return ret;
}

/* Reads a whole file into a malloc buffer and stores its size; 0 on any error. */
static void *SndLoadFile(const char *path, int *size)
{
	FILE *fp;
	void *buf;

	fp = fopen(path, "rb");
	if (fp != 0) {
		fseek(fp, 0, SEEK_END);
		*size = ftell(fp);
		rewind(fp);
		buf = malloc(*size);
		if (buf != 0) {
			if (fread(buf, 1, *size, fp) == (unsigned)*size) {
				fclose(fp);
				return buf;
			}
			free(buf);
			fclose(fp);
			return 0;
		}
		fclose(fp);
	}
	return 0;
}

/* Fetches a resource, reads it and, if `encoded` is set, decodes it; 0 or -1. */
static int SndLoadResource(string *url, void **pbuf, int *psize, int encoded)
{
	string path;
	int size;
	void *buf;

	if (SndFetchUrl(url, &path) != 200)
		return -1;
	buf = SndLoadFile(path.c_str(), &size);
	if (buf == 0)
		return -1;
	if (encoded != 0 && Script__Abort2(size, buf, &size) < 0)
		return -1;
	*pbuf = buf;
	*psize = size;
	return 0;
}

/* BGM thread body: loads the three banks and hands them to midiInitBGM; result in m_int[SND_JOB_RESULT]. */
static void SndBgmLoad(thJob *th)
{
	void *vb = 0;
	int vbSize;
	void *hd = 0;
	int hdSize;
	void *sq = 0;
	int sqSize;
	unsigned int flags = th->m_int[SND_JOB_DECODE];
	int hdEncoded = (flags & 2) != 0;
	int sqEncoded = (flags & 4) != 0;

	if (SndLoadResource(&th->m_str[0], &vb, &vbSize, flags & 1) != 0
	    || SndLoadResource(&th->m_str[1], &hd, &hdSize, hdEncoded) != 0
	    || SndLoadResource(&th->m_str[2], &sq, &sqSize, sqEncoded) != 0
	    || midiInitBGMPtr((const char *)vb, vbSize, hd, hdSize, sq, sqSize) < 0)
		th->m_int[SND_JOB_RESULT] = -1;
	else
		th->m_int[SND_JOB_RESULT] = 0;
	if (vb != 0) {
		free(vb);
		vb = 0;
	}
	if (hd != 0) {
		free(hd);
		hd = 0;
	}
	if (sq != 0) {
		free(sq);
		sq = 0;
	}
}

/* BGM completion callback: stores the result in the '_SNL' state (1 = ok, 2 = failed) and drops the thread. */
static int SndBgmLoadDone(thJob *th, AMX *amx)
{
	SoundLoadData *d;

	d = ScriptSoundData(AmxScript(amx));
	if (th->m_int[SND_JOB_RESULT] == 0)
		d->state = 1;
	else
		d->state = 2;
	if (d->bgmThread != 0) {
		d->bgmThread->abort();
		d->bgmThread->release();
		d->bgmThread = 0;
	}
	return 1;
}

/* SE thread body: as SndBgmLoad, with two banks and midiInitSE. */
static void SndSeLoad(thJob *th)
{
	void *vb = 0;
	int vbSize;
	void *hd = 0;
	int hdSize;
	unsigned int flags = th->m_int[SND_JOB_DECODE];
	int hdEncoded = (flags & 2) != 0;

	if (SndLoadResource(&th->m_str[0], &vb, &vbSize, flags & 1) != 0
	    || SndLoadResource(&th->m_str[1], &hd, &hdSize, hdEncoded) != 0
	    || midiInitSEPtr((const char *)vb, vbSize, hd, hdSize) < 0)
		th->m_int[SND_JOB_RESULT] = -1;
	else
		th->m_int[SND_JOB_RESULT] = 0;
	if (vb != 0) {
		free(vb);
		vb = 0;
	}
	if (hd != 0) {
		free(hd);
		hd = 0;
	}
}

/* SE completion callback: as SndBgmLoadDone, on the SE thread. */
static int SndSeLoadDone(thJob *th, AMX *amx)
{
	SoundLoadData *d;

	d = ScriptSoundData(AmxScript(amx));
	if (th->m_int[SND_JOB_RESULT] == 0)
		d->state = 1;
	else
		d->state = 2;
	if (d->seThread != 0) {
		d->seThread->abort();
		d->seThread->release();
		d->seThread = 0;
	}
	return 1;
}

static cell AMX_NATIVE_CALL n_soundBgmPlay(AMX *amx, cell *params)
{
	sceSdRemote(1, 0x01000000, 0);
	return 0;
}

static cell AMX_NATIVE_CALL n_soundBgmStop(AMX *amx, cell *params)
{
	sceSdRemote(1, 0x01000020, 0);
	return 0;
}

/* soundBgmSeek(pos): restarts the player with the last soundBgmChange request if it is idle, then seeks. */
static cell AMX_NATIVE_CALL n_soundBgmSeek(AMX *amx, cell *params)
{
	if (sceSdRemote(1, 0x01008090, 0) == 0)
		sceSdRemote(1, 0x01000040, gDataTopAddrForCslMidi, s_bgmChanged, s_bgmChangeArg);
	sceSdRemote(1, 0x01000030, params[1]);
	return 0;
}

/* soundBgmChange(arg): the only one here returning the sceSdRemote result instead of 0. */
static cell AMX_NATIVE_CALL n_soundBgmChange(AMX *amx, cell *params)
{
	s_bgmChanged = 1;
	s_bgmChangeArg = params[1];
	return sceSdRemote(1, 0x01000040, gDataTopAddrForCslMidi, 1, params[1]);
}

static cell AMX_NATIVE_CALL n_soundSePlay(AMX *amx, cell *params)
{
	midiSePlay((short)params[1], (short)params[2]);
	return 0;
}

/*
 * soundGetInfo(what, dest, size): what 0-4 copies a bank path (packed,
 * truncated to `size` cells), 5 stores the BGM player status in a cell.
 */
static cell AMX_NATIVE_CALL n_soundGetInfo(AMX *amx, cell *params)
{
	int ok = 0;
	string s;
	cell *dest;
	cell *ip;
	cell len, addr, end;
	int val = 0;
	int valid;

	switch (params[1]) {
	case 0:
		ok = 1;
		s = s_bgmBd;
		break;
	case 1:
		ok = 1;
		s = s_bgmHd;
		break;
	case 2:
		ok = 1;
		s = s_bgmSq;
		break;
	case 3:
		ok = 1;
		s = s_seBd;
		break;
	case 4:
		ok = 1;
		s = s_seHd;
		break;
	case 5:
		val = sceSdRemote(1, 0x01008090, 0);
		break;
	default:
		amx_RaiseError(amx, 10);
		return 0;
	}
	if (ok) {
		if (params[3] * 4 < s.length() + 1)
			s = string(s, 0, params[3] * 4 - 1);
		len = s.length();
		addr = params[2];
		end = len + addr;
		if (addr >= 0 && addr < amx->hea && end >= 0 && end < amx->hea)
			valid = 1;
		else
			valid = addr >= amx->stk && addr < amx->stp && end >= amx->stk && end < amx->stp;
		if (!valid) {
			amx_RaiseError(amx, AMX_ERR_MEMACCESS);
			return 0;
		}
		amx_GetAddr(amx, params[2], &dest);
		amx_SetString(dest, (char *)s.c_str(), 1);
	} else {
		if (amx_GetAddr(amx, params[2], &ip) == 0)
			*ip = val;
	}
	return 0;
}

AMX_NATIVE_INFO smallext_snd[] = {
	{ "sndSePlay", n_sndSePlay },
	{ "sndBgmPlay", n_sndBgmPlay },
	{ "sndBgmStop", n_sndBgmStop },
	{ "soundBgmLoadStart", n_soundBgmLoadStart },
	{ "soundBgmLoadSync", n_soundBgmLoadSync },
	{ "soundBgmPlay", n_soundBgmPlay },
	{ "soundBgmStop", n_soundBgmStop },
	{ "soundBgmSeek", n_soundBgmSeek },
	{ "soundBgmChange", n_soundBgmChange },
	{ "soundSeLoadStart", n_soundSeLoadStart },
	{ "soundSeLoadSync", n_soundSeLoadSync },
	{ "soundSePlay", n_soundSePlay },
	{ "soundGetInfo", n_soundGetInfo },
	{ 0, 0 },
};
