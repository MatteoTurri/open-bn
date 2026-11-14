/*
 * MCSaveProperty: properties of a memory card save, filled by
 * thFileOperation::getproperty.
 */

#ifndef MCSAVEPROPERTY_H
#define MCSAVEPROPERTY_H

#include <string>

struct MCSaveProperty {
	/* 0x00 */ string name;
	/* 0x04 */ short titleBreakOffset;   /* title wrap column * 2, 0 = automatic */
	/* 0x08 */ int unk_08;
	/* 0x0C */ int damaged;
}; /* size = 0x10 */

#endif
