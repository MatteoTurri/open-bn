/*
 * The application entry point: main(), the startup, frame loop and shutdown
 * stages, argument parsing, the launcher of external applications and the
 * signal handlers.
 */

#include <string>
#include <list>
#include <fcntl.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "lib/resuid.h"
#include <pthread.h>
#include <sys/ioctl.h>
#include <linux/kd.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include "thread/thposixthread.h"
#include "fs/bbfile.h"
#include "fs/bbfileaccess.h"
#include "fs/bbmount.h"
#include "natives/xmlextension.h"
#include "ui/navimenu.h"
#include "ui/msgbox.h"
#include "bnexec.h"
#include "input/mypad.h"
#include "input/irrm.h"
#include "app/bn.h"
#include "app/systemoptions.h"
#include "app/variables.h"
#include "syscnf.h"
#include "app/systemparam.h"
#include <sys/stat.h>
#include "fw/fw_pk.h"
#include "gfx/font.h"
#include "gfx/gm_graph.h"
#include "scene/scenefacade.h"
#include "fw/fw.h"
#include "fw/fw_gsdev.h"
#include "fw/fw_ev.h"
#include "fw/gsbuf.h"
#include "lib/ccc.h"
#include "script/scenewindow.h"
#include "natives/sndextension.h"
#include "gfx/bt2ipu.h"
#include "resource.h"
#include "fw/dmem.h"
#include "lib/mymalloc.h"
#include "lib/mymalloc2.h"
#include "gfx/texture.h"
#include "image/thimagedecoder.h"
#include "scene/objecttable.h"
#include "xml/xmlparse_ns.h"
#include "app/module.h"
#include "script/script.h"
#include "ime/textim.h"
#include "script/scriptabort.h"
#include "http/httpfile.h"
#include "fs/thmediachecker.h"
#include "biff.h"
#include "xml/xml_filedescriptor.h"
#include "app/buffer.h"
#include "script/thxmlloader.h"
#include "thread/thjob.h"
#include "ime/conkbd.h"
#include <signal.h>

class thJob;

static void appPreInit(int mode);
static void appParseArgs(int argc, char **argv);
static int appInitInterupt();
static void appStartup();
static void appBootUi();
static void appFrameLoop();
static void appFadeOut();
static void appShutdown();
static void appStartMediaChecker(thJob *job);
static void checkNetUp();
static void appCrtBlendReset();
static void appFontRun();
static void appGraphRun();

myPad *pPad0;
myPad *pPad1;
IRrm *pRM0;

static BN *s_bn;
static void *s_dmamemAux;
void *dmamem;
class thMediaChecker;
static thMediaChecker *s_mediaCheckerObj;
int running_status;
/* Launcher state; launcher_arg holds the arguments of the next application. */
int launcher_switching;
int launcher_mode;
int launcher_id;
int launcher_vt;
char launcher_arg[1024];
int exit_flag;

static int s_bootRetry;

/* Pad-to-key binding tables: zero-terminated lists of key codes. */
/* clang-format off */
unsigned short gBindKeytableDefault[] = {
	0x121, 0, 0x122, 0, 0x123, 0,
	0x165, 0, 0x168, 0, 0x167, 0, 0x166, 0,
	0x125, 0, 0x128, 0,
	0x12c, 0, 0x12b, 0, 0x12a, 0,
	0x02b, 0x42b, 0, 0x11b, 0, 0x10d, 0x00d, 0, 0x02d, 0,
};

unsigned short gBindKeytableDisableLnRn[] = {
	0, 0, 0,
	0x165, 0, 0x168, 0, 0x167, 0, 0x166, 0,
	0x125, 0, 0x128, 0,
	0, 0, 0,
	0x02b, 0x42b, 0, 0x11b, 0, 0x10d, 0x00d, 0, 0x02d, 0,
};

unsigned short gBindKeytableForMailer[] = {
	0x121, 0x162, 0, 0x122, 0, 0x123, 0,
	0x165, 0, 0x168, 0, 0x167, 0, 0x166, 0,
	0x125, 0, 0x128, 0,
	0x12c, 0x161, 0, 0x12b, 0, 0x12a, 0,
	0x02b, 0x42b, 0, 0x11b, 0, 0x10d, 0x00d, 0, 0x02d, 0,
};
/* clang-format on */

