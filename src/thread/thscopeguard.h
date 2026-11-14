/*
 * thScopeGuard: scope object whose constructor and destructor are empty in
 * this build.
 */

#ifndef TH_SCOPEGUARD_H
#define TH_SCOPEGUARD_H

class thScopeGuard {
public:
	thScopeGuard() {}
	~thScopeGuard() {}
};

#endif /* TH_SCOPEGUARD_H */
