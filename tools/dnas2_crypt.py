#!/usr/bin/env python3
"""Reconstruction of the self-modifying DNAS2 transform (bn-jpn.elf).

Mechanism (all read from the asm):
  __sce_dnas2_symbol11002(start_or_end, len^code, code, (id<<1)|dir)
     dir=0 -> mprotect RWX; guard=start; 11000(bank B, "decrypt")
     dir=1 -> mprotect RWX; guard=0;     11003(bank A, "encrypt")
  11004(code): count = ((code>>28)&7)==0 ? 5 : (code>>28)&7 ; nibbles 0..6 &= 7
  11003/11000: for i in 0..count-1: prim_bank[(code>>4i)&0xf](region)
  11005(off,count,list): predicate -> 0 if off is in list (skip), 1 otherwise.

Usage: python3 tools/dnas2_crypt.py ADDR NWORDS CODE   (decrypts and dumps)
"""
import struct, sys, importlib.util

M = 0xFFFFFFFF
def rol(x, n): return ((x << n) | (x >> (32 - n))) & M
def ror(x, n): return ((x >> n) | (x << (32 - n))) & M

def bp_a(x):   # 11015 : out.b0=x.b1 out.b1=x.b3 out.b2=x.b0 out.b3=x.b2
    b = [(x >> (8*i)) & 0xff for i in range(4)]
    return (b[1] | (b[3] << 8) | (b[0] << 16) | (b[2] << 24)) & M
def bp_b(x):   # 11040 : inverse of 11015
    b = [(x >> (8*i)) & 0xff for i in range(4)]
    return (b[2] | (b[0] << 8) | (b[3] << 16) | (b[1] << 24)) & M

K0 = 0x09F8ED17   # 11010 / 11036
K1 = 0xA95128C3   # 11011 / 11037

# bank A = table @0x100a8de0 entries 0..7  (dir=1, "encrypt")
BANK_A = {
 0: ('11010 xor K0',   lambda x: x ^ K0),
 1: ('11011 xor K1',   lambda x: x ^ K1),
 2: ('11013 rol13',    lambda x: rol(x, 13)),
 3: ('11014 rol8',     lambda x: rol(x, 8)),
 4: ('11015 byteperm', bp_a),
 5: ('11007 rol13^K0', lambda x: rol(x, 13) ^ K0),
 6: ('11008 bp(rol8^K1)', lambda x: bp_a(rol(x, 8) ^ K1)),
 7: ('11009 rol21(bp)^K0', lambda x: rol(bp_a(x), 21) ^ K0),
}
# bank B = table @0x100a8e00 entries 0..7  (dir=0, "decrypt")
BANK_B = {
 0: ('11036 xor K0',   lambda x: x ^ K0),
 1: ('11037 xor K1',   lambda x: x ^ K1),
 2: ('11038 ror13',    lambda x: ror(x, 13)),
 3: ('11039 ror8',     lambda x: ror(x, 8)),
 4: ('11040 byteperm-1', bp_b),
 5: ('11033 ror13(^K0)', lambda x: ror(x ^ K0, 13)),
 6: ('11034 ror8(bp-1^K1)', lambda x: ror(bp_b(x) ^ K1, 8)),
 7: ('11035 bp-1(ror21(^K0))', lambda x: bp_b(ror(x ^ K0, 21))),
}

def recipe(code):
    """Reproduce 11004 + the loop of 11003/11000: list of primitive indices."""
    top = (code >> 28) & 0xf
    count = 5 if (top & 7) == 0 else (top & 7)
    norm = [(((code >> (4*i)) & 0xf) & 0x7) for i in range(7)]
    return count, [norm[i] for i in range(count)]

def transform(words, code, bank, base_off=0, excl=()):
    count, idxs = recipe(code)
    out = list(words)
    for i in idxs:
        name, fn = bank[i]
        if fn is None:
            raise SystemExit('primitive %d not implemented (%s)' % (i, name))
        for k in range(len(out)):
            if (base_off + 4*k) in excl:
                continue
            out[k] = fn(out[k])
    return out

# ---------------------------------------------------------------- ELF access
spec = importlib.util.spec_from_file_location("gc", "tools/got_callees.py")
gc = importlib.util.module_from_spec(spec); spec.loader.exec_module(gc)
DATA, SECTIONS = gc.load_elf(gc.ELF)
A2O = gc.make_addr_to_off(SECTIONS)
SYMS, SYMINFO = gc.load_symbols(gc.SYMBOL_ADDRS)

def read_words(addr, n):
    return list(struct.unpack_from('<%dI' % n, DATA, A2O(addr)))

if __name__ == '__main__':
    addr = int(sys.argv[1], 16); n = int(sys.argv[2]); code = int(sys.argv[3], 16)
    w = read_words(addr, n)
    count, idxs = recipe(code)
    print('code=0x%08x count=%d chain=%s' % (code, count, idxs))
    out = transform(w, code, BANK_B)
    for i, (a, b) in enumerate(zip(w, out)):
        print('%08x: %08x -> %08x' % (addr + 4*i, a, b))
