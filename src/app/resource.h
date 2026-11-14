/*
 * The textures built into the program (resource.cpp), by index.
 */

#ifndef RESOURCE_H
#define RESOURCE_H

void LoadResource();
void ReleaseResource();
int GetResource(int i);

/* Descriptor of the "cloud" pattern textures, filled by ResourceInitRest. */
struct CloudInfoS {
	/* 0x00 */ int count;  /* number of textures loaded */
	/* 0x04 */ int *ids;  /* points to g_cloudTex */
};
extern CloudInfoS CloudInfo;   /* the default cloud table */

#endif
