/*
 * Scene facade: free functions the application uses to set up the scene
 * graph's color, frame and depth buffers without touching gm_graph directly.
 */
#include <iostream.h>
#include "gfx/gm_graph.h"
#include "gfx/vec.h"
#include "scenefacade.h"
#include "fw/fw.h"

/* Unit matrix and unit w row: the file-scope objects built at startup. */
static mat4 s_unitMatrix(1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    1.0f);
static vec4 s_unitRow3(0.0f, 0.0f, 0.0f, 1.0f);

static int g_accumBuffer[4];
static gm_graph *s_graph;
static int g_frameBuffer[4] = { -1, -1, -1, -1 };
static int g_depthBuffer[4] = { -1, -1, -1, -1 };

/* Creates the four accumulation buffers at decreasing resolution and the
   sine table, then remembers the graph. */
void SceneInit(gm_graph &graph)
{
	g_accumBuffer[0] = graph.CreateColorBuffer(320, 224, 0);
	g_accumBuffer[1] = graph.CreateColorBuffer(256, 256, 0);
	g_accumBuffer[2] = graph.CreateColorBuffer(128, 128, 0);
	g_accumBuffer[3] = graph.CreateColorBuffer(64, 64, 0);
	gmInitSinTable();
	s_graph = &graph;
}

void SceneRun(gm_graph &graph)
{}

/* NOTE: bug: frees only three of the four buffers, g_accumBuffer[3] leaks */
void SceneTerminate(gm_graph &graph)
{
	graph.DeleteColorBuffer(g_accumBuffer[0]);
	graph.DeleteColorBuffer(g_accumBuffer[1]);
	graph.DeleteColorBuffer(g_accumBuffer[2]);
}

void *SceneGetDefaultPltmFontID()
{
	return 0;
}

int *SceneGetAccumlationBuffer()
{
	return g_accumBuffer;
}

gm_graph *SceneGetGraph()
{
	return s_graph;
}

void SceneSetFrameBuffer(int index, int handle)
{
	g_frameBuffer[index] = handle;
}

void SceneSetDepthBuffer(int index, int handle)
{
	g_depthBuffer[index] = handle;
}

int SceneGetFrameBuffer(int index)
{
	return g_frameBuffer[index];
}

int SceneGetDepthBuffer(int index)
{
	return g_depthBuffer[index];
}

/* Attaches the draw/display pair g_frameBuffer[0..1] to the real z-buffer
   (g_depthBuffer[0]) or, when disabling, to g_depthBuffer[1] if one was set
   (the initializer -1 means none). */
void SceneEnableDepthBuffer(bool enable)
{
	if (enable) {
		fwGsAttachZBuf(g_frameBuffer[0], g_depthBuffer[0]);
		fwGsAttachZBuf(g_frameBuffer[1], g_depthBuffer[0]);
	} else if (g_depthBuffer[1] >= 0) {
		fwGsAttachZBuf(g_frameBuffer[0], g_depthBuffer[1]);
		fwGsAttachZBuf(g_frameBuffer[1], g_depthBuffer[1]);
	}
}
