/*
 * Sound (sound.c): system sound start/stop and the midi* API for the BGM and
 * SE banks.
 */

#ifndef SOUND_H
#define SOUND_H

#ifdef __cplusplus
extern "C" {
#endif

int soundstart(void);
int soundinit(void);
int soundstop(void);
int soundquit(void);
void soundStart(void);
void soundStop(void);
int midiInitBGM(const char *vbfile, const char *hdfile, const char *sqfile);
int midiInitSE(const char *vbfile, const char *hdfile);
int midiInitBGMPtr(const char *vbptr, unsigned int vbsize, const void *hdptr, int hdsize, const void *sqptr,
    int sqsize);
int midiInitSEPtr(const char *vbptr, unsigned int vbsize, const void *hdptr, int hdsize);
int midiQuitBGM(void);
int midiQuitSE(void);
int midiSePlay(int note, int vel);
int midiSetMaxNote(int bank, int n);
int SetSeVolume(int vol);

extern int gDataTopAddrForCslMidi;   /* IOP heap handle of the BGM bank, -1 = none */

#ifdef __cplusplus
}
#endif

#endif
