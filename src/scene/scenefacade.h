/*
 * Scene facade: the gm_graph of the scene and its color, frame and depth buffers.
 */

#ifndef SCENEFACADE_H
#define SCENEFACADE_H

class gm_graph;

void SceneInit(gm_graph &graph);
void SceneRun(gm_graph &graph);
void SceneTerminate(gm_graph &graph);
void *SceneGetDefaultPltmFontID();
int *SceneGetAccumlationBuffer();
gm_graph *SceneGetGraph();
void SceneSetFrameBuffer(int index, int handle);
void SceneSetDepthBuffer(int index, int handle);
int SceneGetFrameBuffer(int index);
int SceneGetDepthBuffer(int index);
void SceneEnableDepthBuffer(bool enable);

#endif
