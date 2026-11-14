/* GSlib */

/*
 	Copyright (C) 2001  Sony Computer Entertainment Inc.
 
  This file is subject to the terms and conditions of the GNU Library
  General Public License Version 2. See the file "COPYING.LIB" in the 
  main directory of this archive for more details.
*/

/* this source code is derived from ... */
/* VGAlib version 1.2 - (c) 1993 Tommy Frandsen                    */
/*                                                                 */
/* This library is free software; you can redistribute it and/or   */
/* modify it without any restrictions. This library is distributed */
/* in the hope that it will be useful, but without any warranty.   */

/* Multi-chipset support Copyright (C) 1993 Harm Hanemaayer */
/* partially copyrighted (C) 1993 by Hartmut Schirmer */
/* Changes by Michael Weller. */
/* Modified by Don Secrest to include Tseng ET6000 handling */
/* Changes around the config things by 101 (Attila Lendvai) */

#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <signal.h>
#include <termios.h>
#include <string.h>
#include <unistd.h>
#include <stdarg.h>
#include <sys/mman.h>
#include <sys/kd.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/vt.h>
#include <sys/wait.h>
#include <errno.h>
#include <ctype.h>

#include <linux/ps2/dev.h>
#include "gs_vc.h"

#ifndef PS2_GS_VC_SIG
#define PS2_GS_VC_SIG	SIGUSR1
#endif /* PS2_GS_VC_SIG */

int __ps2_gs_vc_tty_fd = -1;	/* /dev/tty file descriptor */
int __ps2_gs_vc_signum = PS2_GS_VC_SIG;
struct vt_mode __ps2_gs_vc_oldvtmode;

int (*__ps2_gs_vc_go_to_background) (void) = 0;
int (*__ps2_gs_vc_come_from_background) (void) = 0;

static int gslib_vc = -1, startup_vc = -1;
static int current_mode = KD_TEXT;
static int my_pid = 0;		/* process PID, used with atexit() */
static int use_vt_process = 0;

/*#define DEBUG */

#ifdef DEBUG
static void _DEBUG(int dnr)
{
    static int first = 1;
    FILE *dfile;

    dfile = fopen("ps2gs_vc.debug", (first ? "w" : "a"));
    first = 0;
    if (dfile == NULL)
	exit(1);
    fprintf(dfile, "debug #%d\n", dnr);
    fclose(dfile);
    sync();
}
#else
#define _DEBUG(d)
#endif

/* The following is rather messy and inelegant. The only solution I can */
/* see is getting a extra free VT for graphics like XFree86 does. */

void __ps2_gs_vc_waitvtactive(void)
{
    if (__ps2_gs_vc_tty_fd < 0)
	return; /* Not yet initialized */

    while (ioctl(__ps2_gs_vc_tty_fd, VT_WAITACTIVE, gslib_vc) < 0) {
	if ((errno != EAGAIN) && (errno != EINTR)) {
	    perror("ioctl(VT_WAITACTIVE)");
	    exit(1);
	}
	usleep(150000);
    }
}

#define ROOT_VC_SHORTCUT

static int check_owner(int vc)
{
    struct stat sbuf;
    char fname[30];

#ifdef ROOT_VC_SHORTCUT
    if (!getuid())
        return 1;               /* root can do it always */
#endif
    sprintf(fname, "/dev/tty%d", vc);
    if ((stat(fname, &sbuf) >= 0) && (getuid() == sbuf.st_uid)) {
        return 1;
    }
    printf("You must be the owner of the current console to use ps2gs_vc.\n");
    return 0;
}

