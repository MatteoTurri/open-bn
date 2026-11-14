/*
 * ATOK engine client (atok.c): loads the conversion engine and exchanges messages with it for
 * the front-end processor.
 */
#ifndef ATOK_H
#define ATOK_H

int atok_sendmsg(int type, int code);
int atok_set_inputmode(int mode, int sub);
int atok_set_kanamode(int mode);
int atok_open(void);
int atok_init(int mode, int speed);
void atok_close(void);
void atok_shutdown(void);

#endif