int gs_crtc_mode = 0;

#include "jobinit2.h"

static int dropToUid(int suid);

gm_graph g_graph;

static void quit_handler(int sig);
static void int_handler(int sig);
static void abrt_handler(int sig);
static void alrm_handler(int sig);
static void tstp_handler(int sig);
static void term_handler(int sig);

/* Clears the screen and hides the cursor on /dev/console and sets its mode
   (KDSETMODE: graphics when mode == 0, text otherwise), then does the same on
   /dev/tty<launcher_vt>. */
static void appPreInit(int mode)
{
	char tty[16];
	int fd;
	FILE *fp;

	fd = open("/dev/console", O_RDWR);
	if (fd >= 0) {
		fp = fdopen(fd, "wb");
		fprintf(fp, "\x1b[2J");
		fprintf(fp, "\x1b[?25l");
		if (mode == 0)
			ioctl(fd, KDSETMODE, KD_GRAPHICS);
		else
			ioctl(fd, KDSETMODE, KD_TEXT);
		close(fd);
	}
	sprintf(tty, "/dev/tty%d", launcher_vt);
	fp = fopen(tty, "wb");
	if (fp) {
		fprintf(fp, "\x1b[2J");
		fprintf(fp, "\x1b[?25l");
		fclose(fp);
	}
}

/* System initialization, then launcher() runs again for as long as
   something sets launcher_switching back to 1; finally the console reboots. */
int main(int argc, char **argv)
{
	struct rlimit core;

	core.rlim_cur = 0;
	core.rlim_max = 0;
	setrlimit(RLIMIT_CORE, &core);
	appPreInit(0);
	appParseArgs(argc, argv);
	appInitInterupt();
	Variables::Initialize();

	exit_flag = 0;
	launcher_switching = 1;
	launcher_id = -1;
	launcher_vt = -1;
	memset(&launcher_arg, 0, 1024);

	while (launcher_switching) {
		launcher_id = -1;
		launcher_switching = 0;
		appStartup();
		appBootUi();
		appFrameLoop();
		appFadeOut();
		appShutdown();
		launcher();
	}

	bnReboot();
	return 0;
}

/* Starts the subsystems before the frame loop: pid file, sound, file
   system, the graphics chain, pads and remote control, tables, parser,
   modules, scripts, windows, resources, sysconf, TextIM, errors, BN,
   NaviMenu, MsgBox and the HttpFile cache. */
static void appStartup()
{
	FILE *fp;

	InitInterupt();
	fp = fopen("/var/run/bn.pid", "wb");
	if (fp) {
		fprintf(fp, "%d", getpid());
		fclose(fp);
	}
	SndExtension_Init();
	BBFileAccessInitCPP();
	if (SysOption.getNewConsoleMode())
		fwVcInitEnable(1);

	DMemInit();
	s_dmamemAux = DMemAlloc(0x80000);
	fwPkInitChainBuf(s_dmamemAux, 0x80000, 1);
	fwEvStartThread();
	fwEvRegistHandle(PS2EV_N_VBSTART, 0, 0);
	fwEvRegistHandle(PS2EV_N_FINISH, 0, 0);
	if (SysOption.getXGAMode()) {
		fwGsInitAll(FW_GS_VESA_1024x768);
		GsBufInit(GSBUF_1024x768);
	} else if (SysOption.getSPECIALMode()) {
		fwGsInitAll(FW_GS_VESA_1024x768);
		GsBufInit(GSBUF_896x512);
	} else {
		if (gs_crtc_mode == 0)
			fwGsInitAll(FW_GS_RESET_GIF | FW_GS_NTSC_FIELD);
		else
			fwGsInitAll(FW_GS_NTSC_FIELD);
		GsBufInit(GSBUF_640x448);
	}
	dmamem = DMemAlloc(0x780000);
	myMalloc_InitHeap(dmamem, 0x600000);
	myMalloc2_InitHeap((char *)dmamem + 0x600000, 0x180000);
	Texture::Init();
	fwEvRegistCbfunc(PS2EV_N_VBSTART, (int)fwGsFlipRegDBuf_cb);
	fwGsStartDisplay(fbuf0, fbuf1, 1);
	Font::Init();
	thImageDecoder::Init();
	bt2IpuInit();
	if (!g_graph.Init(0x90000, 0x200000))
		return;
	g_graph.SetTextureManager(Texture::Manager());
	SceneInit(g_graph);
	SceneSetFrameBuffer(0, fbuf0);
	SceneSetFrameBuffer(1, fbuf1);
	SceneSetDepthBuffer(0, zbuf0);
	SceneSetDepthBuffer(1, zbuf1);

	myPad::Init();
	pPad0 = new myPad(0, 0);
	pPad1 = new myPad(1, 0);
	pRM0 = new IRrm;
	if (((IRrm *)pRM0)->GetStat() < 0) {
		delete (IRrm *)pRM0;
		pRM0 = 0;
	}

	ObjectTable::Init();
	ModelTable::Init();
	XmlParse::Init();
	Module::Init();
	Script::Init();
	SceneWindow::Init();
	LoadResource();
	appSysconfRead(1);
	TextIM::Init();
	TextIM::Shutdown();
	TextIM::Init();
	Error::Init();
	s_bn = new BN(SysOption.getResWidth(), SysOption.getResHeight());
	NaviMenu::Init();
	MsgBox::Init();
	HttpFile::Init(string("/opt0/bn/cache"));
}

