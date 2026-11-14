/*
 * Command: a named application command with string arguments, dispatched to
 * a single global handler.
 */

#ifndef COMMAND_H
#define COMMAND_H

#include <string>
#include <map>

class Command {
public:
	/* What a command handler returns. */
	enum Result {
		Result_ok,
		Result_noHandler, /* no handler is registered for the command */
		Result_missingArg /* a required argument is missing */
	};

	Command(const string &n) : name(n) {}
	virtual ~Command() {}
	void SetName(const string &s);
	void SetArg(const string &k, const string &v);
	int GetArg(const string &k, string &out) const;
	Result CallHandler() const;
	static Result (*SetHandler(Result (*fn)(const Command &)))(const Command &);
	static Result (*handler)(const Command &);

	string name;
	map<string, string> args;
};

#endif
