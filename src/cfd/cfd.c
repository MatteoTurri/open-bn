/*
 * __cfd_symbol*: DES block cipher (one 8-byte block is one 64-bit value) with
 * single and triple-DES drivers in ECB and CBC modes, used to protect a
 * memory-mapped session file. The PRNG is in cfd_rand.c.
 */

#include "cfd.h"

/* --- DES block cipher ----------------------------------------------------- */

/* DES tables: key schedule rotation counts, S-boxes combined with the P
   permutation, PC2 tables. */
/* clang-format off */
static unsigned char cfd_shifts[16] = {
    1, 1, 2, 2, 2, 2, 2, 2, 1, 2, 2, 2, 2, 2, 2, 1,
};

static unsigned long cfd_SP[8][64] = {
    {
        0x00808200L, 0x00000000L, 0x00008000L, 0x00808202L,
        0x00808002L, 0x00008202L, 0x00000002L, 0x00008000L,
        0x00000200L, 0x00808200L, 0x00808202L, 0x00000200L,
        0x00800202L, 0x00808002L, 0x00800000L, 0x00000002L,
        0x00000202L, 0x00800200L, 0x00800200L, 0x00008200L,
        0x00008200L, 0x00808000L, 0x00808000L, 0x00800202L,
        0x00008002L, 0x00800002L, 0x00800002L, 0x00008002L,
        0x00000000L, 0x00000202L, 0x00008202L, 0x00800000L,
        0x00008000L, 0x00808202L, 0x00000002L, 0x00808000L,
        0x00808200L, 0x00800000L, 0x00800000L, 0x00000200L,
        0x00808002L, 0x00008000L, 0x00008200L, 0x00800002L,
        0x00000200L, 0x00000002L, 0x00800202L, 0x00008202L,
        0x00808202L, 0x00008002L, 0x00808000L, 0x00800202L,
        0x00800002L, 0x00000202L, 0x00008202L, 0x00808200L,
        0x00000202L, 0x00800200L, 0x00800200L, 0x00000000L,
        0x00008002L, 0x00008200L, 0x00000000L, 0x00808002L
    },
    {
        0x40084010L, 0x40004000L, 0x00004000L, 0x00084010L,
        0x00080000L, 0x00000010L, 0x40080010L, 0x40004010L,
        0x40000010L, 0x40084010L, 0x40084000L, 0x40000000L,
        0x40004000L, 0x00080000L, 0x00000010L, 0x40080010L,
        0x00084000L, 0x00080010L, 0x40004010L, 0x00000000L,
        0x40000000L, 0x00004000L, 0x00084010L, 0x40080000L,
        0x00080010L, 0x40000010L, 0x00000000L, 0x00084000L,
        0x00004010L, 0x40084000L, 0x40080000L, 0x00004010L,
        0x00000000L, 0x00084010L, 0x40080010L, 0x00080000L,
        0x40004010L, 0x40080000L, 0x40084000L, 0x00004000L,
        0x40080000L, 0x40004000L, 0x00000010L, 0x40084010L,
        0x00084010L, 0x00000010L, 0x00004000L, 0x40000000L,
        0x00004010L, 0x40084000L, 0x00080000L, 0x40000010L,
        0x00080010L, 0x40004010L, 0x40000010L, 0x00080010L,
        0x00084000L, 0x00000000L, 0x40004000L, 0x00004010L,
        0x40000000L, 0x40080010L, 0x40084010L, 0x00084000L
    },
    {
        0x00000104L, 0x04010100L, 0x00000000L, 0x04010004L,
        0x04000100L, 0x00000000L, 0x00010104L, 0x04000100L,
        0x00010004L, 0x04000004L, 0x04000004L, 0x00010000L,
        0x04010104L, 0x00010004L, 0x04010000L, 0x00000104L,
        0x04000000L, 0x00000004L, 0x04010100L, 0x00000100L,
        0x00010100L, 0x04010000L, 0x04010004L, 0x00010104L,
        0x04000104L, 0x00010100L, 0x00010000L, 0x04000104L,
        0x00000004L, 0x04010104L, 0x00000100L, 0x04000000L,
        0x04010100L, 0x04000000L, 0x00010004L, 0x00000104L,
        0x00010000L, 0x04010100L, 0x04000100L, 0x00000000L,
        0x00000100L, 0x00010004L, 0x04010104L, 0x04000100L,
        0x04000004L, 0x00000100L, 0x00000000L, 0x04010004L,
        0x04000104L, 0x00010000L, 0x04000000L, 0x04010104L,
        0x00000004L, 0x00010104L, 0x00010100L, 0x04000004L,
        0x04010000L, 0x04000104L, 0x00000104L, 0x04010000L,
        0x00010104L, 0x00000004L, 0x04010004L, 0x00010100L
    },
    {
        0x80401000L, 0x80001040L, 0x80001040L, 0x00000040L,
        0x00401040L, 0x80400040L, 0x80400000L, 0x80001000L,
        0x00000000L, 0x00401000L, 0x00401000L, 0x80401040L,
        0x80000040L, 0x00000000L, 0x00400040L, 0x80400000L,
        0x80000000L, 0x00001000L, 0x00400000L, 0x80401000L,
        0x00000040L, 0x00400000L, 0x80001000L, 0x00001040L,
        0x80400040L, 0x80000000L, 0x00001040L, 0x00400040L,
        0x00001000L, 0x00401040L, 0x80401040L, 0x80000040L,
        0x00400040L, 0x80400000L, 0x00401000L, 0x80401040L,
        0x80000040L, 0x00000000L, 0x00000000L, 0x00401000L,
        0x00001040L, 0x00400040L, 0x80400040L, 0x80000000L,
        0x80401000L, 0x80001040L, 0x80001040L, 0x00000040L,
        0x80401040L, 0x80000040L, 0x80000000L, 0x00001000L,
        0x80400000L, 0x80001000L, 0x00401040L, 0x80400040L,
        0x80001000L, 0x00001040L, 0x00400000L, 0x80401000L,
        0x00000040L, 0x00400000L, 0x00001000L, 0x00401040L
    },
    {
        0x00000080L, 0x01040080L, 0x01040000L, 0x21000080L,
        0x00040000L, 0x00000080L, 0x20000000L, 0x01040000L,
        0x20040080L, 0x00040000L, 0x01000080L, 0x20040080L,
        0x21000080L, 0x21040000L, 0x00040080L, 0x20000000L,
        0x01000000L, 0x20040000L, 0x20040000L, 0x00000000L,
        0x20000080L, 0x21040080L, 0x21040080L, 0x01000080L,
        0x21040000L, 0x20000080L, 0x00000000L, 0x21000000L,
        0x01040080L, 0x01000000L, 0x21000000L, 0x00040080L,
        0x00040000L, 0x21000080L, 0x00000080L, 0x01000000L,
        0x20000000L, 0x01040000L, 0x21000080L, 0x20040080L,
        0x01000080L, 0x20000000L, 0x21040000L, 0x01040080L,
        0x20040080L, 0x00000080L, 0x01000000L, 0x21040000L,
        0x21040080L, 0x00040080L, 0x21000000L, 0x21040080L,
        0x01040000L, 0x00000000L, 0x20040000L, 0x21000000L,
        0x00040080L, 0x01000080L, 0x20000080L, 0x00040000L,
        0x00000000L, 0x20040000L, 0x01040080L, 0x20000080L
    },
    {
        0x10000008L, 0x10200000L, 0x00002000L, 0x10202008L,
        0x10200000L, 0x00000008L, 0x10202008L, 0x00200000L,
        0x10002000L, 0x00202008L, 0x00200000L, 0x10000008L,
        0x00200008L, 0x10002000L, 0x10000000L, 0x00002008L,
        0x00000000L, 0x00200008L, 0x10002008L, 0x00002000L,
        0x00202000L, 0x10002008L, 0x00000008L, 0x10200008L,
        0x10200008L, 0x00000000L, 0x00202008L, 0x10202000L,
        0x00002008L, 0x00202000L, 0x10202000L, 0x10000000L,
        0x10002000L, 0x00000008L, 0x10200008L, 0x00202000L,
        0x10202008L, 0x00200000L, 0x00002008L, 0x10000008L,
        0x00200000L, 0x10002000L, 0x10000000L, 0x00002008L,
        0x10000008L, 0x10202008L, 0x00202000L, 0x10200000L,
        0x00202008L, 0x10202000L, 0x00000000L, 0x10200008L,
        0x00000008L, 0x00002000L, 0x10200000L, 0x00202008L,
        0x00002000L, 0x00200008L, 0x10002008L, 0x00000000L,
        0x10202000L, 0x10000000L, 0x00200008L, 0x10002008L
    },
    {
        0x00100000L, 0x02100001L, 0x02000401L, 0x00000000L,
        0x00000400L, 0x02000401L, 0x00100401L, 0x02100400L,
        0x02100401L, 0x00100000L, 0x00000000L, 0x02000001L,
        0x00000001L, 0x02000000L, 0x02100001L, 0x00000401L,
        0x02000400L, 0x00100401L, 0x00100001L, 0x02000400L,
        0x02000001L, 0x02100000L, 0x02100400L, 0x00100001L,
        0x02100000L, 0x00000400L, 0x00000401L, 0x02100401L,
        0x00100400L, 0x00000001L, 0x02000000L, 0x00100400L,
        0x02000000L, 0x00100400L, 0x00100000L, 0x02000401L,
        0x02000401L, 0x02100001L, 0x02100001L, 0x00000001L,
        0x00100001L, 0x02000000L, 0x02000400L, 0x00100000L,
        0x02100400L, 0x00000401L, 0x00100401L, 0x02100400L,
        0x00000401L, 0x02000001L, 0x02100401L, 0x02100000L,
        0x00100400L, 0x00000000L, 0x00000001L, 0x02100401L,
        0x00000000L, 0x00100401L, 0x02100000L, 0x00000400L,
        0x02000001L, 0x02000400L, 0x00000400L, 0x00100001L
    },
    {
        0x08000820L, 0x00000800L, 0x00020000L, 0x08020820L,
        0x08000000L, 0x08000820L, 0x00000020L, 0x08000000L,
        0x00020020L, 0x08020000L, 0x08020820L, 0x00020800L,
        0x08020800L, 0x00020820L, 0x00000800L, 0x00000020L,
        0x08020000L, 0x08000020L, 0x08000800L, 0x00000820L,
        0x00020800L, 0x00020020L, 0x08020020L, 0x08020800L,
        0x00000820L, 0x00000000L, 0x00000000L, 0x08020020L,
        0x08000020L, 0x08000800L, 0x00020820L, 0x00020000L,
        0x00020820L, 0x00020000L, 0x08020800L, 0x00000800L,
        0x00000020L, 0x08020020L, 0x00000800L, 0x00020820L,
        0x08000800L, 0x00000020L, 0x08000020L, 0x08020000L,
        0x08020020L, 0x08000000L, 0x00020000L, 0x08000820L,
        0x00000000L, 0x08020820L, 0x00020020L, 0x08000020L,
        0x08020000L, 0x08000800L, 0x08000820L, 0x00000000L,
        0x08020820L, 0x00020800L, 0x00020800L, 0x00000820L,
        0x00000820L, 0x00020020L, 0x08000000L, 0x08020800L
    },
};

