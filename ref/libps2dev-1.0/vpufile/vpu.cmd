/* vu linker script */

OUTPUT_ARCH(mips:5900)
SECTIONS {
	.vutext		0x0: { *(.vutext)}
	.vudata		0x0: AT ( SIZEOF(.vutext) + ALIGN(128) )
				{ *(.vubss) *(.vudata) }
	/DISCARD/	: { *(.*) }
	/*
	.DVP.ovlystrtab
	.DVP.ovlytab
	.DVP.overlay.
	*/
}
