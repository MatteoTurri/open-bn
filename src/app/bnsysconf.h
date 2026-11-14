/*
 * Enumeration of the "<name>,<value>" files of the sysconf directory
 * (bnsysconf.cpp).
 */

#ifndef BNSYSCONF_H
#define BNSYSCONF_H

typedef int (*SysconfEnumCb)(const char *, const char *, void *);

extern "C" int bnEnumSysconf(const char *key, const char *name, SysconfEnumCb cb, void *ctx);

#endif