static unsigned long cfd_skb[8][64] = {
    {
        0x00000000L, 0x00000400L, 0x01000000L, 0x01000400L,
        0x00040000L, 0x00040400L, 0x01040000L, 0x01040400L,
        0x00002000L, 0x00002400L, 0x01002000L, 0x01002400L,
        0x00042000L, 0x00042400L, 0x01042000L, 0x01042400L,
        0x00000001L, 0x00000401L, 0x01000001L, 0x01000401L,
        0x00040001L, 0x00040401L, 0x01040001L, 0x01040401L,
        0x00002001L, 0x00002401L, 0x01002001L, 0x01002401L,
        0x00042001L, 0x00042401L, 0x01042001L, 0x01042401L,
        0x02000000L, 0x02000400L, 0x03000000L, 0x03000400L,
        0x02040000L, 0x02040400L, 0x03040000L, 0x03040400L,
        0x02002000L, 0x02002400L, 0x03002000L, 0x03002400L,
        0x02042000L, 0x02042400L, 0x03042000L, 0x03042400L,
        0x02000001L, 0x02000401L, 0x03000001L, 0x03000401L,
        0x02040001L, 0x02040401L, 0x03040001L, 0x03040401L,
        0x02002001L, 0x02002401L, 0x03002001L, 0x03002401L,
        0x02042001L, 0x02042401L, 0x03042001L, 0x03042401L
    },
    {
        0x00000000L, 0x00000002L, 0x00080000L, 0x00080002L,
        0x08000000L, 0x08000002L, 0x08080000L, 0x08080002L,
        0x00000100L, 0x00000102L, 0x00080100L, 0x00080102L,
        0x08000100L, 0x08000102L, 0x08080100L, 0x08080102L,
        0x00010000L, 0x00010002L, 0x00090000L, 0x00090002L,
        0x08010000L, 0x08010002L, 0x08090000L, 0x08090002L,
        0x00010100L, 0x00010102L, 0x00090100L, 0x00090102L,
        0x08010100L, 0x08010102L, 0x08090100L, 0x08090102L,
        0x00000010L, 0x00000012L, 0x00080010L, 0x00080012L,
        0x08000010L, 0x08000012L, 0x08080010L, 0x08080012L,
        0x00000110L, 0x00000112L, 0x00080110L, 0x00080112L,
        0x08000110L, 0x08000112L, 0x08080110L, 0x08080112L,
        0x00010010L, 0x00010012L, 0x00090010L, 0x00090012L,
        0x08010010L, 0x08010012L, 0x08090010L, 0x08090012L,
        0x00010110L, 0x00010112L, 0x00090110L, 0x00090112L,
        0x08010110L, 0x08010112L, 0x08090110L, 0x08090112L
    },
    {
        0x00000000L, 0x00000004L, 0x00100000L, 0x00100004L,
        0x10000000L, 0x10000004L, 0x10100000L, 0x10100004L,
        0x00000020L, 0x00000024L, 0x00100020L, 0x00100024L,
        0x10000020L, 0x10000024L, 0x10100020L, 0x10100024L,
        0x00000800L, 0x00000804L, 0x00100800L, 0x00100804L,
        0x10000800L, 0x10000804L, 0x10100800L, 0x10100804L,
        0x00000820L, 0x00000824L, 0x00100820L, 0x00100824L,
        0x10000820L, 0x10000824L, 0x10100820L, 0x10100824L,
        0x20000000L, 0x20000004L, 0x20100000L, 0x20100004L,
        0x30000000L, 0x30000004L, 0x30100000L, 0x30100004L,
        0x20000020L, 0x20000024L, 0x20100020L, 0x20100024L,
        0x30000020L, 0x30000024L, 0x30100020L, 0x30100024L,
        0x20000800L, 0x20000804L, 0x20100800L, 0x20100804L,
        0x30000800L, 0x30000804L, 0x30100800L, 0x30100804L,
        0x20000820L, 0x20000824L, 0x20100820L, 0x20100824L,
        0x30000820L, 0x30000824L, 0x30100820L, 0x30100824L
    },
    {
        0x00000000L, 0x00001000L, 0x00000008L, 0x00001008L,
        0x00020000L, 0x00021000L, 0x00020008L, 0x00021008L,
        0x04000000L, 0x04001000L, 0x04000008L, 0x04001008L,
        0x04020000L, 0x04021000L, 0x04020008L, 0x04021008L,
        0x00200000L, 0x00201000L, 0x00200008L, 0x00201008L,
        0x00220000L, 0x00221000L, 0x00220008L, 0x00221008L,
        0x04200000L, 0x04201000L, 0x04200008L, 0x04201008L,
        0x04220000L, 0x04221000L, 0x04220008L, 0x04221008L,
        0x00000200L, 0x00001200L, 0x00000208L, 0x00001208L,
        0x00020200L, 0x00021200L, 0x00020208L, 0x00021208L,
        0x04000200L, 0x04001200L, 0x04000208L, 0x04001208L,
        0x04020200L, 0x04021200L, 0x04020208L, 0x04021208L,
        0x00200200L, 0x00201200L, 0x00200208L, 0x00201208L,
        0x00220200L, 0x00221200L, 0x00220208L, 0x00221208L,
        0x04200200L, 0x04201200L, 0x04200208L, 0x04201208L,
        0x04220200L, 0x04221200L, 0x04220208L, 0x04221208L
    },
    {
        0x00000000L, 0x00020000L, 0x00000200L, 0x00020200L,
        0x00000001L, 0x00020001L, 0x00000201L, 0x00020201L,
        0x08000000L, 0x08020000L, 0x08000200L, 0x08020200L,
        0x08000001L, 0x08020001L, 0x08000201L, 0x08020201L,
        0x00002000L, 0x00022000L, 0x00002200L, 0x00022200L,
        0x00002001L, 0x00022001L, 0x00002201L, 0x00022201L,
        0x08002000L, 0x08022000L, 0x08002200L, 0x08022200L,
        0x08002001L, 0x08022001L, 0x08002201L, 0x08022201L,
        0x00000002L, 0x00020002L, 0x00000202L, 0x00020202L,
        0x00000003L, 0x00020003L, 0x00000203L, 0x00020203L,
        0x08000002L, 0x08020002L, 0x08000202L, 0x08020202L,
        0x08000003L, 0x08020003L, 0x08000203L, 0x08020203L,
        0x00002002L, 0x00022002L, 0x00002202L, 0x00022202L,
        0x00002003L, 0x00022003L, 0x00002203L, 0x00022203L,
        0x08002002L, 0x08022002L, 0x08002202L, 0x08022202L,
        0x08002003L, 0x08022003L, 0x08002203L, 0x08022203L
    },
    {
        0x00000000L, 0x00000010L, 0x20000000L, 0x20000010L,
        0x00001000L, 0x00001010L, 0x20001000L, 0x20001010L,
        0x00080000L, 0x00080010L, 0x20080000L, 0x20080010L,
        0x00081000L, 0x00081010L, 0x20081000L, 0x20081010L,
        0x04000000L, 0x04000010L, 0x24000000L, 0x24000010L,
        0x04001000L, 0x04001010L, 0x24001000L, 0x24001010L,
        0x04080000L, 0x04080010L, 0x24080000L, 0x24080010L,
        0x04081000L, 0x04081010L, 0x24081000L, 0x24081010L,
        0x00000004L, 0x00000014L, 0x20000004L, 0x20000014L,
        0x00001004L, 0x00001014L, 0x20001004L, 0x20001014L,
        0x00080004L, 0x00080014L, 0x20080004L, 0x20080014L,
        0x00081004L, 0x00081014L, 0x20081004L, 0x20081014L,
        0x04000004L, 0x04000014L, 0x24000004L, 0x24000014L,
        0x04001004L, 0x04001014L, 0x24001004L, 0x24001014L,
        0x04080004L, 0x04080014L, 0x24080004L, 0x24080014L,
        0x04081004L, 0x04081014L, 0x24081004L, 0x24081014L
    },
    {
        0x00000000L, 0x00100000L, 0x00000100L, 0x00100100L,
        0x02000000L, 0x02100000L, 0x02000100L, 0x02100100L,
        0x00000020L, 0x00100020L, 0x00000120L, 0x00100120L,
        0x02000020L, 0x02100020L, 0x02000120L, 0x02100120L,
        0x00000400L, 0x00100400L, 0x00000500L, 0x00100500L,
        0x02000400L, 0x02100400L, 0x02000500L, 0x02100500L,
        0x00000420L, 0x00100420L, 0x00000520L, 0x00100520L,
        0x02000420L, 0x02100420L, 0x02000520L, 0x02100520L,
        0x00200000L, 0x00300000L, 0x00200100L, 0x00300100L,
        0x02200000L, 0x02300000L, 0x02200100L, 0x02300100L,
        0x00200020L, 0x00300020L, 0x00200120L, 0x00300120L,
        0x02200020L, 0x02300020L, 0x02200120L, 0x02300120L,
        0x00200400L, 0x00300400L, 0x00200500L, 0x00300500L,
        0x02200400L, 0x02300400L, 0x02200500L, 0x02300500L,
        0x00200420L, 0x00300420L, 0x00200520L, 0x00300520L,
        0x02200420L, 0x02300420L, 0x02200520L, 0x02300520L
    },
    {
        0x00000000L, 0x00040000L, 0x01000000L, 0x01040000L,
        0x00010000L, 0x00050000L, 0x01010000L, 0x01050000L,
        0x10000000L, 0x10040000L, 0x11000000L, 0x11040000L,
        0x10010000L, 0x10050000L, 0x11010000L, 0x11050000L,
        0x00000800L, 0x00040800L, 0x01000800L, 0x01040800L,
        0x00010800L, 0x00050800L, 0x01010800L, 0x01050800L,
        0x10000800L, 0x10040800L, 0x11000800L, 0x11040800L,
        0x10010800L, 0x10050800L, 0x11010800L, 0x11050800L,
        0x00000008L, 0x00040008L, 0x01000008L, 0x01040008L,
        0x00010008L, 0x00050008L, 0x01010008L, 0x01050008L,
        0x10000008L, 0x10040008L, 0x11000008L, 0x11040008L,
        0x10010008L, 0x10050008L, 0x11010008L, 0x11050008L,
        0x00000808L, 0x00040808L, 0x01000808L, 0x01040808L,
        0x00010808L, 0x00050808L, 0x01010808L, 0x01050808L,
        0x10000808L, 0x10040808L, 0x11000808L, 0x11040808L,
        0x10010808L, 0x10050808L, 0x11010808L, 0x11050808L
    },
};
/* clang-format on */

