#!/usr/bin/env python3
"""Run a command making it see a fixed time.

gcc 2.95 expands `__DATE__`/`__TIME__` to the system time of the compilation, and
giflib puts them in .rodata (VersionStr): the original has the times of Sony's build
(`Aug 28 2002,   00:10:29`...). The preprocessor is a static i386 binary, so
LD_PRELOAD does not work: the command is traced with ptrace (children included: the
gcc driver spawns cpp/cc1) and the wanted value is written at the exit of the time
syscalls. The compiler is not modified: only its environment is.

    python3 tools/fixtime.py EPOCH -- command ...     (EPOCH in seconds, read as UTC: TZ=UTC)

Linux x86-64 or aarch64 kernel (>= 5.3: PTRACE_GET_SYSCALL_INFO). The tracee can be
i386, x86-64 or aarch64: on an Apple Silicon host (Docker Desktop) the tracer runs under
Rosetta and the compiler's i386 binaries under the VM's binfmt qemu-i386 for aarch64;
the kernel then sees qemu's aarch64 syscalls, and those are the ones rewritten.

It never hangs: every ptrace call is checked (an error kills the traced tree and exits
3 with a message), the tracee dies with the tracer (PTRACE_O_EXITKILL), there is a
timeout (FIXTIME_TIMEOUT, seconds, default 600: exits 124) and, if the command ends
without any time syscall having been rewritten, it exits 4 (the time was not fixed: the
result is not reliable). FIXTIME_ALLOW_NONE=1 disables this last check."""
import ctypes
import os
import signal
import struct
import sys

libc = ctypes.CDLL(None, use_errno=True)
libc.ptrace.restype = ctypes.c_long
libc.ptrace.argtypes = [ctypes.c_long, ctypes.c_long, ctypes.c_void_p, ctypes.c_void_p]

PTRACE_TRACEME, PTRACE_SYSCALL = 0, 24
PTRACE_SETOPTIONS = 0x4200
PTRACE_GETREGSET, PTRACE_SETREGSET = 0x4204, 0x4205
PTRACE_GET_SYSCALL_INFO = 0x420e
NT_PRSTATUS = 1
# TRACESYSGOOD | FORK | VFORK | CLONE | EXEC | EXITKILL
OPT = 0x1 | 0x2 | 0x4 | 0x8 | 0x10 | 0x100000
WALL = 0x40000000                     # __WALL: the os module does not export it
SYSINFO_NONE, SYSINFO_ENTRY, SYSINFO_EXIT = 0, 1, 2

# arch (AUDIT_ARCH_*) -> time syscalls, size of the NT_PRSTATUS regset, index of the
# return register in the regset, format of time_t/timeval/timespec
I386, X86_64, AARCH64 = 0x40000003, 0xc000003e, 0xc00000b7
ARCH = {
    I386: dict(time=13, gtod=78, cgt=(265, 403), cgt64=(403,), regs=17 * 4, ret=6, w='<I',
               tv='<ii'),
    X86_64: dict(time=201, gtod=96, cgt=(228,), cgt64=(), regs=27 * 8, ret=10, w='<Q',
                 tv='<qq'),
    AARCH64: dict(time=None, gtod=169, cgt=(113,), cgt64=(), regs=34 * 8, ret=0, w='<Q',
                  tv='<qq'),
}
DEBUG = bool(os.environ.get('FIXTIME_DEBUG'))
REALTIME_CLOCKS = (0, 5)              # CLOCK_REALTIME, CLOCK_REALTIME_COARSE


def sig_of(st):
    return os.WSTOPSIG(st)


class Fail(Exception):
    def __init__(self, msg, errno=0):
        super().__init__(msg)
        self.errno = errno


class Iovec(ctypes.Structure):
    _fields_ = [('base', ctypes.c_void_p), ('len', ctypes.c_size_t)]


def ptrace(req, pid, addr=None, data=None):
    ctypes.set_errno(0)
    r = libc.ptrace(req, pid, addr, data)
    if r == -1 and ctypes.get_errno():
        e = ctypes.get_errno()
        raise Fail(f'ptrace({req:#x}, pid {pid}): {os.strerror(e)} (errno {e})', e)
    return r


