#!/usr/bin/env python3
"""The DNAS1 self-modifying transformation (bn-jpn.elf).

Everything read from the real assembly (expected/asm/*.s + binutils disassembly of
the gap):

  _sce_dnas_pwrap_main(a0, a1, a2, a3)            @0x5c5030
      (expected/asm/005C5030__sce_dnas_pwrap_main.s)
     s3 = len  = a1 ^ a2      (a2 = code, a1 = len^code)
     s6 = id   = a3 >> 1      s5 = dir = a3 & 1
     dir=0 -> a0 is the start of the region; *(start-20) = start   ("guard")
     dir=1 -> a0 is the end   of the region; *(start-20) = 0
     mprotect(pagebase, ..., 7=RWX)
     <gap 0x5c5220>(id, &shift, &count, &list)     -- lookup in
         _sce_dnas_pwrap_info_tbl @0x5d9a00 (via GOT -23080(gp))
     dir=0 -> _sce_dnas_pwrap_descramble(start, end, start-shift, code,
                                         count, list)
     dir=1 -> _sce_dnas_pwrap_scramble  (start, end, start-shift, code,
                                         count, list)
     cacheflush(start-20, len+20, 2); cacheflush(start-20, len+20, 1)
     mprotect(pagebase, ..., 5=RX)

  _sce_dnas_pwrap_descramble/scramble             @0x5c5b60 / @0x5c52e0
     seed  = _sce_dnas_pwrap_clean_id(code)       @0x5edc90
     count = seed >> 28 ; for i = 0..count-1:
        table[(seed >> 4i) & 0xf](start, end, base, cnt, list)
     8-pointer tables read from the binary:
        scramble   @0x100a2e60 = GOT_PAGE(-32612(gp)=0x100a0000) + 11872
        descramble @0x100a2e80 = GOT_PAGE(-32612(gp)=0x100a0000) + 11904

  _sce_dnas_pwrap_clean_id(code)                  @0x5edc90
     top = (code >> 28) & 7 ; count = 5 if top == 0 else top
     nibble i (i=0..6) &= 7 ; result = (nibbles) | (count << 28)
     -- identical to __sce_dnas2_symbol11004.

  _sce_dnas_pwrap_is_scramble_area(off, cnt, list) @0x5edc50
     0 if off is in the list (word not to transform), 1 otherwise.
     off = p - base = (p - start) + shift, so the excluded offset relative
     to start is list[i] - shift. -- identical to __sce_dnas2_symbol11005.

Result: the DNAS1 cipher is bit for bit the same as DNAS2 (same two constants,
same 8 primitives, same bank order, same `recipe`). This module defines the
primitives from scratch from the DNAS1 assembly (ROTL/ROTR/PERM32 as in
src/dnas/dnaspwrap.c) and then checks that they equal BANK_A/BANK_B of
tools/dnas2_crypt.py on a sample covering every single bit plus structured
values (self-test at the bottom; run without arguments).

    python3 tools/dnas1_crypt.py                    # self-test
    python3 tools/dnas1_crypt.py ADDR NWORDS CODE   # descramble NWORDS words at ADDR
"""
import struct
import sys
import os

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

M = 0xFFFFFFFF


def rotl(x, n):
    return ((x << n) | (x >> (32 - n))) & M


def rotr(x, n):
    return ((x >> n) | (x << (32 - n))) & M


def perm(x):
    """_sce_dnas_pwrap_word_swap3: b0'=b1 b1'=b3 b2'=b0 b3'=b2."""
    b = [(x >> (8 * i)) & 0xff for i in range(4)]
    return (b[1] | (b[3] << 8) | (b[0] << 16) | (b[2] << 24)) & M


def perm_inv(x):
    """_sce_dnas_pwrap_de_word_swap3."""
    b = [(x >> (8 * i)) & 0xff for i in range(4)]
    return (b[2] | (b[0] << 8) | (b[3] << 16) | (b[1] << 24)) & M


K1 = 0x09F8ED17   # _sce_dnas_pwrap_sample_xor  / mix1 / mix3
K2 = 0xA95128C3   # _sce_dnas_pwrap_sample_xor2 / mix2