#define CFD_PERM_OP(a, b, t, n, m) \
    ((t) = ((((a) >> (n)) ^ (b)) & (m)), (b) ^= (t), (a) ^= ((t) << (n)))

#define CFD_HPERM_OP(a, t, n, m) \
    ((t) = ((((a) << (16 - (n))) ^ (a)) & (m)), \
     (a) = (a) ^ (t) ^ ((t) >> (16 - (n))))

/* One round: R enters rotated right by 3 (with the odd subkey word) and left
   by 1 (with the even one). */
#define CFD_D_ENCRYPT(L, R, S) \
    u = ks[(S) + 1] ^ (((R) << 29) | ((R) >> 3)); \
    t = (((R) << 1) | ((R) >> 31)) ^ ks[(S)]; \
    (L) ^= cfd_SP[0][(u >> 24) & 0x3f] | cfd_SP[1][(t >> 24) & 0x3f] | \
           cfd_SP[2][(u >> 16) & 0x3f] | cfd_SP[3][(t >> 16) & 0x3f] | \
           cfd_SP[4][(u >> 8) & 0x3f] | cfd_SP[5][(t >> 8) & 0x3f] | \
           cfd_SP[6][u & 0x3f] | cfd_SP[7][t & 0x3f]

#define CFD_IP(l, r, w) \
    CFD_PERM_OP(r, l, w, 4, 0x0f0f0f0fL); \
    CFD_PERM_OP(r, l, w, 16, 0x0000ffffL); \
    CFD_PERM_OP(l, r, w, 2, 0x33333333L); \
    CFD_PERM_OP(l, r, w, 8, 0x00ff00ffL); \
    CFD_PERM_OP(r, l, w, 1, 0x55555555L)