def syscall_info(pid):
    """(op, arch, sp, nr, args | rval) via PTRACE_GET_SYSCALL_INFO: independent of
    the architecture, and tells entry from exit without counting stops"""
    buf = ctypes.create_string_buffer(88)
    n = ptrace(PTRACE_GET_SYSCALL_INFO, pid, ctypes.c_void_p(88), buf)
    op, arch, ip, sp = struct.unpack_from('<B3xIQQ', buf.raw, 0)
    if n < 24 or arch not in ARCH:
        raise Fail(f'pid {pid}: unsupported architecture {arch:#x} (syscall_info {n} B)')
    if op == SYSINFO_ENTRY:
        return op, arch, sp, struct.unpack_from('<Q', buf.raw, 24)[0], \
            struct.unpack_from('<6Q', buf.raw, 32)
    if op == SYSINFO_EXIT:
        rval, is_err = struct.unpack_from('<qB', buf.raw, 24)
        return op, arch, sp, None, (rval, is_err)
    return op, arch, sp, None, None


def mem_write(pid, addr, data):
    fd = os.open(f'/proc/{pid}/mem', os.O_RDWR)
    try:
        if os.pwrite(fd, data, addr) != len(data):
            raise Fail(f'pid {pid}: short write at {addr:#x}')
    finally:
        os.close(fd)


def mem_read(pid, addr, n):
    fd = os.open(f'/proc/{pid}/mem', os.O_RDONLY)
    try:
        return os.pread(fd, n, addr)
    finally:
        os.close(fd)


def set_return(pid, arch, value):
    a = ARCH[arch]
    buf = ctypes.create_string_buffer(a['regs'])
    iov = Iovec(ctypes.cast(buf, ctypes.c_void_p), a['regs'])
    ptrace(PTRACE_GETREGSET, pid, ctypes.c_void_p(NT_PRSTATUS), ctypes.byref(iov))
    if iov.len != a['regs']:
        raise Fail(f'pid {pid}: regset of {iov.len} B, expected {a["regs"]}')
    w = struct.calcsize(a['w'])
    struct.pack_into(a['w'], buf, a['ret'] * w, value & (256 ** w - 1))
    ptrace(PTRACE_SETREGSET, pid, ctypes.c_void_p(NT_PRSTATUS), ctypes.byref(iov))


def hide_vdso(pid, sp):
    """At exec: AT_SYSINFO_EHDR -> AT_IGNORE in the auxv on the stack, so that the
    tracee's libc reads the time with a SYSCALL instead of from the vDSO (with the vDSO
    there is no time syscall at all). The auxv is searched on the stack by content
    (exact copy of /proc/PID/auxv), between sp and the end of the [stack] mapping: no
    arithmetic on argc/envp and no unbounded loops."""
    exe = os.path.basename(os.readlink(f'/proc/{pid}/exe'))
    if exe == 'rosetta':
        # Rosetta (Apple Silicon) requires the vDSO: without it, `assertion failed: Failed to
        # find vdso elf header in aux vector` and SIGTRAP. A translated x86-64 process is never
        # the compiler's time reader (i386: under qemu).
        return
    raw = open(f'/proc/{pid}/auxv', 'rb').read()
    # 8-byte words, or 4-byte ones for an i386 process on an x86-64 kernel: in the first case
    # the first type is small, in the second the first 8-byte word also holds the value
    w, fmt = (8, '<QQ') if len(raw) >= 8 and struct.unpack_from('<Q', raw)[0] < 256 else (4, '<II')
    auxv = None
    for i in range(0, len(raw) - 2 * w + 1, 2 * w):
        if struct.unpack_from(fmt, raw, i)[0] == 0:              # AT_NULL: end, included
            auxv = raw[:i + 2 * w]
            break
    if not auxv:
        raise Fail(f'pid {pid}: malformed auxv ({len(raw)} B)')
    end = None
    for line in open(f'/proc/{pid}/maps'):
        lo, hi = (int(x, 16) for x in line.split()[0].split('-'))
        if lo <= sp < hi:
            end = hi
            break
    if end is None:
        raise Fail(f'pid {pid}: sp {sp:#x} outside every mapping')
    stack = mem_read(pid, sp, end - sp)
    off = stack.find(auxv)
    if off < 0:
        raise Fail(f'pid {pid}: auxv not found on the stack')
    for i in range(0, len(auxv), 2 * w):
        if struct.unpack_from(fmt, auxv, i)[0] == 33:            # AT_SYSINFO_EHDR
            mem_write(pid, sp + off + i, struct.pack(fmt[:2], 1))  # AT_IGNORE


