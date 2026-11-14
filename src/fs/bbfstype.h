/*
 * BBFSType: the device types of the BB file layer.
 */

#ifndef BBFSTYPE_H
#define BBFSTYPE_H

// Device types: the `type` column of bbfstbl in bburl.cpp.
enum BBFSType {
	BBFSTYPE_LINUX = 1,
	BBFSTYPE_PFS = 2,
	BBFSTYPE_MC00 = 3,
	BBFSTYPE_MC01 = 4,
	BBFSTYPE_MC02 = 5,
	BBFSTYPE_MC03 = 6,
	BBFSTYPE_MC10 = 7,
	BBFSTYPE_MC11 = 8,
	BBFSTYPE_MC12 = 9,
	BBFSTYPE_MC13 = 10,
	BBFSTYPE_PS1VMC00 = 11,
	BBFSTYPE_PS1VMC10 = 12,
	BBFSTYPE_SMB = 13
};

#endif
