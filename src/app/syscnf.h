/*
 * System configuration (syscnf.cpp): the PS2 system configuration device,
 * system.ini, the remote control settings and the system options.
 */

#ifndef SYSCNF_H
#define SYSCNF_H

int systemini_write(int, void **);
int sysconf_read(int a, void **b);
int syscnf_getRMSysMenu_C();
int syscnf_getRMGA_C();
int syscnf_getRMMenuCMD_C();
int writeDVDProgBit();
void syscnf_initInitMode();
void syscnf_setSysOpt(bool flag);
void syscnf_setAtokMode(int v);
void syscnf_setKbdQwerty(int v);
void syscnf_setKbdOnOff(int v);
int appSysconfRead(bool flag);

#endif
