/*
 * DNAS2Sysupdater: applies the RPM section of a DNAS2 system update
 * (signature checks, filesystem remounts, package installs).
 */

#include "dnas2sysupdater.h"
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/mount.h>
#include <sys/wait.h>

#include "lib/scepfsd.h"
#include "dnas2sysupmanager.h"

DNAS2Sysupdater::DNAS2Sysupdater(DNAS2SysupManager *manager)
{
	m_manager = manager;
}

DNAS2Sysupdater::~DNAS2Sysupdater()
{}

// SysupRPM: validate the section, check the signatures of the RPM data of the section, remount the
// filesystems read-write, install the packages and remount read-only. A
// failed install still remounts, but returns the install error.
int DNAS2Sysupdater::SysupRPM(sysupCommandList *list)
{
	int ret;

	ret = checkDataTagNumOnList(list, 1001, 3001);
	if (ret < 0)
		goto done;

	ret = doSysupRPMDataCheckSig(list, 1001, 3001);
	if (ret < 0)
		goto done;

	ret = doReiserfsRemount(1);
	if (ret < 0)
		goto done;

	ret = doSysupRPMDataRPM(list, 1001, 3001);
	if (ret < 0) {
		doReiserfsRemount(0);
		goto done;
	}

	ret = doReiserfsRemount(0);
	if (ret >= 0)
		return 0;

done:
	return ret;
}

int DNAS2Sysupdater::SysupKernel(sysupCommandList *list)
{
	return 0;
}

int DNAS2Sysupdater::SysupMBR(sysupCommandList *list)
{
	return 0;
}

int DNAS2Sysupdater::SysupPFS(sysupCommandList *list)
{
	return 0;
}

// checkDataTagNumOnList: check that list->tag == tagA and that the node
// reached after *list->data + 1 steps has tag == idxB.
int DNAS2Sysupdater::checkDataTagNumOnList(sysupCommandList *list, unsigned int tagA, unsigned int idxB)
{
	if (list->tag != tagA)
		goto fail;

	{
		unsigned int count = *(unsigned int *)list->data + 1;
		sysupCommandList *node = list;
		for (unsigned int i = 0; i < count; i++)
			node = node->next;
		if (node->tag == idxB)
			return 0;
	}

fail:
	return -1005;
}

// doSysupRPMDataCheckSig: for each node tagged 2001 up to the node tagged
// idxB, check the package signature unless the package is to be erased
// (mode "e"): copy to /bntmp, rpm --checksig, remove. Then count one step.
// Stops at the first error.
int DNAS2Sysupdater::doSysupRPMDataCheckSig(sysupCommandList *list, unsigned int tagA, unsigned int idxB)
{
	if (list->tag != tagA)
		return -1001;
	if (list->tag == idxB)
		goto done;

	do {
		if (list->tag == 2001) {
			RPMData *d = (RPMData *)list->data;
			int r = strncmp(d->mode, "e", 1);
			if (r != 0)
				goto pipeline;

			m_manager->CountUpStep();
			list = list->next;
			goto loopcheck;

pipeline:
			r = copyFileToBNTmpFromPackage(d->offset, d->size, d->len, d->name);
			if (r < 0)
				return r;
			r = execRPMCheckSigOnBNTmp(d->len, d->name);
			if (r < 0)
				return r;
			r = removeFileOnBNTmp(d->len, d->name);
			if (r < 0)
				return r;
			m_manager->CountUpStep();
		}
		list = list->next;
loopcheck:;
	} while (list->tag != idxB);

done:
	return 0;
}

// doReiserfsRemount: remount "/" read-only (mode 0) or read-write (mode 1),
// then /opt0, /net and /bntmp the same way. Returns 0 or -1002.
int DNAS2Sysupdater::doReiserfsRemount(int mode)
{
	unsigned long flags;
	const char *data;

	switch (mode) {
	case 0:
		flags = 0xc0ed0020;
		data = "suidimmu";
		break;
	case 1:
		flags = 0xc0ed0022;
		data = "";
		break;
	default:
		return -1002;
	}

	if (mount("/dev/hda1", "/", "reiserfs", flags, data) < 0)
		goto fail;
	if (mount("/dev/hda4", "/opt0", "reiserfs", flags, data) < 0)
		goto fail;
	if (mount("/dev/hda5", "/net", "reiserfs", flags, data) < 0)
		goto fail;
	if (mount("/dev/hda6", "/bntmp", "reiserfs", flags, data) < 0)
		goto fail;

	return 0;

fail:
	return -1002;

	return 0;
}

// doSysupRPMDataRPM: like doSysupRPMDataCheckSig, but always installs each
// package (copy to /bntmp, rpm, remove) and counts a step on success.
int DNAS2Sysupdater::doSysupRPMDataRPM(sysupCommandList *list, unsigned int tagA, unsigned int idxB)
{
	if (list->tag != tagA)
		return -1001;
	if (list->tag == idxB)
		goto done;

	do {
		if (list->tag == 2001) {
			RPMData *d = (RPMData *)list->data;
			int r = copyFileToBNTmpFromPackage(d->offset, d->size, d->len, d->name);
			if (r < 0)
				return r;
			r = execRPMOnBNTmp(d->len, d->name, d->mode_len, d->mode);
			if (r < 0)
				return r;
			r = removeFileOnBNTmp(d->len, d->name);
			if (r < 0)
				return r;
			m_manager->CountUpStep();
		}
		list = list->next;
	} while (list->tag != idxB);

done:
	return 0;
}

