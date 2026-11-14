/*
 * CMP_Constructor, CMP_Destructor, CMP_realloc, CMP_reallocNoCopy and CMP_free.
 */
#include "bsafe.h"

#include "cmp.h"


/* Clears the header of a new CMPInt. */
void CMP_Constructor(CMPInt *x)
{
    x->allocated = 0;
    x->length = 0;
    x->value = 0;
}

/* Wipes the buffer, frees it and clears the header. */
void CMP_Destructor(CMPInt *x)
{
    if (x->value != 0) {
        T_memset(x->value, 0, x->allocated * 4);
        T_free(x->value);
        x->value = 0;
    }
    x->allocated = 0;
    x->length = 0;
}

/* Grows x->value to newAllocated words, copying the contents and wiping and freeing the old buffer.
 * 0x100 when the allocation fails. */
int CMP_realloc(int newAllocated, CMPInt *x)
{
    unsigned long *p;
    unsigned long *newBuf;

    if (x->allocated >= newAllocated)
        return 0;
    if (x->value == 0) {
        x->allocated = newAllocated;
        p = T_malloc(newAllocated * 4);
        x->value = p;
        if (p != 0)
            return 0;
        x->allocated = 0;
        return 0x100;
    }
    newBuf = T_malloc(newAllocated * 4);
    if (newBuf != 0) {
        T_memcpy(newBuf, x->value, x->length * 4);
        T_memset(x->value, 0, x->allocated * 4);
        T_free(x->value);
        x->allocated = newAllocated;
        x->value = newBuf;
        return 0;
    }
    T_memset(x->value, 0, x->allocated * 4);
    T_free(x->value);
    x->allocated = 0;
    x->length = 0;
    x->value = 0;
    return 0x100;
}

/* As CMP_realloc without keeping the contents: x->length becomes 0. */
int CMP_reallocNoCopy(int newAllocated, CMPInt *x)
{
    unsigned long *p;
    unsigned long *newBuf;

    if (x->allocated >= newAllocated)
        return 0;
    if (x->value == 0) {
        x->allocated = newAllocated;
        p = T_malloc(newAllocated * 4);
        x->value = p;
        if (p != 0)
            return 0;
        x->allocated = 0;
        return 0x100;
    }
    newBuf = T_malloc(newAllocated * 4);
    if (newBuf != 0) {
        T_memset(x->value, 0, x->allocated * 4);
        T_free(x->value);
        x->allocated = newAllocated;
        x->length = 0;
        x->value = newBuf;
        return 0;
    }
    T_memset(x->value, 0, x->allocated * 4);
    T_free(x->value);
    x->allocated = 0;
    x->length = 0;
    x->value = 0;
    return 0x100;
}

/* Wipes the buffer, frees it and clears the header, as CMP_Destructor. */
void CMP_free(CMPInt *x)
{
    if (x->value != 0) {
        T_memset(x->value, 0, x->allocated * 4);
        T_free(x->value);
        x->value = 0;
    }
    x->allocated = 0;
    x->length = 0;
}
