/*
 * PFS and SMB mount points of the BB file access layer (bbfileaccess.cpp).
 */

#ifndef BBMOUNT_H
#define BBMOUNT_H

#include <string>
#include "bbfstype.h"

int BBpfsMounter(string path);
int BBpfsUnMounter(string path);
int BBpfsUnMount(string path);
int BBpfsAllUnMounter();
int BBpfsDisableMounter(string smbPath);
int BBpfsEnableMounter(string smbPath);
int BBpfsClearDisableMounter();
int BBsmbMounter(string user, string pass);
int BBconvMCPortNo(BBFSType type, int *port, int *slot);

#endif