void __ps2_gs_vc_open_devconsole(void)
{
    struct vt_mode vtm;
    struct vt_stat vts;
    struct stat sbuf;
    char fname[30];

    if (__ps2_gs_vc_tty_fd >= 0)
        return;

    /*  The code below assumes file descriptors 0, 1, and 2
     *  are already open; make sure that's true.  */
    if (fcntl(0,F_GETFD) < 0) open("/dev/null", O_RDONLY);
    if (fcntl(1,F_GETFD) < 0) open("/dev/null", O_WRONLY);
    if (fcntl(2,F_GETFD) < 0) open("/dev/null", O_WRONLY);

    /*
     * Now, it would be great if we could use /dev/tty and see what it is connected to.
     * Alas, we cannot find out reliably what VC /dev/tty is bound to. Thus we parse
     * stdin through stderr for a reliable VC
     */
    for (__ps2_gs_vc_tty_fd = 0; __ps2_gs_vc_tty_fd < 3; __ps2_gs_vc_tty_fd++) {
        if (fstat(__ps2_gs_vc_tty_fd, &sbuf) < 0)
            continue;
        if (ioctl(__ps2_gs_vc_tty_fd, VT_GETMODE, &vtm) < 0)
            continue;
        if ((sbuf.st_rdev & 0xff00) != 0x400)
            continue;
        if (!(sbuf.st_rdev & 0xff))
            continue;
        gslib_vc = sbuf.st_rdev & 0xff;
        return;                 /* perfect */
    }

    if ((__ps2_gs_vc_tty_fd = open("/dev/console", O_RDWR)) < 0) {
        printf("ps2gs_vc: can't open /dev/console \n");
        exit(1);
    }
    if (ioctl(__ps2_gs_vc_tty_fd, VT_OPENQRY, &gslib_vc) < 0)
        goto error;
    if (gslib_vc <= 0)
        goto error;
    sprintf(fname, "/dev/tty%d", gslib_vc);
    close(__ps2_gs_vc_tty_fd);
    /* change our control terminal: */
    setpgid(0,getppid());
    setsid();
    /* We must use RDWR to allow for output... */
    if (((__ps2_gs_vc_tty_fd = open(fname, O_RDWR)) >= 0) &&
        (ioctl(__ps2_gs_vc_tty_fd, VT_GETSTATE, &vts) >= 0)) {
        if (!check_owner(vts.v_active))
            goto error;
        /* success, redirect all stdios */
	printf("[ps2gs_vc: allocated virtual console #%d]\n", gslib_vc);
        fflush(stdin);
        fflush(stdout);
        fflush(stderr);
        close(0);
        close(1);
        close(2);
        dup(__ps2_gs_vc_tty_fd);
        dup(__ps2_gs_vc_tty_fd);
        dup(__ps2_gs_vc_tty_fd);
        /* clear screen and switch to it */
        fwrite("\e[H\e[J", 6, 1, stderr);
        fflush(stderr);
        if (gslib_vc != vts.v_active) {
            startup_vc = vts.v_active;
	    ioctl(__ps2_gs_vc_tty_fd, VT_ACTIVATE, gslib_vc);
            __ps2_gs_vc_waitvtactive();
	}
    } else {
error:
    if (__ps2_gs_vc_tty_fd > 2)
	close(__ps2_gs_vc_tty_fd);
    __ps2_gs_vc_tty_fd = - 1;
    printf("Not running in a graphics capable console,\n"
	 "and unable to find one.\n");
    }
}

/* We invoke the old interrupt handler after setting text mode */
/* We catch all signals that cause an exit by default (aka almost all) */
static char sig2catch[] =
{SIGHUP, SIGINT, SIGQUIT, SIGILL,
 SIGTRAP, SIGIOT, SIGBUS, SIGFPE,
 SIGSEGV, SIGPIPE, SIGALRM, SIGTERM,
 SIGXCPU, SIGXFSZ, SIGVTALRM,
/* SIGPROF ,*/ SIGPWR};
static struct sigaction old_signal_handler[sizeof(sig2catch)];

static void signal_handler(int v)
{
    int i;

    ps2_gs_vc_textmode();

    printf("ps2gs_vc: Signal %d: %s received%s.\n", v, strsignal(v),
	   (v == SIGINT) ? " (ctrl-c pressed)" : "");

    for (i = 0; i < sizeof(sig2catch); i++)
	if (sig2catch[i] == v) {
	    raise(v);
	    break;
	}
    if (i >= sizeof(sig2catch)) {
	printf("ps2gs_vc: Aieeee! Illegal call to signal_handler, raising segfault.\n");
	raise(SIGSEGV);
    }
}

