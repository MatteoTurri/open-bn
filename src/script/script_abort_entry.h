/*
 * script_abort_entry.cpp: the Script__Abort entry points, which decode a buffer through DNAS.
 */

#ifndef SCRIPT_ABORT_ENTRY_H
#define SCRIPT_ABORT_ENTRY_H

extern "C" {

int Script__Abort(int size, void *buf, int *outsize);
int Script__Abort2(int size, void *buf, int *outsize);
int Script__Abort3(int size, void *buf, int *outsize);
}

#endif