#define CFD_FP(l, r, w) \
    CFD_PERM_OP(l, r, w, 1, 0x55555555L); \
    CFD_PERM_OP(r, l, w, 8, 0x00ff00ffL); \
    CFD_PERM_OP(r, l, w, 2, 0x33333333L); \
    CFD_PERM_OP(l, r, w, 16, 0x0000ffffL); \
    CFD_PERM_OP(l, r, w, 4, 0x0f0f0f0fL)

/* DES encryption of one block (subkeys in forward order). */
static void cfd_des_encrypt_block(unsigned long long v, unsigned long long *out, unsigned long *ks)
{
	unsigned long l, r, t, u;

	r = (unsigned long)(v >> 32);
	l = (unsigned long)v;

	CFD_IP(l, r, u);

	CFD_D_ENCRYPT(r, l, 0);
	CFD_D_ENCRYPT(l, r, 2);
	CFD_D_ENCRYPT(r, l, 4);
	CFD_D_ENCRYPT(l, r, 6);
	CFD_D_ENCRYPT(r, l, 8);
	CFD_D_ENCRYPT(l, r, 10);
	CFD_D_ENCRYPT(r, l, 12);
	CFD_D_ENCRYPT(l, r, 14);
	CFD_D_ENCRYPT(r, l, 16);
	CFD_D_ENCRYPT(l, r, 18);
	CFD_D_ENCRYPT(r, l, 20);
	CFD_D_ENCRYPT(l, r, 22);
	CFD_D_ENCRYPT(r, l, 24);
	CFD_D_ENCRYPT(l, r, 26);
	CFD_D_ENCRYPT(r, l, 28);
	CFD_D_ENCRYPT(l, r, 30);

	CFD_FP(l, r, u);

	*out = r | ((unsigned long long)l << 32);
}

/* DES decryption of one block (subkeys in reverse order). */
static void cfd_des_decrypt_block(unsigned long long v, unsigned long long *out, unsigned long *ks)
{
	unsigned long l, r, t, u;

	r = (unsigned long)(v >> 32);
	l = (unsigned long)v;

	CFD_IP(l, r, u);

	CFD_D_ENCRYPT(r, l, 30);
	CFD_D_ENCRYPT(l, r, 28);
	CFD_D_ENCRYPT(r, l, 26);
	CFD_D_ENCRYPT(l, r, 24);
	CFD_D_ENCRYPT(r, l, 22);
	CFD_D_ENCRYPT(l, r, 20);
	CFD_D_ENCRYPT(r, l, 18);
	CFD_D_ENCRYPT(l, r, 16);
	CFD_D_ENCRYPT(r, l, 14);
	CFD_D_ENCRYPT(l, r, 12);
	CFD_D_ENCRYPT(r, l, 10);
	CFD_D_ENCRYPT(l, r, 8);
	CFD_D_ENCRYPT(r, l, 6);
	CFD_D_ENCRYPT(l, r, 4);
	CFD_D_ENCRYPT(r, l, 2);
	CFD_D_ENCRYPT(l, r, 0);

	CFD_FP(l, r, u);

	*out = r | ((unsigned long long)l << 32);
}

