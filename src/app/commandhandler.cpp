/*
 * Dispatcher of the application commands ("StartX", "ExecApp") sent to the
 * launcher.
 */

#include "command.h"
#include "variables.h"
#include "thread/thposixthread.h"
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/vt.h>
#include "app/jobinit2.h"

typedef Command::Result (*CommandHandlerFn)(const Command &);

map<string, CommandHandlerFn> command_handler_table;

Command::Result CommandHandler(const Command &cmd)
{
	/* NOTE: bug: an unknown command dereferences end() */
	CommandHandlerFn h = (*command_handler_table.find(cmd.name)).second;

	if (h != 0)
		return h(cmd);
	return Command::Result_noHandler;
}

/* Records the requested launcher mode ("router" or not) and the
   active virtual console. Returns 2 when the "mode" argument is missing. */
Command::Result StartX(const Command &cmd)
{
	string value;

	if (!cmd.GetArg("mode", value))
		return Command::Result_missingArg;

	if (value.compare("router") == 0)
		launcher_mode = 1;
	else
		launcher_mode = 0;
	launcher_id = 0;
	launcher_switching = 1;

	int fd = open("/dev/console", O_RDWR);
	if (fd < 0)
		perror("open error");
	{
		char mode[2];
		/* NOTE: bug: VT_GETSTATE writes a 6-byte struct vt_stat (active console
		   first) into this 2-byte buffer */
		ioctl(fd, VT_GETSTATE, mode);
		launcher_vt = *(unsigned short *)mode;
	}
	close(fd);
	return Command::Result_ok;
}

/* For "RealPlayer", records the application and its argument
   list for the launcher, then the active virtual console. */
Command::Result ExecApp(const Command &cmd)
{
	string appname;
	string arglist;

	if (!cmd.GetArg("appname", appname))
		return Command::Result_missingArg;
	cmd.GetArg("arglist", arglist);
	if (appname.compare("RealPlayer") == 0) {
		launcher_id = 1;
		launcher_switching = 1;
		/* NOTE: bug: the argument list is used as the format string */
		sprintf(launcher_arg, arglist.c_str());
	}
	int fd = open("/dev/console", O_RDWR);
	if (fd < 0)
		perror("open error");
	{
		char mode[2];
		/* NOTE: bug: VT_GETSTATE writes a 6-byte struct vt_stat (active console
		   first) into this 2-byte buffer */
		ioctl(fd, VT_GETSTATE, mode);
		launcher_vt = *(unsigned short *)mode;
	}
	close(fd);
	return Command::Result_ok;
}

/* Registers the handlers at static initialization time. */
class CommandHandlerRegister {
public:
	CommandHandlerRegister()
	{
		command_handler_table.insert(make_pair(string("StartX"), &StartX));
		command_handler_table.insert(make_pair(string("ExecApp"), &ExecApp));
		Command::SetHandler(CommandHandler);
	}
} commandhandlertableinitializer;
