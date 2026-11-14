/*
 * Hook: base of the scene objects, holding user data by id and callback
 * lists by type.
 */

#ifndef HOOK_H
#define HOOK_H

#include <map>
#include <vector>

/* Base class with no polymorphic base: its vptr follows the data, at 0x10. */
class Hook {
public:
	struct CallbackData {
		void (*fn)(Hook *, void *, void *);
		void *data;
		CallbackData() {}
		CallbackData(void (*f)(Hook *, void *, void *), void *d) : fn(f), data(d) {}
	};

	/* 0x00 */ map<unsigned int, void *> user_data;
	/* 0x0C */ vector<CallbackData> *callbacks;  /* array of n callback lists, one per type */

	Hook(int n);
	virtual ~Hook();

	void *GetUserData(unsigned int id);
	/* Sets user data `id` and returns its previous value (0 if none). */
	void *SetUserData(unsigned int id, void *data);
	void *DelUserData(unsigned int id);
	void DoCallback(int type, void *data);
	void SetCallback(int type, void (*fn)(Hook *, void *, void *), void *data);
};

#endif
