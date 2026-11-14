/*
 * Installed applications on the hard disk (bnapp.cpp): uninstall, the
 * application list and the game import thread.
 */

#ifndef BNAPP_H
#define BNAPP_H

int bnDeleteApplication(char *path);
int bnUninstallApplication(char *path);
void bnReadDirent(void);
int bnGameImportStart();
int bnGameImportStat();
int bnGameImportCancel();

#endif
