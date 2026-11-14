/*
 * DNAS2 transform descriptor: reduce each 4-bit nibble of id modulo 8; the top
 * nibble (the round count) defaults to 5 when its low three bits are zero.
 */

#ifndef DNAS2M12_XFORM_DESC_H
#define DNAS2M12_XFORM_DESC_H

int __sce_dnas2_symbol12004(int id);

#endif
