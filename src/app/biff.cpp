/*
 * Biff: the new-mail indicator. A periodic tick runs the external pop3check
 * tool on the configured mail account and records whether mail is waiting.
 */

#include <map>
#include <string>
#include <iostream.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/wait.h>
#include "systemoptions.h"
#include "thread/thposixthread.h"

/* Debug print of the settings read, disabled. */
#define BIFF_DEBUG 0

#include "bn_base64.h"
#include "cfd/cfd.h"
#include "app/systemparam.h"
#include <sys/stat.h>
#include "biff.h"
#include "thread/thjob.h"
#include "fs/bbfile.h"

/* Results of the mail check job (m_int[BIFF_JOB_RESULT]); the last three also
   become s_state. */
enum {
	BIFF_RETRY = -10000, /* the check failed, try again on the next tick */
	BIFF_ERROR = -1,
	BIFF_NO_MAIL = 0,
	BIFF_NEW_MAIL = 1
};

/* thJob::m_int slot of the mail check job. */
enum {
	BIFF_JOB_RESULT = 0
};

/* Exit statuses of /opt0/bn/tools/pop3check (2 also when execl fails). */
enum {
	POP3CHECK_NEW_MAIL,
	POP3CHECK_NO_MAIL,
	POP3CHECK_FAILED,
	POP3CHECK_ERROR
};

static int s_soundHandle = 0;
static int s_state = 0;         /* > 0 new mail, 0 none, < 0 disabled */
static int s_tickCount = 0;     /* ticks counted by Run(); -1 disables the check */
static int s_intervalMin = 0;   /* polling interval in minutes */
static int s_suspendCount = 0;  /* > 0 suspends polling */
static int s_retryCount = 0;
static int s_syncNavi = 1;

/* Key/value settings of the two ini files read by Init; s_mailB holds the
   mail account. */
static map<string, string> s_mailA;
static map<string, string> s_mailB;

static void CheckMail();

static int LoadIni(const char *path, map<string, string> &m);
static int GetValue(string &out, const char *key, map<string, string> &m);
static int SetValue(const char *val, const char *key, map<string, string> &m);

static int ParseIni(char *buf, map<string, string> &m);

static int ParseIni(char *buf, map<string, string> &m)
{
	char *top;
	char *line;

	/* NOTE: bug: `top` is used uninitialized */
	if ((line = strsep(&top, "</folder>")) == 0)
		return -1;
	while ((line = strsep(&buf, "\r\n")) != 0 && line != top) {
		char *key = strsep(&line, "=");

		if (key == 0 || line == 0)
			continue;
		m[key] = line;
	}
	return 0;
}

/* Reads a whole configuration file into a malloc'ed buffer and parses it.
   Any failure (stat, malloc, open, short read) returns BIFF_RETRY, the
   result code of the mail job. */
static int LoadIni(const char *path, map<string, string> &m)
{
	struct stat st;
	int ret;
	char *buf;

	if (BBlstatCPP(path, &st) < 0)
		return BIFF_RETRY;
	buf = (char *)malloc(st.st_size + 1);
	if (buf != 0) {
		int fd = BBopenCPP(path, O_RDONLY);
		if (fd >= 0) {
			int n = BBreadCPP(fd, buf, st.st_size);

			if (n == st.st_size) {
				buf[n] = 0;
				ret = ParseIni(buf, m);
			} else {
				ret = BIFF_RETRY;
			}
			BBcloseCPP(fd);
		} else {
			ret = BIFF_RETRY;
		}
		free(buf);
	} else {
		ret = BIFF_RETRY;
	}
	return ret;
}

/* Copies m[key] to `out` if present; returns its length, or -1. */
static int GetValue(string &out, const char *key, map<string, string> &m)
{
	map<string, string>::iterator it;

	if ((it = m.find(key)) != m.end()) {
		out = (*it).second;
		return out.length();
	}
	return -1;
}

static int SetValue(const char *val, const char *key, map<string, string> &m)
{
	map<string, string>::iterator it;

	if ((it = m.find(key)) != m.end()) {
		(*it).second = val;
		return 0;
	}
	return -1;
}

/* Mail check job, run on the CheckMail thread. Reads the POP server
   settings from s_mailB, validates port (0..65535) and protocol (0/1), then
   runs pop3check in a child process. Its exit status becomes the result
   (BIFF_RETRY when pop3check does not run or fails). */