// parse_param: split str in place on spaces and tabs into argv[] (null
// terminated); returns the number of tokens. Trailing blanks give an empty
// token.
int DNAS2Sysupdater::parse_param(char *str, char **argv)
{
	int argc = 0;

	while (*str) {
		while (*str == ' ' || *str == '\t')
			*str++ = '\0';
		*argv++ = str;
		argc++;
		while (*str && *str != ' ' && *str != '\t')
			str++;
	}
	*argv = 0;
	return argc;
}

// execRPMCheckSigOnBNTmp: run "rpm --checksig" on /bntmp/name in a child
// process and accept the package if its output reports ": md5 gpg OK".
int DNAS2Sysupdater::execRPMCheckSigOnBNTmp(unsigned int len, char *name)
{
	char buf[272];
	char cmd[288];
	char out[256];
	char *argv[64];
	int fds[2];
	int status;
	int ret;

	fds[0] = -1;
	fds[1] = -1;
	strcpy(buf, "/bntmp/");
	strncat(buf, name, len);
	sprintf(cmd, "%s %s", "/bin/rpm --checksig", buf);
	if (pipe(fds) < 0) {
		ret = -1001;
		goto cleanup;
	}
	{
		int pid = fork();
		if (pid < 0) {
			ret = -1001;
			goto cleanup;
		}
		if (pid == 0) {
			if (dup2(fds[1], 1) < 0)
				exit(1);
			if (dup2(fds[1], 2) < 0)
				exit(1);
			if (close(fds[0]) < 0)
				exit(1);
			parse_param(cmd, argv);
			if (execv(argv[0], argv) < 0)
				exit(1);
			exit(1);
		}
		out[read(fds[0], out, 256)] = '\0';
		if (close(fds[0]) < 0) {
			ret = -1001;
			goto cleanup;
		}
		if (close(fds[1]) < 0) {
			ret = -1001;
			goto cleanup;
		}
		if (wait4(pid, (int *)&status, 0, 0) <= 0)
			goto bad;
	}
	if ((status & 0x7f) != 0)
		goto bad;
	if ((status & 0xff00) != 0) {
		ret = -1006;
		goto cleanup;
bad:
		ret = -1007;
		goto cleanup;
	}
	ret = -1006;
	/* NOTE: bug: strchr returns NULL if the output has no colon. */
	if (strncmp(strchr(out, ':'), ": md5 gpg OK", 12) == 0)
		return 0;
cleanup:
	if (fds[0] >= 0)
		close(fds[0]);
	if (fds[1] >= 0)
		close(fds[1]);
	return ret;
}

// execRPMOnBNTmp: in a child process, run "rpm -e" on the package name, or
// "rpm -i" / "rpm -U" on /bntmp/name, as selected by *mode.
int DNAS2Sysupdater::execRPMOnBNTmp(unsigned int len, char *name, unsigned int len2, char *mode)
{
	char buf[272];
	char cmd[320];
	char *argv[64];
	int status;

	if (strncmp(mode, "e", 1) == 0) {
		strncpy(buf, name, len);
		buf[len] = '\0';
		sprintf(cmd, "%s %s", "/bin/rpm -e --nodeps", buf);
	} else if (strncmp(mode, "i", 1) == 0) {
		strcpy(buf, "/bntmp/");
		strncat(buf, name, len);
		sprintf(cmd, "%s %s", "/bin/rpm -i --force --nodeps", buf);
	} else if (strncmp(mode, "U", 1) == 0) {
		strcpy(buf, "/bntmp/");
		strncat(buf, name, len);
		sprintf(cmd, "%s %s", "/bin/rpm -U --force --nodeps", buf);
	} else {
		return -1005;
	}
	int pid = fork();
	if (pid < 0)
		return -1001;
	if (pid == 0) {
		parse_param(cmd, argv);
		if (execv(argv[0], argv) < 0)
			exit(1);
		exit(1);
	}
	if (wait4(pid, &status, 0, 0) <= 0)
		goto bad;
	if ((status & 0x7f) != 0)
		return -1007;
	if ((status & 0xff00) != 0) {
bad:
		return -1007;
	}
	return 0;
}

// copyFileToBNTmpFromPackage: copy size bytes at offset off of the package
// file to /bntmp/name, in 1 KB chunks. Returns 0 or -1004.
int DNAS2Sysupdater::copyFileToBNTmpFromPackage(unsigned long long size, unsigned long long off, unsigned int len,
    char *name)
{
	char buf[272];
	char tmp[1024];
	int fd;
	int out = -1;

	strcpy(buf, "/bntmp/");
	strncat(buf, name, len);
	fd = scepfsOpen((char *)m_manager + 196, 1);
	if (fd < 0)
		goto err;
	out = creat(buf, 0600);
	if (out < 0)
		goto err;
	if (scepfsLseek64(fd, off, 0) < 0)
		goto err;
	{
		unsigned long long rem = size;
		while (rem != 0) {
			unsigned long long chunk = (rem < 1024) ? rem : 1024;
			if ((int)chunk != scepfsRead(fd, tmp, chunk))
				goto err;
			if ((int)chunk != write(out, tmp, chunk))
				goto err;
			rem -= chunk;
		}
	}
	scepfsClose(fd);
	close(out);
	return 0;
err:
	if (fd >= 0)
		scepfsClose(fd);
	if (out >= 0)
		close(out);
	return -1004;
}

// removeFileOnBNTmp: delete /bntmp/name.
int DNAS2Sysupdater::removeFileOnBNTmp(unsigned int len, char *name)
{
	char buf[272];

	strcpy(buf, "/bntmp/");
	strncat(buf, name, len);
	if (remove(buf) < 0)
		return -1004;
	return 0;
}
