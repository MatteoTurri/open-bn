/*
 * The surrender hook of RCOM: lets the caller of a long operation ask it to give up.
 */

#ifndef RCOM_CHECKSURRENDER_H
#define RCOM_CHECKSURRENDER_H

struct SurrenderHook {
	/* 0x00 */ int (*fn)(void *arg);
	/* 0x04 */ void *arg;
};

int CheckSurrender(struct SurrenderHook *h);

#endif
