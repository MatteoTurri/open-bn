/*
 * Debug trace macros of the Small natives, empty in this release build.
 * BN_TRACE_FMT(buf, val) formats val into buf and logs it; BN_TRACE(n, val)
 * traces val through an n-byte buffer and yields val as an lvalue;
 * BN_ASSERT(e) only type-checks e.
 */

#ifndef BNTRACE_H
#define BNTRACE_H

#define BN_TRACE_FMT(buf, val)
#define BN_TRACE(n, val)	(*({ char bn_trace_buf_[n]; BN_TRACE_FMT(bn_trace_buf_, val); &(val); }))
#define BN_ASSERT(e)		((void) sizeof(e))

#endif
