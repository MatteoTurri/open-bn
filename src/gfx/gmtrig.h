/*
 * Sine/cosine lookup table of the gm layer (gmtrig.cpp). Angles are fixed
 * point: 0x10000 = full turn.
 */

#ifndef GMTRIG_H
#define GMTRIG_H

float gmGetTableSin(int angle);
float gmGetTableCos(int angle);
void gmInitSinTable(void);

#endif
