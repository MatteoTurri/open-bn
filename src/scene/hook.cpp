/*
 * Hook: base of the scene objects, holding user data by id and callback
 * lists by type.
 */

#include "scene/hook.h"

Hook::Hook(int n)
{
	if (n > 0)
		callbacks = new vector<CallbackData>[n];
	else
		callbacks = 0;
}

Hook::~Hook()
{
	delete[] callbacks;
}

void *Hook::GetUserData(unsigned int id)
{
	map<unsigned int, void *>::const_iterator it = user_data.find(id);
	if (it != user_data.end())
		return it->second;
	return 0;
}

void *Hook::SetUserData(unsigned int id, void *data)
{
	map<unsigned int, void *>::const_iterator it = user_data.find(id);
	void *old = (it != user_data.end()) ? it->second : 0;

	user_data[id] = data;
	return old;
}

void *Hook::DelUserData(unsigned int id)
{
	map<unsigned int, void *>::iterator it = user_data.find(id);
	if (it != user_data.end()) {
		void *old = (*it).second;
		user_data.erase(it);
		return old;
	}
	return 0;
}

void Hook::SetCallback(int type, void (*fn)(Hook *, void *, void *), void *data)
{
	callbacks[type].push_back(CallbackData(fn, data));
}

void Hook::DoCallback(int type, void *data)
{
	CallbackData *it = callbacks[type].begin();
	while (it != callbacks[type].end()) {
		it->fn(this, it->data, data);
		it++;
	}
}
