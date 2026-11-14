/*
 * TRY module filesystem factory, and the partition size labels.
 */

#include "trypfsproxy.h"
#include "trycreatefs.h"

/* Factory of the abstract filesystem of the TRY module (C linkage). */
TRYAbstFS *TRYCreateFS(int type)
{
	if (type == 1)
		return new TRYPFSProxy;
	return 0;
}

/* Text label of a partition size by index 1..9; NULL out of range. */
char *try_getStrPatSize(int size)
{
	switch (size) {
	case 1:
		return "128M";
	case 2:
		return "256M";
	case 3:
		return "512M";
	case 4:
		return "1G";
	case 5:
		return "2G";
	case 6:
		return "4G";
	case 7:
		return "8G";
	case 8:
		return "16G";
	case 9:
		return "32G";
	}
	return 0;
}
