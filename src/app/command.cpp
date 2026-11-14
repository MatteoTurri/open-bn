/*
 * Out-of-line methods of Command. The handler is a static member: SetHandler
 * installs it and returns the previous one, CallHandler passes it *this.
 */

#include "command.h"

void Command::SetName(const string &s)
{
	name = s;
}

void Command::SetArg(const string &k, const string &v)
{
	args[k] = v;
}

int Command::GetArg(const string &k, string &out) const
{
	map<string, string>::const_iterator it = args.find(k);

	if (it != args.end()) {
		out = (*it).second;
		return 1;
	}
	out.erase(out.begin(), out.end());
	return 0;
}

Command::Result (*Command::handler)(const Command &) = 0;

Command::Result Command::CallHandler() const
{
	return handler(*this);
}

Command::Result (*Command::SetHandler(Result (*fn)(const Command &)))(const Command &)
{
	Result (*old)(const Command &) = handler;
	handler = fn;
	return old;
}