/* Rotates a 28-bit key half (C or D) left by 1 or 2 bits. */
static unsigned int cfd_rotl28(unsigned int x, int shift)
{
	if (shift == 1)
		return (x >> 27) | (x << 1);
	return (x >> 26) | (x << 2);
}

/* DES key schedule: 16 64-bit subkeys. */
static void cfd_des_key_schedule(unsigned long long key, unsigned long long *out)
{
	unsigned long c, d, s, t;
	int i;

	c = (unsigned long)(key >> 32);
	d = (unsigned long)key;

	CFD_PERM_OP(d, c, s, 4, 0x0f0f0f0fL);
	CFD_HPERM_OP(c, s, -2, 0xcccc0000L);
	CFD_HPERM_OP(d, s, -2, 0xcccc0000L);
	CFD_PERM_OP(d, c, s, 1, 0x55555555L);
	CFD_PERM_OP(c, d, s, 8, 0x00ff00ffL);
	CFD_PERM_OP(d, c, s, 1, 0x55555555L);
	CFD_HPERM_OP(c, s, 12, 0xf0f0f0f0L);
	CFD_HPERM_OP(d, s, 12, 0xf0f0f0f0L);
	d = ((d & 0x00ffffffL) << 4) | ((c >> 24) & 0x0fL);
	c = ((c & 0x000000ffL) << 20) | ((c & 0x0000ff00L) << 4) | ((c & 0x00ff0000L) >> 12)
	    | ((c & 0xf0000000L) >> 28);

	for (i = 0; i < 16; i++) {
		c = cfd_rotl28(c, cfd_shifts[i]);
		d = cfd_rotl28(d, cfd_shifts[i]);
		s = cfd_skb[0][(c >> 22) & 0x3f] | cfd_skb[1][((c >> 16) & 0x30) | ((c >> 15) & 0x0f)]
		    | cfd_skb[2][((c >> 9) & 0x3c) | ((c >> 8) & 0x03)]
		    | cfd_skb[3][((c >> 2) & 0x20) | ((c >> 1) & 0x18) | (c & 0x07)];
		t = cfd_skb[4][(d >> 22) & 0x3f] | cfd_skb[5][((d >> 15) & 0x30) | ((d >> 14) & 0x0f)]
		    | cfd_skb[6][(d >> 7) & 0x3f] | cfd_skb[7][((d >> 1) & 0x3c) | (d & 0x03)];
		out[i] = (((unsigned long long)s << 32) & 0xffff000000000000ULL)
		    | (((unsigned long long)t << 16) & 0x0000ffff00000000ULL)
		    | (((unsigned long long)s << 16) & 0x00000000ffff0000ULL) | (t & 0x0000ffffL);
	}
}

/* Loads the 8-byte big-endian key at `src` and builds its key schedule in `dst`. */
void __cfd_symbol16(unsigned char *src, void *dst)
{
	unsigned long long v = 0;
	int i;

	for (i = 0; i < 8; i++) {
		v <<= 8;
		v |= src[i];
	}
	cfd_des_key_schedule(v, dst);
}

/* DES-encrypts one block with a key schedule: (in, schedule, out). */
void __cfd_symbol15(unsigned char *src, void *ks, unsigned char *dst)
{
	unsigned long long v = 0;
	unsigned long long state;
	int i;

	for (i = 0; i < 8; i++) {
		v <<= 8;
		v |= src[i];
	}
	cfd_des_encrypt_block(v, &state, ks);

	for (i = 0; i < 8; i++) {
		dst[7 - i] = (unsigned char)state;
		state >>= 8;
	}
}

/* DES-decrypts one block with a key schedule: (in, schedule, out). */
void __cfd_symbol12(unsigned char *src, void *ks, unsigned char *dst)
{
	unsigned long long v = 0;
	unsigned long long state;
	int i;

	for (i = 0; i < 8; i++) {
		v <<= 8;
		v |= src[i];
	}
	cfd_des_decrypt_block(v, &state, ks);

	for (i = 0; i < 8; i++) {
		dst[7 - i] = (unsigned char)state;
		state >>= 8;
	}
}

/* DES-ECB encryption of `count` blocks: (in, key, out, count). */
void __cfd_symbol1(unsigned char *src, unsigned char *key, unsigned char *dst, unsigned int count)
{
	unsigned int ks[32];
	unsigned int i;

	__cfd_symbol16(key, ks);
	for (i = 0; i < count; i++)
		__cfd_symbol15(src + i * 8, ks, dst + i * 8);
}

/* --- Drivers: key schedule via __cfd_symbol16, then one call per block ---- */

/* DES-ECB decryption of `count` blocks: (in, key, out, count). */
void __cfd_symbol0(unsigned char *src, unsigned char *key, unsigned char *dst, unsigned int count)
{
	unsigned int ks[32];
	unsigned int i;

	__cfd_symbol16(key, ks);
	for (i = 0; i < count; i++)
		__cfd_symbol12(src + i * 8, ks, dst + i * 8);
}

/* DES-CBC encryption: (in, key, out, iv, count). */
void __cfd_symbol11(unsigned char *src, unsigned char *key, unsigned char *dst, unsigned char *iv, unsigned int count)
{
	unsigned char block[16];
	unsigned int ks[32];
	unsigned int i;
	unsigned int j;

	__cfd_symbol16(key, ks);

	j = 0;
	do {
		block[j] = src[j] ^ iv[j];
		j++;
	} while (j < 8);
	__cfd_symbol15(block, ks, dst);

	i = 1;
	while (i < count) {
		j = 0;
		do {
			block[j] = src[i * 8 + j] ^ dst[(i - 1) * 8 + j];
			j++;
		} while (j < 8);
		__cfd_symbol15(block, ks, dst + i * 8);
		i++;
	}
}

/* DES-CBC decryption: (in, key, out, iv, count); `in` and `out` must not overlap. */
void __cfd_symbol9(unsigned char *src, unsigned char *key, unsigned char *dst, unsigned char *iv, unsigned int count)
{
	unsigned char block[16];
	unsigned int ks[32];
	unsigned int i;
	unsigned int j;

	__cfd_symbol16(key, ks);

	__cfd_symbol12(src, ks, block);
	j = 0;
	do {
		dst[j] = block[j] ^ iv[j];
		j++;
	} while (j < 8);

	i = 1;
	while (i < count) {
		__cfd_symbol12(src + i * 8, ks, block);
		j = 0;
		do {
			dst[i * 8 + j] = block[j] ^ src[(i - 1) * 8 + j];
			j++;
		} while (j < 8);
		i++;
	}
}

/* Triple-DES (EDE) encryption of one block with three key schedules:
   (in, k1, k2, k3, out). */
