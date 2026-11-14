/*
 * Debug trace helper shared by the C units of the text input system
 * (textim, fep*, skb*, ...); not called.
 */
#ifndef TEXTIM_DEBUG_H
#define TEXTIM_DEBUG_H

#include <stdio.h>

static __inline__ void textim_print(const char *msg)
{
	printf("textim: ");
	printf("%s", msg);
}

#endif /* TEXTIM_DEBUG_H */
