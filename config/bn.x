/* bn.x: linker script for linking bn (pipeline: docs/linking.md).
   Used by tools/relink/link.sh (`make link`). It lives here, next to units.txt, because it
   is CONFIGURATION of the binary (section addresses of the original, order of the output
   sections), not pipeline code, and it is meant to stay the script of the real link. It
   holds only addresses and section names (like config/symbol_addrs.txt), no bytes of the
   original.

   Base: the built-in script of ld 2.9-ee-990721 for elf32lsmip (dynamic executable),
   `docker/toolchain/sonybinutils/mipsEEel-linux-ld --verbose`.
   Differences from the default:
   - every allocated output section starts at the address of the original
     (readelf -lSW orig/bn-jpn.elf), so a difference stays confined to its section and
     tools/relink/cmp.py measures it section by section instead of seeing a cascade; if
     the content is longer than the original, ld reports the overlap (the link succeeds
     without --no-check-sections);
   - .rel.dyn after .rodata and .rld_map after .data as in the original (the linker creates
     them: in the default script they are orphans);
   - .note.ABI-tag (an orphan from crt1.o in the default script) after .interp as in the
     original;
   - .lit*, .gptab: none of our objects has them; they stay as in the default.

   FIXED (from the original): addresses of .init .text .fini .rodata .rel.dyn .data .rld_map
   .eh_frame .gcc_except_table .ctors .dtors .got .sbss; _gp = 0x100d9910 (= .got + 0x7ff0,
   like the default ALIGN(16)+0x7ff0). The leading sections (.interp ... .gnu.version_r)
   follow SIZEOF_HEADERS as in the default and fall at the original addresses by themselves.
   NOT FIXED (ld decides, cmp.py measures): content of .hash, .dynsym, .dynstr,
   .gnu.version, .gnu.version_r, .got, .dynamic, .rel.dyn, .compact_rel.
   - .fini: QUAD(0)x3 at the end as in the default (24 zero bytes, 0x93bf28..0x93bf40). The
     other 24 zero bytes between the .fini of crtbegin and that of crtn come from the
     `ld -r` of config/relink_partial_r.txt (the -r script closes .fini with QUAD(0)x3);
   - no PHDRS: `mipsEEel-linux-strip` 2.9EE without options on the linked image reproduces
     the PHDRs (7 segments, p_filesz 0xe0), sections and shstrndx of the original;
   - .compact_rel and .mdebug are non-ALLOC orphans (outside this script): their order in
     the file before stripping is decided by the kit's crt1.o, linked with its .mdebug. */
OUTPUT_FORMAT("elf32-littlemips", "elf32-bigmips", "elf32-littlemips")
OUTPUT_ARCH(mips)
ENTRY(__start)
SECTIONS
{
  . = 0x0400000 + SIZEOF_HEADERS;
  .interp : { *(.interp) }
  .note.ABI-tag : { *(.note.ABI-tag) }
  .reginfo : { *(.reginfo) }
  .dynamic : { *(.dynamic) }
  .hash : { *(.hash) }
  .dynsym : { *(.dynsym) }
  .dynstr : { *(.dynstr) }
  .gnu.version : { *(.gnu.version) }
  .gnu.version_r : { *(.gnu.version_r) }
  .init        0x0047c168 : { KEEP (*(.init)) } =0
  .text        0x0047c1d0 :
  {
    _ftext = . ;
    *(.text)
    *(.text.*)
    *(.stub)
    *(.gnu.warning)
    *(.gnu.linkonce.t.*)
  } =0
  .fini        0x0093beb8 :
  {
    KEEP (*(.fini))
    QUAD(0); QUAD(0); QUAD(0)
  } =0
  PROVIDE (__etext = .);
  PROVIDE (_etext = .);
  PROVIDE (etext = .);
  .rodata      0x0093bf40 : { *(.rodata) *(.rodata.*) *(.gnu.linkonce.r.*) }
  .rel.dyn     0x00978f50 : { *(.rel.dyn) }

  .data        0x10000000 :
  {
    _fdata = . ;
    *(.data)
    *(.data.*)
    *(.gnu.linkonce.d.*)
    SORT(CONSTRUCTORS)
  }
  .rld_map     0x100cb858 : { *(.rld_map) }
  .eh_frame    0x100cb85c : { KEEP (*(.eh_frame)) }
  .gcc_except_table 0x100d0cfc : { *(.gcc_except_table) }
  .ctors       0x100d17a0 :
  {
    KEEP (*crtbegin.o(.ctors))
    KEEP (*(SORT(.ctors.*)))
    KEEP (*(.ctors))
  }
  .dtors       0x100d18ac :
  {
    KEEP (*crtbegin.o(.dtors))
    KEEP (*(SORT(.dtors.*)))
    KEEP (*(.dtors))
  }
  _gp = 0x100d1920 + 0x7ff0;
  .got         0x100d1920 : { *(.got.plt) *(.got) }
  .sdata       : { *(.sdata) *(.sdata.*) *(.gnu.linkonce.s.*) }
  _edata = .;
  PROVIDE (edata = .);
  __bss_start = .;
  _fbss = .;
  .sbss        0x100db254 :
  {
    PROVIDE (__sbss_start = .);
    PROVIDE (___sbss_start = .);
    *(.dynsbss)
    *(.sbss)
    *(.sbss.*)
    *(.gnu.linkonce.sb.*)
    *(.scommon)
    PROVIDE (__sbss_end = .);
    PROVIDE (___sbss_end = .);
  }
  .bss         :
  {
   *(.dynbss)
   *(.bss)
   *(.bss.*)
   *(.gnu.linkonce.b.*)
   *(COMMON)
   . = ALIGN(32 / 8);
  }
  . = ALIGN(32 / 8);
  _end = .;
  PROVIDE (end = .);
  .comment 0 : { *(.comment) }
  .gptab.sdata : { *(.gptab.data) *(.gptab.sdata) }
  .gptab.sbss : { *(.gptab.bss) *(.gptab.sbss) }
}
