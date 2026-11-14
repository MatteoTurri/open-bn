/*
 * The platform layer of BSAFE (T_*): thin wrappers of the C library. Each one returns at once on a
 * zero length (T_malloc never passes a size of 0 to malloc).
 */
#include <string.h>

#include <stdlib.h>

void T_memset(void *s, int c, unsigned int n)
{
    if (n != 0)
        memset(s, c, n);
}

void T_memcpy(void *dst, const void *src, unsigned int n)
{
    if (n != 0)
        memcpy(dst, src, n);
}

void T_memmove(void *dst, const void *src, unsigned int n)
{
    if (n != 0)
        memmove(dst, src, n);
}

int T_memcmp(const void *a, const void *b, unsigned int n)
{
    if (n == 0)
        return 0;
    return memcmp(a, b, n);
}

void *T_malloc(unsigned int size)
{
    if (size == 0)
        size = 1;
    return malloc(size);
}

void *T_realloc(void *ptr, unsigned int size)
{
    void *result;
    if (ptr == 0)
        return T_malloc(size);
    result = realloc(ptr, size != 0 ? size : 1);
    if (result == 0)
        free(ptr);
    return result;
}

void T_free(void *p)
{
    if (p != 0)
        free(p);
}

unsigned int T_strlen(const char *s)
{
    return strlen(s);
}

char *T_strcpy(char *dst, const char *src)
{
    return strcpy(dst, src);
}

int T_strcmp(const char *a, const char *b)
{
    return strcmp(a, b);
}
