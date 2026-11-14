/*
 * _command_info: a command queued to a worker thread (thDBAccess,
 * thDLModule): an opcode and its string parameter.
 */

#ifndef COMMANDINFO_H
#define COMMANDINFO_H

#include <string>

struct _command_info {
	/* 0x00 */ int cmd;
	/* 0x04 */ string param;

	~_command_info() {}
};

#endif
