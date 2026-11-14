/*
 * TRY module filesystem factory and partition size labels (trycreatefs.cpp).
 */

#ifndef TRYCREATEFS_H
#define TRYCREATEFS_H

class TRYAbstFS;

extern "C" {

TRYAbstFS *TRYCreateFS(int type);
char *try_getStrPatSize(int size);
}

#endif
