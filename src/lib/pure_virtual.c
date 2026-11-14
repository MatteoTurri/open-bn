/*
 * __pure_virtual: the C++ runtime handler for a call to a pure virtual method
 * (the L_pure section of libgcc2.c, written out here).
 */

extern int __write(int fd, const void *buf, unsigned int n);
extern void __terminate(void);

#define MESSAGE "pure virtual method called\n"

void __pure_virtual()
{
	__write(2, MESSAGE, sizeof(MESSAGE) - 1);
	__terminate();
}
