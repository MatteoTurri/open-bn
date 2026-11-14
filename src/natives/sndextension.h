/*
 * sndextension.cpp: SndExtension, the sound natives.
 */

#ifndef SNDEXTENSION_H
#define SNDEXTENSION_H

void SndExtension_Init();
void SndExtension_Exit();

extern int g_dspFd;   /* file descriptor of /dev/dsp2, -1 while closed */

#endif
