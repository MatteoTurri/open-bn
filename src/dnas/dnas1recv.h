/*
 * DNAS1 message layer, receiving side: reads the HTTP reply, checks its header and
 * parses the fields of each reply type (_sce_dnas1_recv_message).
 */

#ifndef DNAS1RECV_H
#define DNAS1RECV_H

int _sce_dnas1_recv_message(int code, void *ctx, int flags);

#endif
