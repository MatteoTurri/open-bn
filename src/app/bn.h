/*
 * BN: the application class of the Broadband Navigator.
 */

#ifndef BN_H
#define BN_H

#include "desktop.h"

class BN : public Desktop {
public:
	BN(int a, int b);
	virtual ~BN();
};

#endif
