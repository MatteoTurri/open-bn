/*
 * glibc lstat(2) wrapper of the PS2 Linux Kit, linked statically: it forwards
 * to __lxstat with the struct stat version _STAT_VER in front.
 */


#include <sys/stat.h>

#undef lstat
/* only the compiler diagnostics see this line number */
#line 29
int __lstat(const char *file, struct stat *buf)
{
	return __lxstat(_STAT_VER, file, buf);
}


extern __typeof(__lstat) lstat __attribute__((weak, alias("__lstat")));