/* Startup job callback: starts the media checker thread, then checks the
   network and starts Biff. */
static void appStartMediaChecker(thJob *job)
{
	thMediaChecker *mc;

	(void)job;
	appSysconfRead(true);
	mc = new thMediaChecker;
	s_mediaCheckerObj = mc;
	mc->start();
	s_mediaCheckerObj->release();
	checkNetUp();
	Biff::Init(0);
}

static bool bootRetryReset(thJob *job, void *arg)
{
	s_bootRetry = 0;
	return true;
}

/* Creates pfs:__net/etc/bnexe if missing (and unmounts __net), then reloads
   the replace words, NaviMenu and MsgBox. */
static void job_init_impl(thJob *job)
{
	struct stat st;

	(void)job;
	if (BBlstatCPP("pfs:__net/etc/bnexe", &st) < 0) {
		int fd = BBopenCPP("pfs:__net/etc/bnexe", 257, 436);
		if (fd >= 0) {
			BBcloseCPP(fd);
			BBpfsUnMounter("__net");
		}
	}
	XmlExtension_UpdateReplaceWord();
	NaviMenu::Load();
	MsgBox::Load();
}

void jobInit2(thJob *job)
{
	job_init_impl(job);
}

/* User interface boot: loads fg.xml, runs the startup job with a reduced
   frame loop until it ends, creates /opt0/bn/sysconf/ if missing and loads
   init_setup.xml. */
static void appBootUi()
{
	thXmlLoader *fg = new thXmlLoader(string("file:/opt0/bn/script/system/fg.xml"), "", "", SceneWindow::MODE_FG, 0,
	    0, 0, Buffer());
	fg->start();
	pthread_join(fg->m_tid, 0);
	fg->release();

	s_bootRetry = 1;
	thJob *job = new thJob(0, appStartMediaChecker, bootRetryReset, 0);
	job->start();
	job->release();
	running_status = 1;
	while (s_bootRetry) {
		thPosixThread::checkThread();
		appCrtBlendReset();
		Script::RunAll();
		SceneWindow::AnimAll();
		SceneWindow::DrawAll();
		appGraphRun();
		appFontRun();
	}

	struct stat st;
	if (BBlstatCPP("/opt0/bn/sysconf/", &st) < 0)
		BBmkdirCPP("/opt0/bn/sysconf/", 0777);
	syscnf_initInitMode();

	string url, post, arg;
	url = "file:/opt0/bn/script/utility/init_setup.xml";
	post = "";
	arg = "mode=START";
	thXmlLoader *setup = new thXmlLoader(url, post, arg, SceneWindow::MODE_MAIN, 0, 0, 0, Buffer());
	setup->start();
	pthread_join(setup->m_tid, 0);
	setup->release();
}

