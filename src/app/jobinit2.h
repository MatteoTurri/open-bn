/*
 * Program start-up (jobinit2.cpp): the init job, the launcher of external
 * applications and the interrupt handlers.
 */

#ifndef JOBINIT2_H
#define JOBINIT2_H

class thJob;
class IRrm;
class gm_graph;

extern IRrm *pRM0;   /* the IR remote control */
extern int running_status;   /* scripts run only while it is set */
extern int launcher_switching;
extern int launcher_mode;
extern int launcher_id;
extern int launcher_vt;
extern char launcher_arg[1024];
extern gm_graph g_graph;

void jobInit2(thJob *job);
int exec_binary(char *cmd, char *arg);
int exec_script(char *cmd, char *arg);
void launcher();
void InitInterupt(void);

#endif