def fix_exit(pid, entry, rval, epoch):
    """At the exit of a time syscall: write the fixed time. Returns 1 if it rewrote it."""
    arch, nr, args = entry
    a = ARCH[arch]
    if DEBUG and nr in (a['time'], a['gtod']) + a['cgt']:
        sys.stderr.write(f'fixtime: pid {pid} arch {arch:#x} syscall {nr} args {args[:2]} -> {rval}\n')
    if rval[1]:                                  # the syscall failed: leave it alone
        return 0
    if nr == a['time']:
        if args[0]:
            mem_write(pid, args[0], struct.pack(a['w'][0] + a['w'][1].lower(), epoch))
        set_return(pid, arch, epoch)
    elif nr == a['gtod']:
        if not args[0]:
            return 0
        mem_write(pid, args[0], struct.pack(a['tv'], epoch, 0))
    elif nr in a['cgt']:
        if not args[1] or (args[0] & 0xffffffff) not in REALTIME_CLOCKS:
            return 0
        tv = '<qq' if nr in a['cgt64'] else a['tv']
        mem_write(pid, args[1], struct.pack(tv, epoch, 0))
    else:
        return 0
    return 1


def main():
    if len(sys.argv) < 4 or sys.argv[2] != '--':
        sys.exit(__doc__)
    epoch = int(sys.argv[1])
    argv = sys.argv[3:]
    timeout = int(os.environ.get('FIXTIME_TIMEOUT', '600'))
    allow_none = os.environ.get('FIXTIME_ALLOW_NONE') == '1'
    os.environ['TZ'] = 'UTC'
    pid = os.fork()
    if pid == 0:
        try:
            ptrace(PTRACE_TRACEME, 0)
            os.kill(os.getpid(), signal.SIGSTOP)    # the parent sets the options
            os.execvp(argv[0], argv)
        except BaseException as e:
            os.write(2, f'fixtime: {e}\n'.encode())
        os._exit(127)
    live, seen = {pid}, {pid}

    def die(msg, code):
        for p in live:
            try:
                os.kill(p, signal.SIGKILL)
            except OSError:
                pass
        sys.stderr.write(f'fixtime: {msg}\n')
        os._exit(code)

    signal.signal(signal.SIGALRM, lambda *_: die(f'timeout of {timeout} s exceeded: '
                                                  f'command killed ({" ".join(argv)})', 124))
    signal.alarm(timeout)
    fixed, status = 0, 0
    pending = {}                                  # pid -> (arch, nr, args) at syscall entry
    try:
        _, st = os.waitpid(pid, 0)
        if not os.WIFSTOPPED(st):
            die(f'the child did not stop (status {st:#x})', 3)
        ptrace(PTRACE_SETOPTIONS, pid, None, ctypes.c_void_p(OPT))
        ptrace(PTRACE_SYSCALL, pid)
        while live:
            try:
                p, st = os.waitpid(-1, WALL)
            except ChildProcessError:
                break
            if os.WIFEXITED(st) or os.WIFSIGNALED(st):
                live.discard(p)
                pending.pop(p, None)
                if p == pid:
                    status = os.WEXITSTATUS(st) if os.WIFEXITED(st) else 128 + os.WTERMSIG(st)
                continue
            live.add(p)
            new = p not in seen
            if DEBUG:
                sys.stderr.write(f'fixtime: stop {p} sig={sig_of(st)} ev={st >> 16}\n')
            seen.add(p)
            sig, event, deliver = os.WSTOPSIG(st), st >> 16, 0
            if sig == (0x80 | signal.SIGTRAP):    # syscall stop (TRACESYSGOOD)
                op, arch, sp, nr, args = syscall_info(p)
                if op == SYSINFO_ENTRY:
                    pending[p] = (arch, nr, args)
                elif op == SYSINFO_EXIT and p in pending:
                    fixed += fix_exit(p, pending.pop(p), args, epoch)
            elif event:
                if event == 4:                    # PTRACE_EVENT_EXEC: new image
                    pending.pop(p, None)
                    hide_vdso(p, syscall_info(p)[2])
            elif sig == signal.SIGSTOP and new:
                pass                              # first stop of a newly created child
            else:                                 # real signal, SIGTRAP included: deliver it
                deliver = sig
            try:
                ptrace(PTRACE_SYSCALL, p, None, ctypes.c_void_p(deliver))
            except Fail as e:
                if e.errno != 3:                  # ESRCH: killed from outside, waitpid will report it
                    raise
    except (Fail, OSError) as e:
        die(f'tracing failed, command killed: {e}', 3)
    signal.alarm(0)
    if status == 0 and fixed == 0 and not allow_none:
        sys.stderr.write('fixtime: no time syscall rewritten: the time was not fixed '
                         '(FIXTIME_ALLOW_NONE=1 to accept it)\n')
        sys.exit(4)
    if os.environ.get('FIXTIME_VERBOSE'):
        sys.stderr.write(f'fixtime: {fixed} time syscalls rewritten\n')
    sys.exit(status)


if __name__ == '__main__':
    main()