/* network_up tells whether /opt0/bn/sysconf/netup.dat exists. */
static void checkNetUp()
{
	struct stat st;

	if (lstat("/opt0/bn/sysconf/netup.dat", &st) == 0)
		SystemParam::network_up = 1;
	else
		SystemParam::network_up = 0;
}

/* Shutdown sequence: exits every subsystem, then deletes the BN object,
   the pads and the remote control. */
static void appShutdown()
{
	SceneWindow::Exit();
	Script::RemoveAll();
	fwGsRegistDBuf(-1, -1, 1);
	fwPkSyncV();
	fwEvReleaseCbfunc(PS2EV_N_VBSTART, (int)fwGsFlipRegDBuf_cb);
	fwEvReleaseHandle(PS2EV_N_VBSTART);
	fwEvReleaseHandle(PS2EV_N_FINISH);
	fwEvFinishThread();
	Error::Exit();
	Biff::Term();
	NaviMenu::Exit();
	MsgBox::Exit();
	TextIM::Shutdown();
	ReleaseResource();
	Script::Exit();
	Module::Exit();
	XmlParse::Exit();
	ObjectTable::Exit();
	ModelTable::Exit();
	HttpFile::Shutdown();
	delete s_bn;
	BBFileAccessExitCPP();
	SndExtension_Exit();
	SceneTerminate(g_graph);
	g_graph.Term();
	thImageDecoder::Exit();
	bt2IpuTerm();
	Texture::Exit();
	Font::Exit();
	Texture::Exit2();
	fwGsReleaseAll();
	DMemFree(dmamem);
	DMemFree(s_dmamemAux);
	DMemFinish();
	if (pRM0 != 0)
		delete pRM0;
	if (pPad0 != 0)
		delete pPad0;
	if (pPad1 != 0)
		delete pPad1;
}

/* The frame loop: runs while running_status is set, that is until
   launcher_switching or exit_flag clears it. */
static void appFrameLoop()
{
	running_status = 1;
	do {
		unsigned short code;
		unsigned short arg;
		int state;

		thPosixThread::checkThread();
		appCrtBlendReset();

		if (pRM0 != 0)
			pRM0->GetSignal();
		code = 0;
		if (pRM0 != 0)
			code = pRM0->GetPadCode();
		arg = code | myUsbKeyboard_LastPseudoPadData;
		if (pPad0 != 0)
			pPad0->GetData(arg);
		if (pPad1 != 0)
			pPad1->GetData(0);

		state = 0;
		if (pPad0 != 0)
			state = pPad0->GetFlags();
		TextIM::Pad(pPad1 ? state | pPad1->GetFlags() : state);

		Biff::Run();
		HttpFile::Sync();
		Error::Run();
		Script::RunAll();
		SceneWindow::AnimAll();
		SceneWindow::DrawAll();
		TextIM::Draw();
		appGraphRun();
		appFontRun();

		if (launcher_switching != 0 || exit_flag != 0)
			running_status = 0;
	} while (running_status != 0);
}

/* For launcher_id 0 or 1: fades the screen out over 16 frames, then
   detaches the blend buffer. */
static void appFadeOut()
{
	int alpha;
	int i;

	if (launcher_id != 0 && launcher_id != 1)
		return;
	alpha = 255;
	i = 15;
	fwGsRegistCrtBlendBuf(fbuf0, fbuf1);
	do {
		fwGsSetCrtBlendAlpha(fbuf0, alpha, 1);
		fwGsSetCrtBlendAlpha(fbuf1, alpha, 1);
		fwGsSetCrtBlendBgColor(fbuf0, 0, 1);
		fwGsSetCrtBlendBgColor(fbuf1, 0, 1);
		fwPkSyncV();
		SceneWindow::DrawAll();
		appGraphRun();
		fwPkWaitDma();
		i--;
		alpha -= 16;
	} while (i >= 0);
	fwPkSyncV();
	fwPkWaitDma();
	fwPkSyncV();
	fwPkWaitDma();
	fwGsSetCrtBlendAlpha(fbuf0, 255, 0);
	fwGsSetCrtBlendAlpha(fbuf1, 255, 0);
	fwGsSetCrtBlendBgColor(fbuf0, 0, 0);
	fwGsSetCrtBlendBgColor(fbuf1, 0, 0);
	fwGsRegistCrtBlendBuf(-1, -1);
	fwPkSyncV();
}