void __cfd_symbol39(unsigned char *src, void *ks1, void *ks2, void *ks3, unsigned char *dst)
{
	unsigned long long v = 0;
	unsigned long long state;
	int i;

	for (i = 0; i < 8; i++) {
		v <<= 8;
		v |= src[i];
	}

	cfd_des_encrypt_block(v, &state, ks1);
	cfd_des_decrypt_block(state, &state, ks2);
	cfd_des_encrypt_block(state, &state, ks3);

	for (i = 0; i < 8; i++) {
		dst[7 - i] = (unsigned char)state;
		state >>= 8;
	}
}

/* Triple-DES decryption of one block, the inverse of __cfd_symbol39. */
void __cfd_symbol36(unsigned char *src, void *ks1, void *ks2, void *ks3, unsigned char *dst)
{
	unsigned long long v = 0;
	unsigned long long state;
	int i;

	for (i = 0; i < 8; i++) {
		v <<= 8;
		v |= src[i];
	}

	cfd_des_decrypt_block(v, &state, ks3);
	cfd_des_encrypt_block(state, &state, ks2);
	cfd_des_decrypt_block(state, &state, ks1);

	for (i = 0; i < 8; i++) {
		dst[7 - i] = (unsigned char)state;
		state >>= 8;
	}
}

/* --- Triple DES with two keys (32, 29) or one key (25, 22) ---------------- */

/* Two-key triple-DES encryption of one block: (in, k1, k2, out). */
void __cfd_symbol32(unsigned char *src, void *ks1, void *ks2, void *dst)
{
	__cfd_symbol39(src, ks1, ks2, ks1, (unsigned char *)dst);
}

/* Two-key triple-DES decryption of one block: (in, k1, k2, out). */
void __cfd_symbol29(unsigned char *src, void *ks1, void *ks2, void *dst)
{
	__cfd_symbol36(src, ks1, ks2, ks1, (unsigned char *)dst);
}

/* EDE with one key schedule (same result as DES): (in, k, out). */
void __cfd_symbol25(unsigned char *src, void *ks, void *dst)
{
	__cfd_symbol39(src, ks, ks, ks, (unsigned char *)dst);
}

/* Inverse of __cfd_symbol25: (in, k, out). */
void __cfd_symbol22(unsigned char *src, void *ks, void *dst)
{
	__cfd_symbol36(src, ks, ks, ks, (unsigned char *)dst);
}

/* Triple-DES-ECB encryption: (in, key1, key2, key3, out, count). */
void __cfd_symbol38(unsigned char *src, unsigned char *key1, unsigned char *key2, unsigned char *key3,
    unsigned char *dst, unsigned int count)
{
	unsigned int ks1[32];
	unsigned int ks2[32];
	unsigned int ks3[32];
	unsigned int i;

	if (count != 0) {
		__cfd_symbol16(key1, ks1);
		__cfd_symbol16(key2, ks2);
		__cfd_symbol16(key3, ks3);
		for (i = 0; i < count; i++)
			__cfd_symbol39(src + i * 8, ks1, ks2, ks3, dst + i * 8);
	}
}

/* Inputs of the key derivation in __cfd_symbol2. */
/* clang-format off */
static unsigned char cfd_symbol2_lit1[32] = {
    0x9c, 0xca, 0x9c, 0x00, 0x02, 0x86, 0x0d, 0x38,
    0x6e, 0x85, 0x3f, 0xf7, 0x13, 0x9e, 0xd2, 0x3a,
    0xa6, 0x50, 0xb7, 0x54, 0x89, 0x6b, 0x26, 0xb6,
    0xdc, 0x23, 0x06, 0x11, 0x39, 0x9b, 0x58, 0xd5,
};

static unsigned char cfd_symbol2_lit2[8] = {
    0x11, 0x86, 0x35, 0x55, 0x63, 0x38, 0xe9, 0x0d,
};
/* clang-format on */

/* Triple-DES-ECB decryption: (in, key1, key2, key3, out, count). */
void __cfd_symbol37(unsigned char *src, unsigned char *key1, unsigned char *key2, unsigned char *key3,
    unsigned char *dst, unsigned int count)
{
	unsigned int ks1[32];
	unsigned int ks2[32];
	unsigned int ks3[32];
	unsigned int i;

	if (count != 0) {
		__cfd_symbol16(key1, ks1);
		__cfd_symbol16(key2, ks2);
		__cfd_symbol16(key3, ks3);
		for (i = 0; i < count; i++)
			__cfd_symbol36(src + i * 8, ks1, ks2, ks3, dst + i * 8);
	}
}

/* Two-key triple-DES-ECB encryption: (in, key1, key2, out, count). */
void __cfd_symbol31(unsigned char *src, unsigned char *key1, unsigned char *key2, unsigned char *dst,
    unsigned int count)
{
	unsigned int i;
	unsigned int ks1[32];
	unsigned int ks2[32];

	if (count != 0) {
		__cfd_symbol16(key1, ks1);
		__cfd_symbol16(key2, ks2);
		for (i = 0; i < count; i++)
			__cfd_symbol32(src + i * 8, ks1, ks2, dst + i * 8);
	}
}

/* Two-key triple-DES-ECB decryption: (in, key1, key2, out, count). */
void __cfd_symbol30(unsigned char *src, unsigned char *key1, unsigned char *key2, unsigned char *dst,
    unsigned int count)
{
	unsigned int ks1[32];
	unsigned int ks2[32];
	unsigned int i;

	if (count != 0) {
		__cfd_symbol16(key1, ks1);
		__cfd_symbol16(key2, ks2);
		for (i = 0; i < count; i++)
			__cfd_symbol29(src + i * 8, ks1, ks2, dst + i * 8);
	}
}

/* One-key EDE-ECB encryption: (in, key, out, count). */
void __cfd_symbol24(unsigned char *src, unsigned char *key, void *dst, unsigned int count)
{
	unsigned int ks[32];
	unsigned int i;

	if (count != 0) {
		__cfd_symbol16(key, ks);
		for (i = 0; i < count; i++)
			__cfd_symbol25(src + i * 8, ks, (unsigned char *)dst + i * 8);
	}
}

/* One-key EDE-ECB decryption: (in, key, out, count). */
void __cfd_symbol23(unsigned char *src, unsigned char *key, void *dst, unsigned int count)
{
	unsigned int ks[32];
	unsigned int i;

	if (count != 0) {
		__cfd_symbol16(key, ks);
		for (i = 0; i < count; i++)
			__cfd_symbol22(src + i * 8, ks, (unsigned char *)dst + i * 8);
	}
}

