/*
 * DNAS1 core: the client state machine (_sce_dnas1_core_loop), state changes, and
 * checks on the action block.
 */

#ifndef DNAS1CORE_H
#define DNAS1CORE_H

int _sce_dnas1_abort_called(void);
int _sce_dnas1_check_action_purpose(int purpose);
int _sce_dnas1_core_in_data(void);
int _sce_dnas1_core_in_sleep(void);
int _sce_dnas1_core_loop(void);
int _sce_dnas1_core_wakeup(void);
int _sce_dnas1_incr_data_wait(void);
int _sce_dnas1_set_state(int newstate, int action, int arg);

#endif