static void appCrtBlendReset()
{
	fwGsRegistCrtBlendBuf(-1, -1);
	fwGsSetCrtBlendAlpha(fbuf0, 0, 0);
	fwGsSetCrtBlendAlpha(fbuf1, 0, 0);
	fwGsSetCrtBlendBgColor(fbuf0, 0, 0);
	fwGsSetCrtBlendBgColor(fbuf1, 0, 0);
	fwPkSyncV();
	fwEvSetSyncCount(PS2EV_N_VBSTART, 1);
}

static void appFontRun()
{
	fwPkWaitDma();
	Font::Run();
}

static void appGraphRun()
{
	g_graph.SwapBuffers();
	SceneRun(g_graph);
	g_graph.GetPacketFailCount();
}

static struct option long_options[] = {
	{ "help", 0, 0, 'h' },
	{ "xga", 0, 0, 'x' },
	{ "special", 0, 0, 's' },
	{ "newconsole", 0, 0, 'n' },
	{ "clear", 0, 0, 'c' },
	{ 0, 0, 0, 0 },
};

/* getopt_long on "xdsnhc" plus five long options; any other option prints
   the usage and exits. */
static void appParseArgs(int argc, char **argv)
{
	int option_index;
	int c;

	while ((c = getopt_long(argc, argv, "xdsnhc", long_options, &option_index)) != -1) {
		switch (c) {
		case 'x':
			SysOption.setXGAMode();
			break;
		case 's':
			SysOption.setSPECIALMode();
			break;
		case 'n':
			SysOption.setNewConsoleMode();
			break;
		case 'c':
			gs_crtc_mode = 4;
			break;
		case 'h':
		case 'd':
		default:
			fprintf(stderr, " USAGE : sample [option]\n");
			fprintf(stderr, "   --debug or -d   ScriptDebugger on\n");
			fprintf(stderr, "   --xga or -x   XGA mode\n");
			fprintf(stderr, "   --newconsole or -n   New Console mode\n");
			exit(0);
		}
	}
}

/* Runs `cmd` with argument `arg` in a child process and waits for it;
   exec_script runs it through /bin/sh. */
int exec_binary(char *cmd, char *arg)
{
	char *argv[4];
	int status;
	int pid;

	pid = fork();
	if (pid < 0) {
		fprintf(stderr, "%s: fork() error\n", "exec_binary");
		return 10;
	}
	if (pid == 0) {
		argv[0] = cmd;
		argv[1] = arg;
		argv[2] = 0;
		execvp(cmd, argv);
		fprintf(stderr, "%s: execvp() error.\n", "exec_binary");
		return 10;
	}
	while (waitpid(pid, &status, WNOHANG) != pid)
		;
	{
		int r;
		if (WIFEXITED(status))
			r = WEXITSTATUS(status);
		else
			r = 10;
		status = r;
		return r;
	}
}

int exec_script(char *cmd, char *arg)
{
	char *argv[4];
	int status;
	int pid;

	pid = fork();
	if (pid < 0) {
		fprintf(stderr, "%s: fork() error\n", "exec_script");
		return 10;
	}
	if (pid == 0) {
		char *sh = (char *)"/bin/sh";
		argv[0] = sh;
		argv[1] = cmd;
		argv[2] = arg;
		argv[3] = 0;
		execvp(sh, argv);
		fprintf(stderr, "%s: execvp() error.\n", "exec_script");
		return 10;
	}
	while (waitpid(pid, &status, WNOHANG) != pid)
		;
	{
		int r;
		if (WIFEXITED(status))
			r = WEXITSTATUS(status);
		else
			r = 10;
		status = r;
		return r;
	}
}

/* Runs the application selected by launcher_id: 0 (default) starts X
   (bnstartx) in a child process, then stops IIim/atokx and rereads the ATOK
   input mode; 1 runs RealPlayer; 2 runs "retnative". */