/* Triple-DES-CBC encryption: (in, key1, key2, key3, out, iv, count). */
void __cfd_symbol35(unsigned char *src, unsigned char *key1, unsigned char *key2, unsigned char *key3,
    unsigned char *dst, unsigned char *iv, unsigned int count)
{
	unsigned char block[16];
	unsigned int ks1[32];
	unsigned int ks2[32];
	unsigned int ks3[32];
	unsigned int i, j;

	if (count != 0) {
		__cfd_symbol16(key1, ks1);
		__cfd_symbol16(key2, ks2);
		__cfd_symbol16(key3, ks3);
		for (i = 0; i < 8; i++)
			block[i] = src[i] ^ iv[i];
		__cfd_symbol39(block, ks1, ks2, ks3, dst);
		for (j = 1; j < count; j++) {
			for (i = 0; i < 8; i++)
				block[i] = src[j * 8 + i] ^ dst[j * 8 - 8 + i];
			__cfd_symbol39(block, ks1, ks2, ks3, dst + j * 8);
		}
	}
}

/* Triple-DES-CBC decryption; `in` and `out` must not overlap. */
void __cfd_symbol33(unsigned char *src, unsigned char *key1, unsigned char *key2, unsigned char *key3,
    unsigned char *dst, unsigned char *iv, unsigned int count)
{
	unsigned char block[16];
	unsigned int ks1[32];
	unsigned int ks2[32];
	unsigned int ks3[32];
	unsigned int i, j;

	if (count != 0) {
		__cfd_symbol16(key1, ks1);
		__cfd_symbol16(key2, ks2);
		__cfd_symbol16(key3, ks3);
		__cfd_symbol36(src, ks1, ks2, ks3, block);
		for (i = 0; i < 8; i++)
			dst[i] = block[i] ^ iv[i];
		for (j = 1; j < count; j++) {
			__cfd_symbol36(src + j * 8, ks1, ks2, ks3, block);
			for (i = 0; i < 8; i++)
				dst[j * 8 + i] = block[i] ^ src[j * 8 - 8 + i];
		}
	}
}

/* Two-key triple-DES-CBC encryption: (in, key1, key2, out, iv, count). */
void __cfd_symbol28(unsigned char *src, unsigned char *key1, unsigned char *key2, unsigned char *dst, unsigned char *iv,
    unsigned int count)
{
	unsigned char block[16];
	unsigned int ks1[32];
	unsigned int ks2[32];
	unsigned int i, j;

	if (count != 0) {
		__cfd_symbol16(key1, ks1);
		__cfd_symbol16(key2, ks2);
		for (i = 0; i < 8; i++)
			block[i] = src[i] ^ iv[i];
		__cfd_symbol32(block, ks1, ks2, dst);
		for (j = 1; j < count; j++) {
			for (i = 0; i < 8; i++)
				block[i] = src[j * 8 + i] ^ dst[j * 8 - 8 + i];
			__cfd_symbol32(block, ks1, ks2, dst + j * 8);
		}
	}
}

/* Two-key triple-DES-CBC decryption; `in` and `out` must not overlap. */
void __cfd_symbol26(unsigned char *src, unsigned char *key1, unsigned char *key2, unsigned char *dst, unsigned char *iv,
    unsigned int count)
{
	unsigned char block[16];
	unsigned int ks1[32];
	unsigned int ks2[32];
	unsigned int i, j;

	if (count != 0) {
		__cfd_symbol16(key1, ks1);
		__cfd_symbol16(key2, ks2);
		__cfd_symbol29(src, ks1, ks2, block);
		for (i = 0; i < 8; i++)
			dst[i] = block[i] ^ iv[i];
		for (j = 1; j < count; j++) {
			__cfd_symbol29(src + j * 8, ks1, ks2, block);
			for (i = 0; i < 8; i++)
				dst[j * 8 + i] = block[i] ^ src[j * 8 - 8 + i];
		}
	}
}

/* One-key EDE-CBC encryption: (in, key, out, iv, count). */
void __cfd_symbol21(unsigned char *src, unsigned char *key, unsigned char *dst, unsigned char *iv, unsigned int count)
{
	unsigned char block[16];
	unsigned int ks[32];
	unsigned int i, j;

	if (count != 0) {
		__cfd_symbol16(key, ks);
		for (i = 0; i < 8; i++)
			block[i] = src[i] ^ iv[i];
		__cfd_symbol25(block, ks, dst);
		for (j = 1; j < count; j++) {
			for (i = 0; i < 8; i++)
				block[i] = src[j * 8 + i] ^ dst[j * 8 - 8 + i];
			__cfd_symbol25(block, ks, dst + j * 8);
		}
	}
}

/* One-key EDE-CBC decryption; `in` and `out` must not overlap. */
void __cfd_symbol19(unsigned char *src, unsigned char *key, unsigned char *dst, unsigned char *iv, unsigned int count)
{
	unsigned char block[16];
	unsigned int ks[32];
	unsigned int i, j;

	if (count != 0) {
		__cfd_symbol16(key, ks);
		__cfd_symbol22(src, ks, block);
		for (i = 0; i < 8; i++)
			dst[i] = block[i] ^ iv[i];
		for (j = 1; j < count; j++) {
			__cfd_symbol22(src + j * 8, ks, block);
			for (i = 0; i < 8; i++)
				dst[j * 8 + i] = block[i] ^ src[j * 8 - 8 + i];
		}
	}
}

/* DES-CBC decryption that keeps the previous ciphertext block, so it also
   works in place: (in, key, out, iv, count). */
void __cfd_symbol10(unsigned char *src, unsigned char *key, unsigned char *dst, unsigned char *iv, unsigned int count)
{
	unsigned char block[16];
	unsigned int ks[32];
	unsigned char prev[2][8];
	unsigned int i, j;

	__cfd_symbol16(key, ks);
	__cfd_symbol12(src, ks, block);
	for (i = 0; i < 8; i++)
		prev[0][i] = src[i];
	for (i = 0; i < 8; i++)
		dst[i] = block[i] ^ iv[i];
	for (j = 1; j < count; j++) {
		__cfd_symbol12(src + j * 8, ks, block);
		for (i = 0; i < 8; i++)
			prev[j & 1][i] = src[j * 8 + i];
		for (i = 0; i < 8; i++)
			dst[j * 8 + i] = block[i] ^ prev[(j + 1) & 1][i];
	}
}