static void BiffMailJob(thJob *job)
{
	struct stat st;

	if (BBlstatCPP("/opt0/bn/tools/pop3check", &st) < 0)
		return;

	string server;
	string port;
	string protocol;
	string user;
	string password;

	if (GetValue(server, "receive_server", s_mailB) <= 0)
		return;
	if (GetValue(port, "pop_port", s_mailB) <= 0)
		return;
	if (GetValue(protocol, "protocol", s_mailB) <= 0)
		return;
	if (GetValue(user, "user_id", s_mailB) <= 0)
		return;
	if (GetValue(password, "password", s_mailB) <= 0)
		return;
	if (BIFF_DEBUG)
		cerr << server << endl;

	int portno = strtol(port.c_str(), 0, 10);
	if (portno < 0 || portno > 65535)
		return;
	int proto = strtol(protocol.c_str(), 0, 10);
	if (proto < 0 || proto > 1)
		return;

	int pid = fork();
	switch (pid) {
	case -1:
		job->m_int[BIFF_JOB_RESULT] = BIFF_RETRY;
		break;
	case 0: {
		char uidl[256];
		char lock[256];

		sprintf(uidl, "/contents/mailer/bn/mailconf/uidl/");
		strcat(uidl, server.c_str());
		strcat(uidl, "-");
		strcat(uidl, user.c_str());
		strcat(uidl, "");
		sprintf(lock, "/contents/mailer/bn/mailconf/uidl/");
		strcat(lock, "lock");
		strcat(lock, "");
		/* NOTE: bug: the argument list is not terminated by a null pointer */
		execl("/opt0/bn/tools/pop3check", "/opt0/bn/tools/pop3check", server.c_str(), port.c_str(),
		    protocol.c_str(), user.c_str(), password.c_str(), uidl, lock);
		exit(POP3CHECK_FAILED);
	}
	default: {
		int status;

		if (wait(&status) <= 0)
			job->m_int[BIFF_JOB_RESULT] = BIFF_RETRY;
		else if (!WIFEXITED(status))
			job->m_int[BIFF_JOB_RESULT] = BIFF_RETRY;
		else {
			switch (WEXITSTATUS(status)) {
			case POP3CHECK_NEW_MAIL:
				job->m_int[BIFF_JOB_RESULT] = BIFF_NEW_MAIL;
				break;
			case POP3CHECK_NO_MAIL:
				job->m_int[BIFF_JOB_RESULT] = BIFF_NO_MAIL;
				break;
			case POP3CHECK_FAILED:
				job->m_int[BIFF_JOB_RESULT] = BIFF_RETRY;
				break;
			case POP3CHECK_ERROR:
				job->m_int[BIFF_JOB_RESULT] = BIFF_ERROR;
				break;
			default:
				job->m_int[BIFF_JOB_RESULT] = BIFF_RETRY;
				break;
			}
		}
		break;
	}
	}
}

/* Completion callback of the mail job. BIFF_RETRY: after four
   attempts the tick is disabled and the state set to -1. Any other result
   becomes the state. */
static bool BiffMailDone(thJob *job, void *arg)
{
	int r;

	if (s_state >= 0) {
		r = job->m_int[BIFF_JOB_RESULT];
		if (r != BIFF_ERROR && s_suspendCount == 1) {
			if (r == BIFF_RETRY) {
				s_retryCount++;
				if (s_retryCount >= 4) {
					s_tickCount = -1;
					s_state = -1;
				}
			} else {
				s_retryCount = 0;
				s_state = r;
			}
		}
	}
	s_suspendCount = 0;
	return true;
}

/* Starts the mail check job on a POSIX thread. */
static void CheckMail()
{
	s_suspendCount = 1;

	thJob *job = new thJob(0, BiffMailJob, BiffMailDone, 0);

	job->start();
	job->release();
}

/* Reads the account and common ini files and the three autochk_* settings,
   and decodes the stored password (base64, XOR with the GetUniqueID key,
   CheckUniqueID) back into plain text in the map. */
void Biff::Init(int flag)
{
	if (flag == 0) {
		struct stat st;
		char path[256];

		sprintf(path, "pfs:__contents");
		strcat(path, "/contents/mailer/bn/mailconf/uidl/");
		strcat(path, "lock");
		strcat(path, "");
		if (BBlstatCPP(path, &st) >= 0)
			BBremoveCPP(path);
		s_state = 0;
		s_suspendCount = 0;
	}

	s_tickCount = 0;
	s_intervalMin = 0;
	if (s_suspendCount > 0)
		s_suspendCount = 2;
	s_retryCount = 0;
	s_mailA.clear();
	s_mailB.clear();

	if (LoadIni("pfs:__contents/contents/mailer/bn/mailconf/account.ini", s_mailB) < 0)
		s_tickCount = -1;
	if (LoadIni("pfs:__contents/contents/mailer/bn/mailconf/common.ini", s_mailA) < 0)
		s_tickCount = -1;

	string s;

	if (GetValue(s, "autochk_sound", s_mailA) <= 0)
		s_soundHandle = 0;
	s_soundHandle = strtol(s.c_str(), 0, 10);
	if (s_soundHandle <= 0)
		s_soundHandle = 0;

	if (GetValue(s, "autochk_newmail", s_mailA) <= 0) {
		s_tickCount = -1;
	} else {
		switch ((int)strtol(s.c_str(), 0, 10)) {
		case 1:
			if (flag != 0)
				s_tickCount = -1;
			s_intervalMin = 0;
			goto Lpassword;
		case 2:
			if (GetValue(s, "autochk_interval", s_mailA) <= 0) {
				s_tickCount = -1;
				break;
			}
			s_intervalMin = strtol(s.c_str(), 0, 10);
			if (s_intervalMin <= 0)
				s_tickCount = -1;
			else
				s_tickCount = s_intervalMin * SysOption.m_frameRate * 60;
			break;
		case 0:
		default:
			s_intervalMin = 0;
			s_tickCount = -1;
			break;
		}
	}
Lpassword:
	if (GetValue(s, "password", s_mailB) <= 0)
		goto Lfail;
	{
		char pass[128];  /* unused, but part of the layout */
		char buf[256];
		char dec[256];
		unsigned char key[64];
		char id[128];
		int n;
		n = s.copy(buf, s.length() > 255 ? 256 : s.length(), 0);
		n = bn_base64_decode((unsigned char *)dec, buf, n);
		if (n >= 128)
			goto Lfail;
		if (n < 8)
			goto Lfail;
		if (Biff::GetUniqueID(key) < 0)
			goto Lfail;
		{
			int m = n < 65 ? n : 64;
			int i;
			for (i = 0; i < n; i++) {
				if (i < m)
					dec[i] = dec[i] ^ key[i];
				if ((unsigned int)((unsigned char)dec[i] - 32) >= 95)
					goto Lfail;
			}
		}
		dec[n] = 0;
		if (Biff::CheckUniqueID(dec, id) < 0)
			goto Lfail;
		SetValue(id, "password", s_mailB);
		if (GetValue(s, "password", s_mailB) < 0)
			goto Lfail;
	}
	return;
Lfail:
	SetValue("", "password", s_mailB);
	s_tickCount = -1;
}