static void install_handlers(void)
{
    struct sigaction siga;
    int i;

    /* do our own interrupt handling */
    for (i = 0; i < sizeof(sig2catch); i++) {
	siga.sa_handler = signal_handler;
	siga.sa_flags = 0;
	sigemptyset(&siga.sa_mask);
	sigaction((int)sig2catch[i], &siga, old_signal_handler + i);
    }
}

static void uninstall_handlers(void)
{
    int i;

    for (i = 0; i < sizeof(sig2catch); i++)
	sigaction((int)sig2catch[i], old_signal_handler + i, NULL);
}

static void __gs_atexit(void)
{
    if (getpid() == my_pid) {	/* protect against forked processes */
	ps2_gs_vc_textmode();
    }
}

static void initialize(void)
{
    static int initialized = 0;

    if (initialized)
	return;

    __ps2_gs_vc_open_devconsole();
    if (__ps2_gs_vc_tty_fd < 0)
	exit(1);

    if (my_pid == 0)
	my_pid = getpid();
    atexit(__gs_atexit);

    initialized = 1;
}

static int forbidvtrelease = 0;
static int forbidvtacquire = 0;
static int lock_count = 0;
static int release_flag = 0;
static int acquisition_succeeded = 0;

#define VTSIG_RELEASE	0
#define VTSIG_ACQUIRE	1

static void __ps2_gs_vc_releasevt_signal(int n);
static void __ps2_gs_vc_acquirevt_signal(int n);

static void __ps2_gs_vc_setvtsignal(int mode)
{
    struct sigaction siga;
    struct vt_mode newvtmode;

    newvtmode = __ps2_gs_vc_oldvtmode;
    newvtmode.mode = VT_PROCESS;	/* handle VT changes */
    if (mode == VTSIG_RELEASE) {
	newvtmode.relsig = __ps2_gs_vc_signum;
	newvtmode.acqsig = 0;
	siga.sa_handler = __ps2_gs_vc_releasevt_signal;
    } else {
	newvtmode.relsig = 0;
	newvtmode.acqsig = __ps2_gs_vc_signum;
	siga.sa_handler = __ps2_gs_vc_acquirevt_signal;
    }

    siga.sa_flags = SA_RESTART;
    sigemptyset(&siga.sa_mask);
    sigaction(__ps2_gs_vc_signum, &siga, NULL);
    ioctl(__ps2_gs_vc_tty_fd, VT_SETMODE, &newvtmode);
}

static void __ps2_gs_vc_releasevt_signal(int n)
{
    int relres = PS2_GS_VC_REL_SUCCESS;

    if (lock_count) {
	release_flag = 1;
	return;
    }
#ifdef DEBUG
    printf("Release request.\n");
#endif
    forbidvtacquire = 1;
    if (forbidvtrelease) {
	forbidvtacquire = 0;
	ioctl(__ps2_gs_vc_tty_fd, VT_RELDISP, 0);
	return;
    }
    if (__ps2_gs_vc_go_to_background) {
	relres = (__ps2_gs_vc_go_to_background)();
	if (relres < PS2_GS_VC_REL_SUCCESS) {
	    ioctl(__ps2_gs_vc_tty_fd, VT_RELDISP, 0);
	    return;
	}
    }

    ioctl(__ps2_gs_vc_tty_fd, VT_RELDISP, 1);
#ifdef DEBUG
    printf("Finished release.\n");
#endif
    forbidvtacquire = 0;
    acquisition_succeeded = 0;

    /* Suspend program until switched to again. */
#ifdef DEBUG
    printf("Suspended.\n");
#endif

    __ps2_gs_vc_setvtsignal(VTSIG_ACQUIRE);

    if (relres == PS2_GS_VC_REL_SUCCESS) {
	__ps2_gs_vc_waitvtactive();
	if (!acquisition_succeeded)
	    __ps2_gs_vc_acquirevt_signal(__ps2_gs_vc_signum);

    }

#ifdef DEBUG
    printf("Waked.\n");
#endif
}