/* Triple-DES-CBC decryption that also works in place (see __cfd_symbol10). */
void __cfd_symbol34(unsigned char *src, unsigned char *key1, unsigned char *key2, unsigned char *key3,
    unsigned char *dst, unsigned char *iv, unsigned int count)
{
	unsigned char block[16];
	unsigned int ks1[32];
	unsigned int ks2[32];
	unsigned int ks3[32];
	unsigned char prev[2][8];
	unsigned int i, j;

	if (count != 0) {
		__cfd_symbol16(key1, ks1);
		__cfd_symbol16(key2, ks2);
		__cfd_symbol16(key3, ks3);
		__cfd_symbol36(src, ks1, ks2, ks3, block);
		for (i = 0; i < 8; i++)
			prev[0][i] = src[i];
		for (i = 0; i < 8; i++)
			dst[i] = block[i] ^ iv[i];
		for (j = 1; j < count; j++) {
			__cfd_symbol36(src + j * 8, ks1, ks2, ks3, block);
			for (i = 0; i < 8; i++)
				prev[j & 1][i] = src[j * 8 + i];
			for (i = 0; i < 8; i++)
				dst[j * 8 + i] = block[i] ^ prev[(j + 1) & 1][i];
		}
	}
}

/* Two-key triple-DES-CBC decryption that also works in place. */
void __cfd_symbol27(unsigned char *src, unsigned char *key1, unsigned char *key2, unsigned char *dst, unsigned char *iv,
    unsigned int count)
{
	unsigned char block[16];
	unsigned int ks1[32];
	unsigned int ks2[32];
	unsigned char prev[2][8];
	unsigned int i, j;

	if (count != 0) {
		__cfd_symbol16(key1, ks1);
		__cfd_symbol16(key2, ks2);
		__cfd_symbol29(src, ks1, ks2, block);
		for (i = 0; i < 8; i++)
			prev[0][i] = src[i];
		for (i = 0; i < 8; i++)
			dst[i] = block[i] ^ iv[i];
		for (j = 1; j < count; j++) {
			__cfd_symbol29(src + j * 8, ks1, ks2, block);
			for (i = 0; i < 8; i++)
				prev[j & 1][i] = src[j * 8 + i];
			for (i = 0; i < 8; i++)
				dst[j * 8 + i] = block[i] ^ prev[(j + 1) & 1][i];
		}
	}
}

/* One-key EDE-CBC decryption that also works in place. */
void __cfd_symbol20(unsigned char *src, unsigned char *key, unsigned char *dst, unsigned char *iv, unsigned int count)
{
	unsigned char block[16];
	unsigned int ks[32];
	unsigned char prev[2][8];
	unsigned int i, j;

	if (count != 0) {
		__cfd_symbol16(key, ks);
		__cfd_symbol22(src, ks, block);
		for (i = 0; i < 8; i++)
			prev[0][i] = src[i];
		for (i = 0; i < 8; i++)
			dst[i] = block[i] ^ iv[i];
		for (j = 1; j < count; j++) {
			__cfd_symbol22(src + j * 8, ks, block);
			for (i = 0; i < 8; i++)
				prev[j & 1][i] = src[j * 8 + i];
			for (i = 0; i < 8; i++)
				dst[j * 8 + i] = block[i] ^ prev[(j + 1) & 1][i];
		}
	}
}

/* --- CBC drivers on key schedules already built -------------------------- */

/* DES-CBC encryption: (in, schedule, out, iv, count). */
void __cfd_symbol18(unsigned char *src, void *ks, unsigned char *dst, unsigned char *iv, unsigned int count)
{
	unsigned char block[16];
	unsigned int i, j;

	for (i = 0; i < 8; i++)
		block[i] = src[i] ^ iv[i];
	__cfd_symbol15(block, ks, dst);
	for (j = 1; j < count; j++) {
		for (i = 0; i < 8; i++)
			block[i] = src[j * 8 + i] ^ dst[j * 8 - 8 + i];
		__cfd_symbol15(block, ks, dst + j * 8);
	}
}

/* DES-CBC decryption, also in place: (in, schedule, out, iv, count). */
void __cfd_symbol17(unsigned char *src, void *ks, unsigned char *dst, unsigned char *iv, unsigned int count)
{
	unsigned char block[16];
	unsigned char prev[2][8];
	unsigned int i, j;

	__cfd_symbol12(src, ks, block);
	for (i = 0; i < 8; i++)
		prev[0][i] = src[i];
	for (i = 0; i < 8; i++)
		dst[i] = block[i] ^ iv[i];
	for (j = 1; j < count; j++) {
		__cfd_symbol12(src + j * 8, ks, block);
		for (i = 0; i < 8; i++)
			prev[j & 1][i] = src[j * 8 + i];
		for (i = 0; i < 8; i++)
			dst[j * 8 + i] = block[i] ^ prev[(j + 1) & 1][i];
	}
}

/* Triple-DES-CBC encryption: (in, k1, k2, k3, out, iv, count). */
void __cfd_symbol41(unsigned char *src, void *ks1, void *ks2, void *ks3, unsigned char *dst, unsigned char *iv,
    unsigned int count)
{
	unsigned char block[16];
	unsigned int i, j;

	if (count != 0) {
		for (i = 0; i < 8; i++)
			block[i] = src[i] ^ iv[i];
		__cfd_symbol39(block, ks1, ks2, ks3, dst);
		for (j = 1; j < count; j++) {
			for (i = 0; i < 8; i++)
				block[i] = src[j * 8 + i] ^ dst[j * 8 - 8 + i];
			__cfd_symbol39(block, ks1, ks2, ks3, dst + j * 8);
		}
	}
}

/* Triple-DES-CBC decryption, also in place: (in, k1, k2, k3, out, iv, count). */
void __cfd_symbol40(unsigned char *src, void *ks1, void *ks2, void *ks3, unsigned char *dst, unsigned char *iv,
    unsigned int count)
{
	unsigned char block[16];
	unsigned char prev[2][8];
	unsigned int i, j;

	if (count != 0) {
		__cfd_symbol36(src, ks1, ks2, ks3, block);
		for (i = 0; i < 8; i++)
			prev[0][i] = src[i];
		for (i = 0; i < 8; i++)
			dst[i] = block[i] ^ iv[i];
		for (j = 1; j < count; j++) {
			__cfd_symbol36(src + j * 8, ks1, ks2, ks3, block);
			for (i = 0; i < 8; i++)
				prev[j & 1][i] = src[j * 8 + i];
			for (i = 0; i < 8; i++)
				dst[j * 8 + i] = block[i] ^ prev[(j + 1) & 1][i];
		}
	}
}

/* Derives 24 bytes of key material into `dst` with __cfd_symbol42, then
   overwrites the temporary buffer with random bytes. */
int __cfd_symbol2(unsigned char *dst, unsigned int addr, unsigned int size)
{
	unsigned char buf[144];
	unsigned int i;
	unsigned char *p;

	__cfd_symbol42(buf, 128, buf + 128, (addr + size) - 512, 56, cfd_symbol2_lit1, cfd_symbol2_lit2);
	p = buf;
	i = 0;
	for (; i < 24; i++) {
		dst[i] = p[i];
	}
	__cfd_symbol3(buf, 128);
	return 0;
}