void Biff::Term()
{
	s_tickCount = -1;
	s_intervalMin = 0;
	s_mailA.clear();
	s_mailB.clear();
}

/* Starts a mail check every s_intervalMin minutes; SysOption.m_frameRate is
   the tick rate per second. */
void Biff::Run()
{
	if (s_tickCount < 0)
		return;
	if (SystemParam::network_up == 0)
		return;
	s_tickCount++;
	if (s_suspendCount > 0)
		return;
	if (s_state < 0)
		return;
	if (SystemParam::enable_navimenu == 0 && s_syncNavi != 0)
		return;
	if (s_intervalMin == 0) {
		CheckMail();
		s_tickCount = -1;
	}
	int hz = SysOption.m_frameRate;
	if (s_intervalMin * hz * 60 < s_tickCount) {
		s_tickCount = 0;
		CheckMail();
	}
}

bool Biff::Sound()
{
	return s_soundHandle > 0;
}

bool Biff::Exist()
{
	return s_state > 0;
}

void Biff::Clear()
{
	s_state = 0;
}

void Biff::Sleep()
{
	s_state = -1;
}

void Biff::Wake()
{
	if (s_state < 0)
		s_state = 0;
}

void Biff::SetSyncNavi(int flag)
{
	if (!flag)
		s_syncNavi = 0;
	else
		s_syncNavi = 1;
}

/* Checks that `id` starts with "_bnMAIL." (stored nibble-swapped) and
   copies what follows to `out`. */
int Biff::CheckUniqueID(const char *id, char *out)
{
	char *p = (char *)id;
	unsigned char key[8] = { 0xf5, 0x26, 0xe6, 0xd4, 0x14, 0x94, 0xc4, 0xe2 };

	for (int i = 0; i < 8; i++)
		key[i] = (key[i] >> 4) | (key[i] << 4);

	if (strncmp(p, (const char *)key, 8) != 0)
		return -1;
	if (strsep(&p, ".") == 0 || p == 0)
		return -1;
	/* NOTE: bug: the id is used as the format string */
	sprintf(out, p);
	return 1;
}

/* Expands the 24-byte session-file key from __cfd_symbol4 into a 64-byte obfuscated id. */
int Biff::GetUniqueID(unsigned char *out)
{
	unsigned char buf[32];
	int i;
	int sum;
	memset(out, 0, 64);
	if (__cfd_symbol4(buf) < 0)
		return -1;

	for (i = 0, sum = 0; i < 24; i++)
		sum += buf[i];

	for (i = 0; i < 12; i++) {
		out[2 + i] = (((i + sum) & 1) ? buf[i] : ~buf[i]) ^ ((buf[23 - i] << 4) | (buf[23 - i] >> 4));
	}

	out[0] = (out[8] & 0xe0) | (out[3] & 0x18) | (out[12] & 0x07);
	out[1] = (out[6] & 0xe0) | (out[13] & 0x18) | (out[5] & 0x07);
	out[14] = (out[11] & 0xe0) | (out[9] & 0x18) | (out[2] & 0x07);
	out[15] = (out[4] & 0xe0) | (out[10] & 0x18) | (out[7] & 0x07);
	for (i = 0; i < 24; i++) {
		int r = (i + sum) % 8;
		int a = buf[i];
		out[16 + i] = ((i + sum) & 1) ? ((a << r) | (a >> (8 - r))) : ~((a << r) | (a >> (8 - r)));
	}

	for (i = 0; i < 24; i++) {
		int a = buf[i];
		int b = buf[23 - i];
		out[40 + i] = (i & 1) ? ((a << 6) | (b >> 2)) : ~((a << 6) | (b >> 2));
	}

	return 0;
}
