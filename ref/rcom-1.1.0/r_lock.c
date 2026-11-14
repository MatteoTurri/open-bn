/*
 * The locks and thread ids of RCOM (R_lock_*, R_thread_*, R_locked_add, R_lockid_new,
 * R_lockids_free): the names of the 27 static locks and of the ones the application adds, and the
 * callbacks an application installs.
 */

#include <string.h>
#include <unistd.h>
#include "buffer.h"

#define R_LOCK_FILE "./os/lock/../../os/lock/r_lock.c"

#include "r_com.h"

/* The three callbacks, the names of the 27 static locks (the CRYPTO_LOCK_* of SSLeay and four of
 * RCOM) and the application locks. */
static R_LockCb      g_lock_cb = 0;
static R_LockedAddCb g_locked_add_cb = 0;
static R_ThreadIdCb  g_thread_id_cb = 0;
static char *lock_names[27] = {
    "<<ERROR>>", "err", "err_hash", "x509",
    "x509_info", "x509_pkey", "x509_crl", "x509_req",
    "dsa", "rsa", "evp_pkey", "x509_store",
    "ssl_ctx", "ssl_cert", "ssl_session", "ssl",
    "rand", "debug_malloc", "BIO", "bio_gethostbyname",
    "<<RESERVED>>", "debug_malloc_mode", "RSA_method", "DH_method",
    "app_lock", "R_CACHE_LOCK", "GET_MODULE"
};
#include "stack.h"

static STACK *app_locks = 0;


/* The two error cases return the empty string. */
static char *lock_name_raw(int type)
{
    char *ret;

    ret = 0;
    if (type < 0) {
        ret = "";
    } else if (type < 27) {
        ret = lock_names[type];
    } else if (app_locks == 0 || (type - 27) >= app_locks->num) {
        ret = "";
    } else {
        ret = app_locks->data[type - 27];
    }
    return ret;
}

int R_lockid_new(char *name)
{
    char *cur;
    int i;
    int num = 0;

    num = R_lock_num();
    R_lock_ctrl(9, 24, R_LOCK_FILE, 116);

    if (name != 0) {
        for (i = 0; i < num; i++) {
            cur = lock_name_raw(i);
            if (cur != 0) {
                if (strcmp(cur, name) == 0) {
                    goto done;
                }
            }
        }
    }

    if (app_locks == 0) {
        app_locks = sk_new(0);
        if (app_locks == 0) {
            i = 0;
            goto done;
        }
    }

    cur = BUF_strdup(name);
    if (cur == 0) {
        i = 0;
        goto done;
    }

    i = sk_push(app_locks, cur);
    if (i == 0) {
        R_free(cur);
    } else {
        i += 26;
    }

done:
    R_lock_ctrl(10, 24, R_LOCK_FILE, 165);
    return i;
}

void R_lockids_free(void)
{
    char *p;

    R_lock_ctrl(9, 24, R_LOCK_FILE, 174);

    if (app_locks != 0) {
        while (*(int *)app_locks != 0) {
            p = sk_pop(app_locks);
            R_free(p);
        }
        sk_free(app_locks);
        app_locks = 0;
    }

    R_lock_ctrl(10, 24, R_LOCK_FILE, 188);
}

int R_lock_num(void)
{
    int ret;

    R_lock_ctrl(9, 24, R_LOCK_FILE, 196);
    ret = 27;
    if (app_locks != 0) {
        ret = ret + *(int *)app_locks;
    }
    R_lock_ctrl(10, 24, R_LOCK_FILE, 202);
    return ret;
}

char *R_lock_get_name(int type)
{
    char *ret = 0;

    if (type < 0) {
        ret = "ERROR";
        goto end;
    }

    if (type < 27) {
        ret = lock_names[type];
        goto end;
    }

    R_lock_ctrl(9, 24, R_LOCK_FILE, 218);

    if (app_locks != 0) {
        if ((type - 27) < app_locks->num) {
            goto valid;
        }
    }
    ret = "ERROR";
    goto unlock;

valid:
    ret = app_locks->data[type - 27];

unlock:
    R_lock_ctrl(10, 24, R_LOCK_FILE, 224);

end:
    return ret;
}


R_LockCb R_lock_get_cb(void)
{
    return g_lock_cb;
}

R_LockedAddCb R_locked_add_get_cb(void)
{
    return g_locked_add_cb;
}

int R_lock_set_cb(R_LockCb cb)
{
    g_lock_cb = cb;
    return 1;
}

int R_locked_add_set_cb(R_LockedAddCb cb)
{
    g_locked_add_cb = cb;
    return 1;
}


int R_lock_ctrl(int mode, int type, char *file, int line)
{
    int ret = 1;

    if (g_lock_cb != 0) {
        ret = (*g_lock_cb)(mode, type, file, line);
    }
    return ret;
}

int R_locked_add(int *pointer, int amount, int type, char *file, int line)
{
    int ret;

    if (g_locked_add_cb != 0) {
        ret = (*g_locked_add_cb)(pointer, amount, type, file, line);
    } else {
        if (R_lock_ctrl(9, type, file, line) == 0) {
            ret = -1;
            goto end;
        }
        ret = *pointer + amount;
        *pointer = ret;
        if (R_lock_ctrl(10, type, file, line) == 0) {
            ret = -1;
            goto end;
        }
    }
end:
    return ret;
}

R_ThreadIdCb R_thread_get_id_cb(void)
{
    return g_thread_id_cb;
}

int R_thread_id_set_cb(R_ThreadIdCb cb)
{
    g_thread_id_cb = cb;
    return 1;
}

unsigned long R_thread_id(void)
{
    unsigned long ret = 0;

    if (g_thread_id_cb == 0) {
        ret = getpid();
    } else {
        ret = (*g_thread_id_cb)();
    }
    return ret;
}