static void __ps2_gs_vc_acquirevt_signal(int n)
{
    sigset_t set;

#ifdef DEBUG
    printf("Acquisition request.\n");
#endif
    forbidvtrelease = 1;
    if (forbidvtacquire) {
	forbidvtrelease = 0;
	return;
    }

    __ps2_gs_vc_setvtsignal(VTSIG_RELEASE);
    forbidvtrelease = 0;

    if (__ps2_gs_vc_come_from_background) {
	if ((__ps2_gs_vc_come_from_background)() != PS2_GS_VC_ACQ_SUCCESS) {
#ifdef DEBUG
	    printf("Failed acquisition. Wait.\n");
#endif
	    acquisition_succeeded = 0;
	    /* unblock signal for release */
	    sigemptyset(&set);
	    sigaddset(&set, __ps2_gs_vc_signum);
	    sigprocmask(SIG_UNBLOCK, &set, NULL);
	    while (!acquisition_succeeded)
		pause();
#ifdef DEBUG
	    printf("Succeeded acquisition.\n");
#endif
	}
    }
    acquisition_succeeded = 1;

    ioctl(__ps2_gs_vc_tty_fd, VT_RELDISP, VT_ACKACQ);
#ifdef DEBUG
    printf("Finished acquisition.\n");
#endif
}

int ps2_gs_vc_graphicsmode(void)
{
    if (current_mode == KD_GRAPHICS)
	return -1;

    initialize();

    __ps2_gs_vc_waitvtactive();
    ioctl(__ps2_gs_vc_tty_fd, VT_GETMODE, &__ps2_gs_vc_oldvtmode);
    ioctl(__ps2_gs_vc_tty_fd, KDSETMODE, KD_GRAPHICS);

    if (use_vt_process)
	__ps2_gs_vc_setvtsignal(VTSIG_RELEASE);

    install_handlers();
    current_mode = KD_GRAPHICS;

    return 0;
}

int ps2_gs_vc_textmode(void)
{
    if (__ps2_gs_vc_tty_fd < 0)
	return -1;
    if (current_mode == KD_TEXT)
	return -1;

    if (__ps2_gs_vc_go_to_background)
	(__ps2_gs_vc_go_to_background)();

    ioctl(__ps2_gs_vc_tty_fd, KDSETMODE, KD_TEXT);
    ioctl(__ps2_gs_vc_tty_fd, VT_SETMODE, &__ps2_gs_vc_oldvtmode);
    ioctl(__ps2_gs_vc_tty_fd, VT_ACTIVATE, startup_vc);

    uninstall_handlers();
    current_mode = KD_TEXT;

    return 0;
}

int ps2_gs_vc_enablevcswitch(int (*acqfunc)(void), int (*relfunc)(void))
{
    if (__ps2_gs_vc_tty_fd < 0)
	return -1;

    __ps2_gs_vc_come_from_background = acqfunc;
    __ps2_gs_vc_go_to_background = relfunc;

    if (current_mode == KD_GRAPHICS) {
	if (!use_vt_process)
	    __ps2_gs_vc_setvtsignal(VTSIG_RELEASE);
    }
    use_vt_process = 1;
    return 0;
}

int ps2_gs_vc_disablevcswitch(void)
{
    if (__ps2_gs_vc_tty_fd < 0)
	return -1;

    ioctl(__ps2_gs_vc_tty_fd, VT_SETMODE, &__ps2_gs_vc_oldvtmode);
    use_vt_process = 0;

    __ps2_gs_vc_come_from_background = 0;
    __ps2_gs_vc_go_to_background = 0;

    return 0;
}

void ps2_gs_vc_lock(void)
{
    lock_count++;
    __ps2_gs_vc_waitvtactive();
}

void ps2_gs_vc_unlock(void)
{
    if (--lock_count <= 0) {
        lock_count = 0;
        if (release_flag) {
            release_flag = 0;
            __ps2_gs_vc_releasevt_signal(__ps2_gs_vc_signum);
        }
    }
}