void launcher()
{
	int fd[2];
	string cmd;

	if (launcher_switching == 0)
		return;

	gs_crtc_mode = 0;
	appPreInit(1);

	switch (launcher_id) {
	case 0:
	default: {
		string sec;
		int status;
		int pid;
		int mode = launcher_mode;

		pid = fork();
		if (pid != -1) {
			if (pid == 0) {
				int lfd;
				int mouseSpeed;
				int mouseChangeLR;

				lfd = open("/var/lock/console/bn", O_RDWR);
				if (lfd >= 0)
					close(lfd);
				dropToUid(500);
				setenv("HOME", "/net/home/bn", 1);
				if (chdir("/net/home/bn/") != 0)
					exit(-1);

				if (SysOption.m_atokMode == 0)
					exec_binary((char *)"/opt0/bn/xwindow/tools/setinputmode", (char *)"1");
				else if (SysOption.m_atokMode == 1)
					exec_binary((char *)"/opt0/bn/xwindow/tools/setinputmode", (char *)"2");

				if (SysOption.m_atokKeyBind == 1) {
					exec_binary((char *)"/opt0/bn/xwindow/tools/setimtype", (char *)"2");
					fprintf(stderr, "OSD_ATOKKB_ATOK\n");
				} else if (SysOption.m_atokKeyBind == 0) {
					exec_binary((char *)"/opt0/bn/xwindow/tools/setimtype", (char *)"1");
					fprintf(stderr, "OSD_ATOKKB_IME\n");
				} else {
					fprintf(stderr, "ATOK KEY BIND CHANGE : ERROR\n");
				}

				mouseChangeLR = SysOption.getMouseChangeLR();
				mouseSpeed = SysOption.getMouseSpeed();

				{
					string a("1000");
					string b("50");
					string c("0");
					if (mouseChangeLR == 0)
						c = "1";
					string d("1,0");
					switch (mouseSpeed) {
					case 0:
						d = "5/2,10";
						break;
					case 2:
						d = "1/3,0";
						break;
					}

					if (mode == 0) {
						fd[0] = open("/dev/null", O_RDWR);
						close(STDOUT_FILENO);
						dup(fd[0]);
						close(fd[0]);
						execlp("/opt0/bn/xwindow/tools/bnstartx",
						    "/opt0/bn/xwindow/tools/bnstartx", c.c_str(), a.c_str(), d.c_str(),
						    b.c_str(), "--", "-screen", "0", "VESA,1024x768x24",
						    "-initial_fading", "0", 0);
					} else {
						int rfd = open("/tmp/router", O_RDWR);
						if (rfd >= 0)
							close(rfd);
						dropToUid(500);
						fd[0] = open("/dev/null", O_RDWR);
						close(STDOUT_FILENO);
						dup(fd[0]);
						close(fd[0]);
						nice(20);
						execlp("/opt0/bn/xwindow/tools/bnstartx",
						    "/opt0/bn/xwindow/tools/bnstartx", c.c_str(), a.c_str(), d.c_str(),
						    b.c_str(), "--", "-x_margin 2", "-y_margin 5", "-screen", "0",
						    "ntsc", "-initial_fading", "0", "-interlace-mix 55", 0);
					}
				}
				exit(-1);
			}

			while (wait(&status) != pid)
				;
			unlink("/tmp/router");
			unlink("/var/lock/console/bn");
		}
	}
		fprintf(stderr, " --------------- IIim/atok stop\n");
		exec_script((char *)"/etc/rc.d/init.d/IIim", (char *)"stop");
		exec_script((char *)"/etc/rc.d/init.d/atokx", (char *)"stop");
		fprintf(stderr, " --------------- IIim/atok stop finish\n");
		BBFileAccessInitCPP();
		{
			int st2 = 0;
			int pid2 = fork();
			if (pid2 != -1) {
				if (pid2 == 0) {
					execlp("/opt0/bn/xwindow/tools/setinputmode",
					    "/opt0/bn/xwindow/tools/setinputmode", "0", 0);
					exit(-1);
				}
				while (wait(&st2) != pid2)
					;
				fprintf(stderr, "ATOK INPUT MODE SAVE = %d\n", WEXITSTATUS(st2));
				int atok = WEXITSTATUS(st2) == 2;
				sysconf_read(0, 0);
				syscnf_setAtokMode(atok);
				systemini_write(0, 0);
			}
		}
		BBFileAccessExitCPP();
		break;

	case 1: {
		int status;
		int pid;

		cmd = "/opt0/Real/RealPlayer/launchrp";
		pid = fork();
		if (pid == -1)
			break;
		if (pid == 0) {
			chdir("/opt0/Real/RealPlayer");
			setenv("ENABLE_PFS_TIMING", "enable", 1);
			fd[0] = open("/dev/null", O_RDWR);
			close(STDOUT_FILENO);
			dup(fd[0]);
			close(fd[0]);
			fd[1] = open("/dev/null", O_RDWR);
			close(STDERR_FILENO);
			dup(fd[1]);
			close(fd[1]);
			nice(20);
			execl(cmd.c_str(), "RealPlayer", launcher_arg, 0);
			exit(-1);
		}
		nice(10);
		while (wait(&status) != pid)
			;
		nice(0);
		memset(launcher_arg, 0, 1024);
	} break;

	case 2:
		system("retnative");
		break;
	}
	appPreInit(0);
}