# table @0x100a2e60 (scramble, dir=1)
BANK_SCRAMBLE = {
 0: ('sample_xor  x^K1',            lambda x: x ^ K1),
 1: ('sample_xor2 x^K2',            lambda x: x ^ K2),
 2: ('word_swap1  rotl13',          lambda x: rotl(x, 13)),
 3: ('word_swap2  rotl8',           lambda x: rotl(x, 8)),
 4: ('word_swap3  perm',            perm),
 5: ('mix1        rotl13^K1',       lambda x: rotl(x, 13) ^ K1),
 6: ('mix2        perm(rotl8^K2)',  lambda x: perm(rotl(x, 8) ^ K2)),
 7: ('mix3        rotl21(perm)^K1', lambda x: rotl(rotl(perm(x), 13), 8) ^ K1),
}
# table @0x100a2e80 (descramble, dir=0)
BANK_DESCRAMBLE = {
 0: ('de_sample_xor  x^K1',              lambda x: x ^ K1),
 1: ('de_sample_xor2 x^K2',              lambda x: x ^ K2),
 2: ('de_word_swap1  rotr13',            lambda x: rotr(x, 13)),
 3: ('de_word_swap2  rotr8',             lambda x: rotr(x, 8)),
 4: ('de_word_swap3  perm-1',            perm_inv),
 5: ('de_mix1        rotr13(^K1)',       lambda x: rotr(x ^ K1, 13)),
 6: ('de_mix2        rotr8(perm-1^K2)',  lambda x: rotr(perm_inv(x) ^ K2, 8)),
 7: ('de_mix3        perm-1(rotr21(^K1))',
     lambda x: perm_inv(rotr(rotr(x ^ K1, 8), 13))),
}


def recipe(code):
    """_sce_dnas_pwrap_clean_id + the scramble/descramble loop."""
    top = (code >> 28) & 0xf
    count = 5 if (top & 7) == 0 else (top & 7)
    norm = [(((code >> (4 * i)) & 0xf) & 0x7) for i in range(7)]
    return count, [norm[i] for i in range(count)]


def transform(words, code, bank, base_off=0, excl=()):
    count, idxs = recipe(code)
    out = list(words)
    for i in idxs:
        _name, fn = bank[i]
        for k in range(len(out)):
            if (base_off + 4 * k) in excl:
                continue
            out[k] = fn(out[k])
    return out


# ---------------------------------------------------------------- ELF access
import importlib.util  # noqa: E402

_spec = importlib.util.spec_from_file_location("gc", "tools/got_callees.py")
gc = importlib.util.module_from_spec(_spec)
_spec.loader.exec_module(gc)
DATA, SECTIONS = gc.load_elf(gc.ELF)
A2O = gc.make_addr_to_off(SECTIONS)
SYMS, SYMINFO = gc.load_symbols(gc.SYMBOL_ADDRS)


def read_words(addr, n):
    return list(struct.unpack_from('<%dI' % n, DATA, A2O(addr)))


# ------------------------------------------------------------------ self-test
def _selftest():
    """Compare the 8+8 DNAS1 primitives with the DNAS2 ones (tools/dnas2_crypt.py)
    on a sample covering every single bit plus structured values."""
    from dnas2_crypt import BANK_A, BANK_B
    xs = [0, M, 0x01234567, 0x89ABCDEF, 0xDEADBEEF, K1, K2]
    xs += [1 << i for i in range(32)]
    xs += [(~(1 << i)) & M for i in range(32)]
    bad = 0
    for i in range(8):
        for x in xs:
            a = BANK_SCRAMBLE[i][1](x)
            b = BANK_A[i][1](x)
            c = BANK_DESCRAMBLE[i][1](x)
            d = BANK_B[i][1](x)
            if a != b or c != d:
                bad += 1
                print('DIFFERS prim %d x=%08x: scr %08x/%08x  des %08x/%08x'
                      % (i, x, a, b, c, d))
            if BANK_DESCRAMBLE[i][1](BANK_SCRAMBLE[i][1](x)) != x:
                bad += 1
                print('not invertible prim %d x=%08x' % (i, x))
    print('self-test DNAS1 vs DNAS2 primitives: %d values x 8 primitives, '
          'differences: %d' % (len(xs), bad))
    return bad == 0


if __name__ == '__main__':
    if len(sys.argv) == 1:
        raise SystemExit(0 if _selftest() else 1)
    addr = int(sys.argv[1], 16)
    n = int(sys.argv[2])
    code = int(sys.argv[3], 16)
    w = read_words(addr, n)
    count, idxs = recipe(code)
    print('code=0x%08x count=%d chain=%s' % (code, count, idxs))
    out = transform(w, code, BANK_DESCRAMBLE)
    for i, (a, b) in enumerate(zip(w, out)):
        print('%08x: %08x -> %08x' % (addr + 4 * i, a, b))
