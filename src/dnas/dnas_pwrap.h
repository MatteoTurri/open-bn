/*
 * DNAS2 "pwrap" transport: plain or TLS socket to the DNAS server, with the errno to
 * DNAS error code (-6xx) translation and the TLS certificate checks.
 */

#ifndef DNAS_PWRAP_H
#define DNAS_PWRAP_H

int _sce_dnas2_init_netglue_table(void **table);

#endif
