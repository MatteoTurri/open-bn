/*
 * The shared UI textures (resource ids 0..29) and the "cloud" pattern
 * sequence, loaded at startup.
 */

#include <string>
#include <vector>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "gfx/texture.h"
#include "gfx/bt2ipu.h"

/* Texture files by resource id; an empty name leaves the id at 0. */
static char *g_resName[30] = {
	"/opt0/bn/data/tex/https.tm2",
	"/opt0/bn/data/tex/dot.tm2",
	"/opt0/bn/data/tex/grid.tm2",
	"",
	"",
	"/opt0/bn/data/tex/spin.tm2",
	"/opt0/bn/data/tex/btn_r.tm2",
	"/opt0/bn/data/tex/btn_d.tm2",
	"/opt0/bn/data/tex/btn_u.tm2",
	"/opt0/bn/data/tex/btn_l.tm2",
	"/opt0/bn/data/tex/focus.tm2",
	"/opt0/bn/data/tex/bar1.tm2",
	"/opt0/bn/data/tex/bar2.tm2",
	"/opt0/bn/data/tex/bar3.tm2",
	"/opt0/bn/data/tex/radio.tm2",
	"",
	"/opt0/bn/data/tex/editbox.tm2",
	"/opt0/bn/data/tex/busy.tm2",
	"/opt0/bn/data/tex/biff.tm2",
	"/opt0/bn/data/tex/scrbar1.tm2",
	"/opt0/bn/data/tex/scrbar2.tm2",
	"/opt0/bn/data/tex/scrbar3.tm2",
	"/opt0/bn/data/tex/window1.tm2",
	"/opt0/bn/data/tex/check0.tm2",
	"/opt0/bn/data/tex/check1.tm2",
	"/opt0/bn/data/tex/btn_l1.tm2",
	"/opt0/bn/data/tex/btn_r1.tm2",
	"/opt0/bn/data/tex/psbblogo.tm2",
	"/opt0/bn/data/tex/button0.tm2",
	"/opt0/bn/data/tex/button1.tm2",
};
static int g_resTexID[30];

#include "resource.h"

CloudInfoS CloudInfo;

/* Texture ids of the cloud patterns. */
static int g_cloudTex[4];

/* Number of cloud patterns loaded (same value as CloudInfo.count). */
static int g_cloudPatternCount = 0;

static void LoadCloudPatterns();

static int LoadTextureSeq(const char *dir, const char *prefix, const char *ext, int w, int h, int a, int b, int *out);

void LoadResource()
{
	unsigned int i;

	for (i = 0; i < 30; i++) {
		if (strlen(g_resName[i]) == 0)
			g_resTexID[i] = 0;
		else
			g_resTexID[i] = Tim2_GetTexID(g_resName[i], 0);
	}
	LoadCloudPatterns();
}

void ReleaseResource()
{
	unsigned int i;

	for (i = 0; i < 30; i++) {
		if (g_resTexID[i])
			Tim2_ReleaseTexID(g_resTexID[i]);
	}
}

int GetResource(int i)
{
	return g_resTexID[i];
}

/* Loads `count` raw images "<dir>/<prefix>NNNN.<ext>" of w*h*c bytes into
   new textures, stopping at the first file that cannot be opened. Returns
   the number of textures created. */
static int LoadTextureSeq(const char *dir, const char *prefix, const char *ext, int w, int h, int c, int count,
    int *out)
{
	char buf[2048];
	int i = 0;
	int size = w * h * c;

	for (;;) {
		sprintf(buf, "%s/%s%4.4d.%s", dir, prefix, i, ext);
		fprintf(stderr, "%s/%s%4.4d.%s", dir, prefix, i, ext);
		FILE *fp = fopen(buf, "r");
		if (!fp) {
			fprintf(stderr, "\n");
			break;
		}
		fprintf(stderr, "...open succesed!\n");
		void *p = malloc(size);
		/* NOTE: bug: the error paths below leave the file open */
		if (!p) {
			fprintf(stderr, "Can't alloc memory for cloud pattern\n");
			break;
		}
		if (fread(p, 1, size, fp) != (unsigned int)size) {
			fprintf(stderr, "Wrong cloud pattern size\n");
			break;
		}
		fclose(fp);
		fprintf(stderr, "Read cloud/ray pattern successfully: %s\n", buf);
		int tex = tmCreateTexture(Texture::Manager(), w, h, 0);
		out[i] = tex;
		tmUpdateTexture(tex, w, h, 0, p);
		free(p);
		i++;
		if (i >= count)
			return i;
	}
	return i;
}

/* Loads the cloud_pattern_NNNN.raw textures and publishes their count. */
static void LoadCloudPatterns()
{
	int n = LoadTextureSeq("/opt0/bn/data/tex", "cloud_pattern_", "raw", 256, 256, 4, 4, g_cloudTex);

	g_cloudPatternCount = n;
	CloudInfo.count = n;
	CloudInfo.ids = g_cloudTex;
}
