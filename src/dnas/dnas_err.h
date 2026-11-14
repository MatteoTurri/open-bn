/*
 * The error codes of the DNAS libraries (DNAS1 and DNAS2 share them). The names of the server
 * results follow the debug URIs that simulate them (dnas1msg.c, debug_fermat/...).
 */

#ifndef DNAS_ERR_H
#define DNAS_ERR_H

/* A header packet with the wrong length, or that does not decrypt (dnas2read.c). */
#define SCE_DNAS_ERR_HEADER (-100)

/* Results of the server ("ng/..."). */
#define SCE_DNAS_ERR_BUSY (-101)
#define SCE_DNAS_ERR_BEFORE_SERVICE (-102)
#define SCE_DNAS_ERR_AFTER_SERVICE (-103)
#define SCE_DNAS_ERR_SERVICE_FINISHED (-104)
#define SCE_DNAS_ERR_TIMEOUT (-105)
#define SCE_DNAS_ERR_BAD_RESPONSE (-106) /* a reply that does not parse */
#define SCE_DNAS_ERR_IO (-107)           /* a short send or receive, or a failed state change */
#define SCE_DNAS_ERR_EXTERNAL (-108)
#define SCE_DNAS_ERR_DIFFERENT_HDD (-109)
#define SCE_DNAS_ERR_ALL_OK (-110) /* nothing to do: the console is already authenticated */

/* Results of a download ("down_result/..."). */
#define SCE_DNAS_ERR_DOWN_NOT_EXIST (-201)
#define SCE_DNAS_ERR_DOWN_BEFORE_SERVICE (-202)
#define SCE_DNAS_ERR_DOWN_SERVICE_FINISHED (-203)
#define SCE_DNAS_ERR_DOWN_NOT_MODIFIED (-204)

/* The identity of the console. */
#define SCE_DNAS_ERR_ID_FAILED (-401) /* a step of the identity computation failed */
#define SCE_DNAS_ERR_ID_BLOCK (-402)  /* the identity block of the medium cannot be read */
#define SCE_DNAS_ERR_ID_MEDIUM (-403) /* unknown medium class */

/* The network layer (dnas_pwrap.c): errno and h_errno classes. */
#define SCE_DNAS_ERR_ABORTED (-601) /* aborted, EINTR or ECONNABORTED */
#define SCE_DNAS_ERR_SSL (-610)
#define SCE_DNAS_ERR_HOST_NOT_FOUND (-611)
#define SCE_DNAS_ERR_TRY_AGAIN (-612)
#define SCE_DNAS_ERR_NO_RECOVERY (-613)
#define SCE_DNAS_ERR_NO_DATA (-614)
#define SCE_DNAS_ERR_RESOLVE (-615)       /* any other resolver error */
#define SCE_DNAS_ERR_NOT_CONNECTED (-616) /* EISCONN, ENOTCONN */
#define SCE_DNAS_ERR_TIMEDOUT (-617)
#define SCE_DNAS_ERR_CONNREFUSED (-618)
#define SCE_DNAS_ERR_NETUNREACH (-619)
#define SCE_DNAS_ERR_NO_RESOURCES (-621) /* EAGAIN, ENOMEM, ENFILE, ENOBUFS */
#define SCE_DNAS_ERR_MFILE (-622)
#define SCE_DNAS_ERR_BAD_SOCKET (-623) /* EBADF, ENOTSOCK */
#define SCE_DNAS_ERR_INVALID (-624)    /* EFAULT, EINVAL, EPROTONOSUPPORT, EAFNOSUPPORT */
#define SCE_DNAS_ERR_NET (-625)        /* any other errno */

/* Results of the identification ("id_result/..."). */
#define SCE_DNAS_ERR_ID_NOT_REGISTERED (-701)
#define SCE_DNAS_ERR_ID_INVALID_CATEGORY (-702)
#define SCE_DNAS_ERR_ID_NOT_JOIN_IN_THIS_CATEGORY (-703)

/* Any other result code n of the server is SCE_DNAS_ERR_NG_BASE - n. */
#define SCE_DNAS_ERR_NG_BASE (-800)

/* DNAS2: a decrypted header without its marker, a digest that does not match. */
#define SCE_DNAS2_ERR_MARKER (-10201)
#define SCE_DNAS2_ERR_DIGEST (-10202)

#endif