static int dropToUid(int suid)
{
	return setresuid(0, 500, suid);
}

/* Loads the JIS/UCS conversion tables of sceCcc (128 KB each). */
static int appInitInterupt()
{
	static char s_jis2ucs[0x20000];
	static char s_ucs2jis[0x20000];
	int jisFd;
	int ucsFd;

	jisFd = open("/opt0/bn/data/tbl/jis2ucs.bin", O_RDONLY);
	if (jisFd < 0)
		return -1;
	read(jisFd, s_jis2ucs, 0x20000);
	close(jisFd);

	ucsFd = open("/opt0/bn/data/tbl/ucs2jis.bin", O_RDONLY);
	if (ucsFd < 0)
		return -1;
	read(ucsFd, s_ucs2jis, 0x20000);
	close(ucsFd);

	sceCccSetTable(s_jis2ucs, s_ucs2jis);
	sceCccSetErrorCharSJIS(0x2121);
	sceCccSetErrorCharUTF8(0x3000);
	sceCccSetErrorCharUTF16(0x3000);
	return 0;
}

/* Installs the signal handlers, except for signals that are ignored. Each
   handler just reinstalls itself; SIGTERM also deletes the pads. */
void InitInterupt(void)
{
	if (signal(SIGQUIT, SIG_IGN) != SIG_IGN)
		signal(SIGQUIT, quit_handler);
	if (signal(SIGINT, SIG_IGN) != SIG_IGN)
		signal(SIGINT, int_handler);
	if (signal(SIGABRT, SIG_IGN) != SIG_IGN)
		signal(SIGABRT, abrt_handler);
	if (signal(SIGALRM, SIG_IGN) != SIG_IGN)
		signal(SIGALRM, alrm_handler);
	if (signal(SIGTSTP, SIG_IGN) != SIG_IGN)
		signal(SIGTSTP, tstp_handler);
	if (signal(SIGTERM, SIG_IGN) != SIG_IGN)
		signal(SIGTERM, term_handler);
}

static void quit_handler(int sig)
{
	signal(sig, SIG_IGN);
	signal(sig, quit_handler);
}

static void int_handler(int sig)
{
	signal(sig, SIG_IGN);
	signal(sig, int_handler);
}

static void abrt_handler(int sig)
{
	signal(sig, SIG_IGN);
	signal(sig, abrt_handler);
}

static void alrm_handler(int sig)
{
	signal(sig, SIG_IGN);
	signal(sig, alrm_handler);
}

static void tstp_handler(int sig)
{
	signal(sig, SIG_IGN);
	signal(sig, tstp_handler);
}

static void term_handler(int sig)
{
	if (pPad0)
		delete pPad0;
	if (pPad1)
		delete pPad1;
	pPad0 = 0;
	pPad1 = 0;
	signal(sig, SIG_IGN);
	signal(sig, term_handler);
}
