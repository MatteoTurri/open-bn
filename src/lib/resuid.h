/*
 * getresuid/setresuid: glibc exports them, but the kit's <unistd.h> does not
 * declare them.
 */

#ifndef RESUID_H
#define RESUID_H

#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

int getresuid(uid_t *ruid, uid_t *euid, uid_t *suid);
int setresuid(uid_t ruid, uid_t euid, uid_t suid);

#ifdef __cplusplus
}
#endif

#endif /* RESUID_H */
