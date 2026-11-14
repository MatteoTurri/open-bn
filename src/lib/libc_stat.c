/*
 * glibc stat(2) wrapper of the PS2 Linux Kit, linked statically: it forwards
 * to __xstat with the struct stat version _STAT_VER in front.
 */


#include <sys/stat.h>

#undef stat
/* only the compiler diagnostics see this line number */
#line 24
int __stat(const char *file, struct stat *buf)
{
	return __xstat(_STAT_VER, file, buf);
}


extern __typeof(__stat) stat __attribute__((weak, alias("__stat")));
